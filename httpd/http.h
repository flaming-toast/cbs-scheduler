/* cs194-24 Lab 1 */


#ifndef HTTP_H
#define HTTP_H

#include <sys/epoll.h>
#include <pthread.h>

#include "palloc.h"

/* Allows HTTP sessions to be transported over the HTTP protocol */
struct http_session
{
        const char * (*gets)(struct http_session *);

        ssize_t (*puts)(struct http_session *, const char *);

        ssize_t (*write)(struct http_session *, const char *, size_t);

        /* Stores a resizeable, circular buffer */
        char *buf;
        size_t buf_size, buf_used;

        /* Lab 1 - epoll stuff */
        struct epoll_event event;

        struct http_server *server;

        /* For retrying requests if could not write to session fd at the time */
        struct mimetype *mt_retry;

        /* For building the current GET request associated with this session */
        struct http_get_request *get_req;

	/* For building the current CGI response (to act as a temporary buffer holding 
	 * http response fields before aborting a CGI script if the request can be 
	 * fulfilled via the client's browser cache. */
	struct http_get_response *get_response;

    int fd;

        /* Ensure that only one thread gets to process this session @ a time */
        pthread_mutex_t fd_lock;
};

/* A server that listens for HTTP connections on a given port. */
struct http_server
{
        struct http_session * (*wait_for_client)(struct http_server *);

        int fd;

        /* Global epoll set */
        struct epoll_event event;
        /* Buffer in which events will be returned from epoll_wait */
        struct epoll_event *events_buf;
        /* epoll descriptor. call epoll_wait() with this descriptor. */
        int efd;

};

/* Creates a new HTTP server listening on the given port. */
struct http_server *http_server_new(palloc_env env, short port);

/* Wrapper struct to provide easy access from epoll_event.data.ptr */
struct http_event
{
        int session;
        void *ptr;
};

/* For building GET requests while reading lines from the session fd */
struct http_get_request
{
	struct http_session *session; 
	struct mimetype *mt; 
	/* Expires field */
	char *cache_control;
	/* The etag we send to the client */
	char *if_none_match;
	/* the request string (e.g. GET /index.html HTTP/1.1 etc */
	char *request_string; 
	/* Add any other HTTP request fields of interest here */


};

/* Mainly to help build conditional CGI responses */
struct http_get_response
{
	struct http_session *session; 
	struct mimetype *mt; 
	/* Expires field */
	char *expires;
	/* The etag we send to the client */
	char *etag;
	/* Add any other HTTP response fields of interest here */
	char *response_string;
};


#endif
