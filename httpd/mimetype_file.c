/* cs194-24 Lab 1 */

#include "mimetype_file.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>

#define BUF_COUNT 4096
#define _GNU_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE

static int http_get(struct mimetype *mt, struct http_session *s);

struct mimetype *mimetype_file_new(palloc_env env, const char *fullpath)
{
    struct mimetype_file *mtf;

    mtf = palloc(env, struct mimetype_file);
    if (mtf == NULL)
	return NULL;

    mimetype_init(&(mtf->mimetype));

    mtf->http_get = &http_get;
    mtf->fullpath = palloc_strdup(mtf, fullpath);

    return &(mtf->mimetype);
}

int http_get(struct mimetype *mt, struct http_session *s)
{
	fprintf(stderr, "http_get\n");
    struct mimetype_file *mtf;
    int fd; /* for reading file from disk */
//    FILE *f;
    int pipefd[2];  /* for cgi */
    char buf[BUF_COUNT];
    ssize_t readed;
    pid_t childpid;

    mtf = palloc_cast(mt, struct mimetype_file);
    if (mtf == NULL)
		return -1;

	/* Simple CGI implementation */
    pipe(pipefd);
    if (access(mtf->fullpath, F_OK|X_OK)){

    	/* static files */
    	/* First check if we can send a 304 */
    	char *etag = s->get_req->if_none_match;
		struct stat filestat;
		char time[sizeof(long)*3];

		if (stat(mtf->fullpath, &filestat) < 0) {
			perror("Could not stat requested file");
		}

		struct tm lt;
		localtime_r(&filestat.st_mtime, &lt);
		fprintf(stderr, "st_mtime from stat: %lx\n", (long)filestat.st_mtime);
		snprintf(time, sizeof time, "%lx", (long)filestat.st_mtime);
		fprintf(stderr, "snprintf:%s\n", time);
		fprintf(stderr, "ETAG WE SAVED FROM THE CLIENT:%s\n", etag);

    	if (etag != NULL) { // We had sent an ETag to this client before
    		if (strcasecmp(time, etag) == 0) { // etags do not match
				fprintf(stderr, "Sending 304 to client\n");
    			s->puts(s, "HTTP/1.1 304 Not Modified\r\n");
    			s->puts(s, "\r\n");
    			return 0; // we are done here
    		}
    	}

		/* We did not receive a If-None-Match from the client, so we send the full response. */
    	fd = open(mtf->fullpath, O_RDONLY);

		/* session->fd must be ready for writing */
    	s->puts(s, "HTTP/1.1 200 OK\r\n");
    	s->puts(s, "Content-Type: text/html\r\n");
    	s->puts(s, "ETag:"); // send an etag so the client can now send conditional GET's
	   	s->puts(s, time);
    	s->puts(s, "\r\n");
    	s->puts(s, "\r\n");

		if (fd < 0) {
			perror("open() failed");
		}
    } else {
		// Before forking and exec, check if cache response still good.
		// if cache_entry.expires exists
		// 	check if current time is before expires
		// 		struct timeval tv, ctv, rtv;
		// 		struct tm *tm, *ctm, *rtm;
		// 		gettimeofday(&tv, NULL);
		// 		
		//  timercmp(tv, expires)
		// 	if it is, send 304 and return
		//else 
		//  continue to fork
		
		
		childpid = fork();
		if (!childpid) {
			dup2(pipefd[1], 1); // duplicate std out to pipefd and close it
			execl(mtf->fullpath, "", NULL);
			exit(0);
		} else {
			dup2(pipefd[0], 0); // duplicate std in to pipefd and close it
			fd = pipefd[0]; //read from child

			FILE *f;
			f = fdopen(fd, "r");

			close(pipefd[1]);


			char line[80];
			char *prot, *response_code, *msg;
			prot = palloc_array(s, char, strlen(line));
			response_code = palloc_array(s, char, strlen(line));
			msg = palloc_array(s, char, strlen(line));
			int ret;
			char *http_field, *http_field_value;
			http_field = palloc_array(s, char, strlen(line));
			http_field_value = palloc_array(s, char, strlen(line));

			struct http_get_response *cgi_response = s->get_response;

			int count = 0; // omg this is quite ghetto..

			while (fgets(line, 80, f) != NULL) {
				//	fprintf(stderr, line);
				if (count < 5) {
					count++; // So we don't end up reading the entire response to find HTTP headers -_-

					if (count == 1 && sscanf(line, "%s %s %s", prot, response_code, msg) == 3 ) {
						if (strcasecmp(prot, "HTTP/1.0") == 0 || strcasecmp(prot, "HTTP/1.1") == 0) {
							fprintf(stderr, "CGI response found HTTP response: [%s]\n", prot);
						} else {
							count = 5; // Stop checking for HTTP headers if the first line isn't even a proper HTTP resonse.
						}
					} 
					
					ret = sscanf(line, "%[^:]: %[^\n\r]", http_field, http_field_value);
					if (ret == 2) {
						if (strcasecmp(http_field, "Expires") == 0) { // put expiration in cache, check with subsequent requests
							cgi_response->expires = http_field_value;
							fprintf(stderr, "CGI response found Expires: [%s] \n", cgi_response->expires);
					
							// sample time comparison code
							struct tm tmtime;
							struct tm tmtime2;
							time_t time, time2;
							struct timeval tv, tv2;
							strptime("Thu, 27 Feb 2014 16:19:00 +0000", "%a, %d %b  %Y %T", &tmtime);
							strptime("Wed, 26 Feb 2014 16:20:00 +0000", "%a, %d %b  %Y %T", &tmtime2);
							
							time = mktime(&tmtime); // seconds since the epoch
							tv.tv_sec = time;

							time2 = mktime(&tmtime2); // seconds since the epoch
							tv2.tv_sec = time2;

							fprintf(stderr, "strptime results: time.tv_sec: %ld ", tv.tv_sec);
							fprintf(stderr, "1st time comes before second time %d ", timercmp(&tv, &tv2, <));

						}

						if (strcasecmp(http_field, "ETag") == 0) { // etag
							// if cache_entry.etag exists
							// check if http_field_value matches cache_entry.etag
							// if matches
							// kill(childpid) -- vedant says another group is doing this, not the best way, but at least I'm checking the cache before doing this.
							// send 304 response and return 0;
							cgi_response->etag = http_field_value;
							fprintf(stderr, "CGI response found ETag: [%s] \n", cgi_response->etag);
						}
					}
				}
				s->puts(s, line);
			}

			/* After we finish reading from the pipe */
			// put buffer of cgi response lines in cache 
			// if cgi_response->etag and cgi_response->expires exists
			// add them to the cache as well.

			close(fd);
			return 0;
			//if line matches Expires, put in cache entry
			//if Etag is generated, put in cache entry?

		}
	}
			


    //long tid = syscall(SYS_gettid);
    //fprintf(stderr,"Thread %ld: Printed out HTTP 200 OK\n", tid);

	//		close(pipefd[0]);



	/* see http://stackoverflow.com/questions/8057892/epoll-on-regular-files */
	/* It is considered correct to block on disk I/O requests */

	// set to listen to epollout events only.

	
	close(pipefd[1]); //stop communcation, signal end of data

    while ((readed = read(fd, buf, BUF_COUNT)) > 0)
    {
    	fprintf(stderr, "############################");
		ssize_t written;
		written = 0;
/*
		if (!checked && (strstr(buf, "HTTP/1.1") == buf) || (strstr(buf, "HTTP/1.0") == buf)) {
			fprintf(stderr, "FOUND HTTP HEAPFDHSAKLF;JSADK;FJA;SDKL");
			checked = 1;
			keepgoing = 1;
		} else {
			keepgoing = 0;
		}
*/

		while (written < readed) /* while # of written bytes < read bytes */
		{
	    	ssize_t w;
	    	/* session->fd must be ready for writing */
	    	/* write readed-written (remaining bytes) from buf+written to session->fd */
	    	w = s->write(s, buf+written, readed-written);
	    	if (w > 0)
				written += w;
			if (errno == EAGAIN) { // If I can't write at this time
				s->mt_retry = mt; // save for next event loop run
				s->event.events = EPOLLOUT | EPOLLET; // listen only for epollout events, in edge triggered mode
				if(epoll_ctl(s->server->efd, EPOLL_CTL_MOD, s->fd, &s->event) < 0) {
    				//fprintf(stderr, "Thread %ld: epoll_ctl failed to modify session fd %d to listen for only EPOLLOUT", tid, s->fd);
    			}
			}
		}
    	fprintf(stderr,"written %d to session # %d\n", (int)written, s->fd);
    }
    fprintf(stderr,"Printed out RESPONSE to session # %d \n", s->fd);
    fprintf(stderr,"readed %d \n", readed);

	s->event.events = EPOLLIN | EPOLLET; // if we happen to have switched to EPOLLOUT in the meantime and we succeed this time, switch back to EPOLLIN
	if(epoll_ctl(s->server->efd, EPOLL_CTL_MOD, s->fd, &s->event) < 0) {
    	//fprintf(stderr, "Thread %ld: epoll_ctl failed to modify session fd %d to listen for only EPOLLOUT", tid, s->fd);
    }

    close(fd);
    close(pipefd[0]);
    //perror("close() requested file");

    return 0;
}
