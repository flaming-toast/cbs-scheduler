/* cs194-24 Lab 1 */

#define _POSIX_C_SOURCE 1
#define _BSD_SOURCE

#define MAX_PENDING_CONNECTIONS 8
#define DEFAULT_BUFFER_SIZE 256
#define MAX_EVENTS 10

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include "http.h"
#include "lambda.h"
#include "palloc.h"


struct string
{
    size_t size;
    char *data;
};

static int listen_on_port(short port);
static struct http_session *wait_for_client(struct http_server *serv);
static struct http_event *http_event_new(palloc_env env,int session, void *ptr);

static int close_session(struct http_session *s);

static const char *http_gets(struct http_session *s);
static ssize_t http_puts(struct http_session *s, const char *m);
static ssize_t http_write(struct http_session *s, const char *m, size_t l);

extern int process_session_data(struct http_session *s);

struct http_server *http_server_new(palloc_env env, short port)
{
    struct http_server *hs;

    hs = palloc(env, struct http_server);
    if (hs == NULL)
	return NULL;

    hs->wait_for_client = &wait_for_client;

	/* listen_on_port() will return a non-blocking fd */
    hs->fd = listen_on_port(port);


	/* create global epoll set associated with this http_server */
	hs->efd = epoll_create1(0);
	if (hs->efd == -1) {
		perror("epoll_create(): unable to create epoll descriptor");
		abort();
	}

	/* ptr to hs http_server struct */
	hs->event.data.ptr = http_event_new(env, 0, hs);
	/* flags to indicate what events we'll listen for 
	 * Only interested in EPOLLIN events on the socket fd since we're just listening */
	/* Need edge triggered so that one thread will be woken up to handle the incoming connection, not multiple threads */
    hs->event.events = EPOLLIN | EPOLLET;
    /* allocate buffer in which events will be returned */
	hs->events_buf = calloc(MAX_EVENTS, sizeof(hs->event));
	/* Add socket fd to epoll set, which will notify us of incoming new connections. */
    if (epoll_ctl(hs->efd, EPOLL_CTL_ADD, hs->fd, &hs->event) < 0) { 
    	perror("http_server_new(): epoll_ctl failed to add the socket fd to epoll set");
    	abort();
    }

    return hs;
}

int listen_on_port(short port)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int so_true;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
	return -1;

	/* Lab 1 - make socket non-blocking */
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("listen_on_port(): fcntl failed to set socket to  O_NONBLOCK");
	}

    /* SO_REUSEADDR allows a socket to bind to a port while there
     * are still outstanding TCP connections there.  This is
     * extremely common when debugging a server, so we're going to
     * use it.  Note that this option shouldn't be used in
     * production, it has some security implications.  It's OK if
     * this fails, we'll just sometimes get more errors about the
     * socket being in use. */
    so_true = true;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &so_true, sizeof(so_true));

    addr_len = sizeof(addr);
    memset(&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, addr_len) < 0)
    {
	perror("listen_on_port(): Unable to bind to HTTP port");
	close(fd);
	return -1;
    }

    if (listen(fd, MAX_PENDING_CONNECTIONS) < 0)
    {
	perror("listen_on_port(): Unable to listen on HTTP port");
	close(fd);
	return -1;
    }

    return fd;
}

struct http_session *wait_for_client(struct http_server *serv)
{
    struct http_session *sess;
    struct sockaddr_in addr;
    socklen_t addr_len;

    sess = palloc(serv, struct http_session);
    if (sess == NULL)
	return NULL;

    sess->gets = &http_gets;
    sess->puts = &http_puts;
    sess->write = &http_write;
    sess->server = serv;

	/* char array of size 256, sess context */	
    sess->buf = palloc_array(sess, char, DEFAULT_BUFFER_SIZE);
    /* zero it */
    memset(sess->buf, '\0', DEFAULT_BUFFER_SIZE);
    sess->buf_size = DEFAULT_BUFFER_SIZE;
    sess->buf_used = 0;

    addr_len = sizeof(addr);

    /* Wait for a client to connect. */
    /* Make sess->fd non-blocking, add to epoll set */
    sess->fd = accept(serv->fd, (struct sockaddr *)&addr, &addr_len);
    long tid = syscall(SYS_gettid);
    fprintf(stderr, "Thread %ld: accept() returns sess->fd = %d\n", tid, sess->fd);

    if (sess->fd < 0)
    {
		perror("wait_for_client(): Unable to accept on client socket");
		pfree(sess);
		return NULL;
    }

    int flags = fcntl(sess->fd, F_GETFL, 0);
    if (fcntl(sess->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		perror("wait_for_client(): Unable to mark accept'd socket non-blocking");
		pfree(sess);
		return NULL;
	}

	/* Set up epoll_event struct associated with this http_session */
	/* Pointer to this http_session */
//	sess->event.data.ptr = http_event_new(serv, 1, sess);
	sess->event.data.ptr = http_event_new(sess, 1, sess);
	/* Flags to indicate what events we'll listen for on the accepted connection socket */
	/* EPOLLONESHOT -- only one thread will get a notif and no other threads will for the time being... */
	/* must reaarm in event loop */
    sess->event.events = EPOLLIN | EPOLLET; // add EPOLLOUT when you write and get EAGAIN
    /* Add to http_server's epoll instance, listen for events on sess->fd */
    if (epoll_ctl(serv->efd, EPOLL_CTL_ADD, sess->fd, &sess->event) < 0) {
    	perror("epoll_ctl");
    	abort();
    }
	int count;
	ioctl(sess->fd, FIONREAD, &count);
		if (count > 0) {
		int ret = process_session_data(sess);
			if (ret < 0) {
				perror("process_session_data");
			}
    	
    	}

    palloc_destructor(sess, &close_session);

    return sess;
}

int close_session(struct http_session *s)
{
    if (s->fd == -1)
	return 0;

    close(s->fd);
    s->fd = -1;

    return 0;
}

/* Read chars from session fd until a newline */
const char *http_gets(struct http_session *s)
{
    while (true)
    {
	char *newline;
	ssize_t readed;

	/* if a newline is reached we are done */
	if ((newline = strstr(s->buf, "\r\n")) != NULL)
	{
	    char *new;

	    *newline = '\0';
	    new = palloc_array(s, char, strlen(s->buf) + 1);
	    strcpy(new, s->buf);

		/* I think you copy the line but remove the newline chars, 
		 * then return the char array representing the line*/
	    memmove(s->buf, s->buf + strlen(new) + 2,
		s->buf_size - strlen(new) - 2);
	    s->buf_used -= strlen(new) + 2;
	    s->buf[s->buf_used] = '\0';
	    fprintf(stderr, "Returning a line: %s\n", new);
	    return new;
	}

	/* read(fd, buffer, count) */
	/* initially read 256-0 */
	/* Read until a newline is reached */
	readed = read(s->fd, s->buf + s->buf_used, s->buf_size - s->buf_used);
	if (readed > 0) {
    	long tid = syscall(SYS_gettid);
    	fprintf(stderr, "Thread %ld: read %d sess->fd = %d\n", tid, (int)readed, s->fd);
	    s->buf_used += readed;
	}
	if (readed <= 0) 
		break;

	if (errno == EAGAIN) {
		s->event.events = EPOLLIN | EPOLLET;
		epoll_ctl(s->server->efd, EPOLL_CTL_MOD, s->fd, &s->event);
	}

	if (s->buf_used >= s->buf_size)
	{
	    s->buf_size *= 2;
	    s->buf = prealloc(s->buf, s->buf_size);
	}
    }

    return NULL;
}

/* write a line to session->fd */
ssize_t http_puts(struct http_session *s, const char *m)
{
    size_t written;

    written = 0;
    while (written < strlen(m))
    {
	ssize_t writed;

	writed = write(s->fd, m + written, strlen(m) - written);
	if (writed < 0)
	    return -1 * written;

	written += writed;
    }

    return written;
}

/* write to session->fd 
 * write(fd, buffer, count)
 */
ssize_t http_write(struct http_session *s, const char *m, size_t l)
{
    int w = write(s->fd, m, l);
    return w;
}


struct http_event *http_event_new(palloc_env env, int session, void *ptr)
{
	struct http_event *ev;

    ev = palloc(env, struct http_event);
    if (ev == NULL)
	return NULL;

	ev->session = session;
	ev->ptr = ptr;
	return ev;
}
