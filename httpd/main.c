/* cs194-24 Lab 1 */

#include <stdbool.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <errno.h>

#include "http.h"
#include "mimetype.h"
#include "palloc.h"

#define PORT 8088
#define LINE_MAX 1024

#define DEFAULT_BUFFER_SIZE 256

static int event_loop(struct http_server *server);
extern int process_session_line(struct http_session *session, const char *data);
extern int process_session_data(struct http_session *session);
//static pthread_mutex_t lock;

int main(int argc, char **argv)
{
    palloc_env env;
    struct http_server *server;
    int num_threads;
	int  i, ret;

    env = palloc_init("httpd root context");

    /* create socket, bind, make non-blocking, listen */
    /* also set up epoll events */
    server = http_server_new(env, PORT);
    if (server == NULL)
    {
		perror("main(): Unable to open HTTP server");
		return 1;
    }

	/* Create N threads, where N is the number of cores */
	//    num_threads = get_nprocs();
	num_threads = 2;
    pthread_t * thread = malloc(sizeof(pthread_t)*num_threads);

    for (i = 0; i < num_threads; i++) {

    	ret = pthread_create(&thread[i], NULL, (void *)&event_loop, server);

    	if(ret != 0) {
    		////fprintf (stderr, "Create pthread error!\n");
    		exit (1);
    	}
    }

	/* Now that each thread should be running the event loop, we wait... */
    pthread_join(thread[0], NULL);

    return 0;
}

int event_loop(struct http_server *server) {

    while (true) /* main event loop */
    {
		//long tid;
		//tid = syscall(SYS_gettid);
		////fprintf(stderr, "Thread %ld event loop() \n", tid);

		struct http_session *session;
		int num_events_ready;
		int i, ret;

		/* Apparently epoll in edge-triggered mode, it only wakes up one thread at a time */
		////fprintf(stderr, "Thread %ld in epoll_wait()\n", tid);
		num_events_ready = epoll_wait(server->efd, server->events_buf, 5, -1);
		////fprintf(stderr, "Thread %ld return from epoll_wait()\n", tid);

		for (i = 0; i < num_events_ready; i++) { // loop through available events

			struct epoll_event ev = server->events_buf[i];
			struct http_event *he = (struct http_event *)ev.data.ptr;

			/* Handle any weird errors */
			if ((ev.events & EPOLLERR) ||
					(ev.events & EPOLLHUP) ||
					(!(ev.events & EPOLLIN)))
			{
			 	/* An error occured on this fd.... */
			 	////fprintf(stderr, "epoll error\n");
			 	(he->session) ? close(((struct http_session *)he->ptr)->fd) : close(((struct http_server *)he->ptr)->fd);
			 	continue;
			}

			/* We got a notification on the listening socket, which means 1+ incoming connection reqs */
			else if (!he->session && server->fd == ((struct http_server *)he->ptr)->fd) {

				/* accept() extracts the first connection request on the queue of pending connections
				 * for the listening socket, creates a new connected socket,
				 * and returns a new file descriptor referring to that socket.
				 * While loop drains the pending connection buffer, important if concurrent requests are being made.
				 * Can't just accept 1 connection per run */
				if (ev.events & EPOLLIN) {
					while ((session = server->wait_for_client(server)) != NULL)
					{
						//fprintf(stderr, "Thread %ld: accepted a new client connection\n", tid);
          			}
				}
				/*
				   if (session == NULL)
				   {
	    		   perror("server->wait_for_client() returned NULL...");
	    		   pfree(server);
	    		   return 1;
				   }
				   */
			} else { // it is not the listening socket fd, but one of the accept'd session fd's

				session = (struct http_session *)he->ptr; /* points to a http_session struct */
				//fprintf(stderr, "Thread %ld: Session fd received event from session %d\n", tid, session->fd);

				/* Ensure only one thread processes a session fd at a time */
				if (pthread_mutex_trylock(&session->fd_lock) == 0) {
					if ((ev.events & EPOLLOUT) && !(ev.events & EPOLLIN)) { // If I had only registered EPOLLOUT events
						// retry the request and see if you can write to session fd now
						int mterr = session->mt->http_get(session->mt, session);
						if (mterr != 0) {
							perror("unrecoverable error while processing a client");
							abort();
						}
					} // I DONT THINK THIS IS EVER BEING CALLED :'(


					/* Read everything available from sess->fd until we hit EAGAIN*/
					if (((ev.events & EPOLLIN) == EPOLLIN) && session->fd > 0) { // htf did a -1 sess->fd get in epoll???

    					/* when session->gets() encounters EAGAIN it returns null */
						/* Got a bunch of data from the session fd, they are a bunch of lines like
				 		 * Host: .....
				 		 * User-Agent: .....
				 		 * We are only interested in GET's, ignore everything else.
				 		 */
						/* read lines from session->fd until we get no more lines */
						ret = process_session_data(session);
						if (ret < 0) {
							perror("process_session_data failed");
							//	abort();
						}
						//					close(session->fd);
					} // We got notified but the session was not ready for reading???

					/* We are done processing this session fd for now. Release it. */
					pthread_mutex_unlock(&session->fd_lock);
				}
    		}
    	}
    }
}


int process_session_line(struct http_session *session, const char *line) {
	char *method, *file, *version;
	struct mimetype *mt;
	int mterr;
	//long tid = syscall(SYS_gettid);

	/* What if 2 session fd's signaled events? */
	/* What if 2 threads are handling the same session fd? */

	method = palloc_array(session, char, strlen(line));
	file = palloc_array(session, char, strlen(line));
	version = palloc_array(session, char, strlen(line));
	if (sscanf(line, "%s %s %s", method, file, version) != 3 || strcasecmp(method, "GET") != 0)
	{
		/* just ignore the line */
		//fprintf(stderr, "Thread %ld : Ignoring this line from session %d: %s\n", tid, session->fd, line);
		return 0;

	} else {

		//fprintf(stderr, "Thread %ld  < '%s' '%s' '%s' from session %d \n", tid,  method, file, version, session->fd);
		mt = mimetype_new(session, file);

		if (strcasecmp(method, "GET") == 0)
		{
			/* http_get involves writing back to session fd, reading file from disk,
			 * writing file contents to session fd */
			/* We need to be able to:
			 * 		write to session fd (s->puts(s, <response>))
			 * 		write to session fd (s->write(s, buffer of read bytes from file, length of file)
			 */

			/* We found a GET. process it. */
	    	mterr = mt->http_get(mt, session);
	    }
		else
		{
	    	//fprintf(stderr, "Unknown method: '%s'\n", method);
	    	goto cleanup;
		}

		if (mterr != 0)
		{
	    	perror("unrecoverable error while processing a client");
	    	abort();
		} else {
			/* Successfully processed GET req */
			//close(session->fd); //This is breaking things...
			return 0;
		}
	}

cleanup:
	/* should call palloc destructor close_session
	 * which closes the session fd and should automatically be
	 * removed from the epoll set */
	pfree(session);
	return -1;
}

int process_session_data(struct http_session* session) {

	//long tid;
	//tid = syscall(SYS_gettid);
	const char *line;
	int ret;
	while ((line = session->gets(session)) != NULL) {
		// readed = read(session->fd, buf + buf_used, DEFAULT_BUFFER_SIZE - buf_used);
		/* Process each line we receive */
		ret = process_session_line(session, line);
		if (ret != 0) {
			perror("process_session_data encountered a problem");
			//abort();
			return ret;
		}
	} // after this while loop ends, should have read everything we could have
	//fprintf(stderr, "Thread %ld closing session fd %d \n", tid, session->fd);
	close(session->fd);
	return 0;
}
