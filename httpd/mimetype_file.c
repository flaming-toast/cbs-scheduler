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
#include <stropts.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "cache.h"
#include "palloc.h"


#define BUF_COUNT 4096
#define _GNU_SOURCE
#define __USE_XOPEN

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
    struct mimetype_file *mtf;
    int fd; /* for reading file from disk */
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
	//fprintf(stderr, "st_mtime from stat: %lx\n", (long)filestat.st_mtime);
	snprintf(time, sizeof time, "%lx", (long)filestat.st_mtime);
//	fprintf(stderr, "snprintf:%s\n", time);
//      fprintf(stderr, "ETAG WE SAVED FROM THE CLIENT:%s\n", etag);

    	if (etag != NULL) { // We had sent an ETag to this client before, and they are sending us back an If-None-Match
    	    if (strcasecmp(time, etag) == 0) { // etags do match
//		fprintf(stderr, "Sending 304 to client\n");
    		s->puts(s, "HTTP/1.1 304 Not Modified\r\n");
    		s->puts(s, "\r\n");
    		close(fd);
    		return 0; // we are done here
    	    }
    	}

	/* We did not receive a If-None-Match from the client, so we have to send the full response. */

	/* Try the cache first, and give it an etag for browser caching for subsequent requests */
	struct cache_entry *trycache;
	if ((trycache = cache_get(s->get_req->request_string)) != NULL) { // cache hit
	    const char *response = trycache->response;
    	    s->puts(s, "HTTP/1.1 200 OK\r\n");
    	    s->puts(s, "Content-Type: text/html\r\n");
    	    s->puts(s, "ETag:"); // send an etag so the client can now send conditional GET's
	    s->puts(s, time);
    	    s->puts(s, "\r\n");
    	    s->puts(s, "\r\n");
	    s->write(s, response, sizeof(response));
	    return 0;
	}

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

    } else { // Handle CGI requests
	// Before forking and exec, check if cache response still good.
	fprintf(stderr, "Handling CGI request");
	struct cache_entry *c;
	c = cache_get(s->get_req->request_string);
	if ((c != NULL) && (c->expires != NULL)) {

	    fprintf(stderr, "Cached exp time: %s", c->expires);
	    fprintf(stderr, "Cached etag time: %s", c->etag);

	    struct tm cache_expire_tm;
	    struct tm now_tm;
	    time_t cache_expire, nowtime;
	    struct timeval cache_expire_tv, now_tv;
	    int gerr;
	    char nowtime_tmbuf[64], nowtime_buf[64];

	    /* Get current time */
	    gerr = gettimeofday(&now_tv, NULL);
	    if (gerr < 0) {
		perror("gettimeofday errored");
	    }

	    //			nowtime = now_tv.tv_sec;
	    gmtime_r(&(now_tv.tv_sec), &now_tm); //localtime or gmtime? may segfault...

	    strftime(nowtime_tmbuf, sizeof nowtime_tmbuf, "%a, %d %b  %Y %T", &now_tm);
	    snprintf(nowtime_buf, sizeof buf, "%s", nowtime_tmbuf); // nowtime_buf should contain time string

	    strptime(c->expires, "%a, %d %b  %Y %T", &cache_expire_tm);
	    strptime(nowtime_tmbuf, "%a, %d %b  %Y %T", &now_tm);

	    cache_expire = mktime(&cache_expire_tm); // seconds since the epoch
	    cache_expire_tv.tv_sec = cache_expire;

	    nowtime = mktime(&now_tm); // seconds since the epoch
	    now_tv.tv_sec = nowtime;

	    fprintf(stderr, "Does current time comes before expiration time (cache entry still good) %d ", timercmp(&now_tv, &cache_expire_tv, <));
	    fprintf(stderr, "Now time: %s", nowtime_tmbuf);
	    //fprintf(stderr, "Cached exp time: %s", c->expires);


	    if(timercmp(&now_tv, &cache_expire_tv, <) ) { // returns non-zero if true
	      fprintf(stderr, "Sending 304 to client\n");
    		s->puts(s, "HTTP/1.1 304 Not Modified\r\n");
    		s->puts(s, "Content-Type: text/html\r\n");
    		s->puts(s, "\r\n");
        s->write(s, "", 1);
    		return 0; // we are done here
	    }

	}
	/* if it hasn't expired but the etag from the client is different from our cache...*/
	if (c != NULL) {
          if (c->etag != NULL && s->get_req->if_none_match != NULL && (strcasecmp(c->etag, s->get_req->if_none_match) != 0)) {
                  const char *response = c->response;
                  s->puts(s, "HTTP/1.1 200 OK\r\n");
                  s->puts(s, "Content-Type: text/html\r\n");
                  s->puts(s, "ETag:"); // send an etag so the client can now send conditional GET's
                  s->puts(s, c->etag);
                  s->puts(s, "\r\n");
                  s->puts(s, "\r\n");
                  s->write(s, response, sizeof(response));
                  return 0;
	    }
	    decrement_and_free(c); // dec and free with every cache_get
	}

	//else
	//  continue to fork

	childpid = fork();
	if (!childpid) { // I am the child
	    dup2(pipefd[1], 1); // duplicate std out to pipefd and close it
	    // set prctl here
	    execl(mtf->fullpath, "", NULL);
	    exit(0);
	} else { // I am the parent
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

	    char *http_field, *http_field_value;
	    http_field = palloc_array(s, char, strlen(line));
	    http_field_value = palloc_array(s, char, strlen(line));

	    int ret;
	    int err;
	    int bytes_avail;

	    struct http_get_response *cgi_response = s->get_response;

	    err = ioctl(fd, FIONREAD, &bytes_avail);
	    if (err < 0) {
		perror("Unable to determine number of bytes waiting in the cgi process pipe");
	    }

	    int count = 0; // omg this is quite ghetto..
	    int response_buf_offset = 0;
	    while (fgets(line, 80, f) != NULL) {
		if (count < 5) {
		    count++; // So we don't end up reading the entire response to find HTTP headers -_-

		    if (count == 1 && sscanf(line, "%s %s %s", prot, response_code, msg) == 3 ) {
			if (strcasecmp(prot, "HTTP/1.0") == 0 || strcasecmp(prot, "HTTP/1.1") == 0) {
			    //fprintf(stderr, "CGI response found HTTP response: [%s]\n", prot);
			} else {
			    count = 5; // Stop checking for HTTP headers if the first line isn't even a proper HTTP resonse.
			}
		    }

		    ret = sscanf(line, "%[^:]: %[^\n\r]", http_field, http_field_value);
		    if (ret == 2) {

//			fprintf(stderr, "http_field_value: [%s] \n", http_field_value);
//			fprintf(stderr, "http_field: [%s] \n", http_field);
			if (strcasecmp(http_field, "Expires") == 0) { // put expiration in cache, check with subsequent requests
			    cgi_response->expires = palloc_strdup(cgi_response, http_field_value);
//			    fprintf(stderr, "CGI response found Expires: [%s] \n", cgi_response->expires);

			}

			if (strcasecmp(http_field, "ETag") == 0) { // etag
			    cgi_response->etag = palloc_strdup(cgi_response, http_field_value);
//			    fprintf(stderr, "CGI response found ETag: [%s] \n", cgi_response->etag);
			}

		    }
		}
		s->puts(s, line);
		/* Put the line in the response string to cache */
		char *ptr = line;
		while (*(ptr++) != '\0') { // fgets terminates the line it read with a null byte
		    s->get_response->response_string[response_buf_offset] = *ptr;
		    response_buf_offset++;
		}

	    }

	    /* After we finish reading from the pipe */
	    // put buffer of cgi response lines in cache
	    // if cgi_response->etag and cgi_response->expires exists
	    // add them to the cache as well.

	    close(fd);
    	    close(pipefd[0]);

	    int cerr;
//	    fprintf(stderr, "cgi_response->expires: %s", cgi_response->expires);
//	    fprintf(stderr, "cgi_response->etag: %s", cgi_response->etag);

	    cerr = cache_add(s->get_req->request_string, s->get_response->response_string, cgi_response->expires, cgi_response->etag);
	    if (cerr < 0) {
		perror("Unable to add to cache");
	    }

	    return 0;

	}
    }



    off_t fsize;
    fsize = lseek(fd, 0, SEEK_END);
    char response_buf[(int)fsize];
    lseek(fd, 0, SEEK_SET); // rewind
    int response_buf_offset = 0;
    while ((readed = read(fd, buf, BUF_COUNT)) > 0)
    {
	ssize_t written;
	written = 0;

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
	/* Copy each char we read from the fd to the response_buf to cache */
	int i;
	for (i = 0 ; i < readed; i++) {
	    response_buf[response_buf_offset+i] = buf[i];
	}
	response_buf_offset += readed;


//    	fprintf(stderr,"written %d to session # %d\n", (int)written, s->fd);
    }
//   fprintf(stderr,"Printed out RESPONSE to session # %d \n", s->fd);
//   fprintf(stderr,"readed %d \n", readed);

    s->event.events = EPOLLIN | EPOLLET; // if we happen to have switched to EPOLLOUT in the meantime and we succeed this time, switch back to EPOLLIN
    if(epoll_ctl(s->server->efd, EPOLL_CTL_MOD, s->fd, &s->event) < 0) {
    	//fprintf(stderr, "Thread %ld: epoll_ctl failed to modify session fd %d to listen for only EPOLLOUT", tid, s->fd);
    }


    /* Finally, cache the response */
    s->get_response->response_string = response_buf;
    int cerr;
    cerr = cache_add(s->get_req->request_string, s->get_response->response_string, NULL, NULL); //No expiration for static files, and etags are dynamically generated and checked.
    if (cerr < 0) {
	perror("Unable to add to cache");
    }

    close(fd);
    return 0;
}
