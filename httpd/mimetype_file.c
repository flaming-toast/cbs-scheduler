/* cs194-24 Lab 1 */

#include "mimetype_file.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <sys/syscall.h>

#define BUF_COUNT 4096

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
                fprintf(stderr, "st_mtime from stat: %lx", (long)filestat.st_mtime);
                snprintf(time, sizeof time, "%lx", (long)filestat.st_mtime);
                fprintf(stderr, "snprintf:%s", time);

                if (etag != NULL) { // We had sent an ETag to this client before

                        if (strcasecmp(time, etag) == 0) { // etags do not match
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
                childpid = fork();
                if (!childpid) {
                        dup2(pipefd[1], 1); // duplicate std out to pipefd and close it
                        execl(mtf->fullpath, "", NULL);
                        exit(0);
                } else {
                        dup2(pipefd[0], 0); // duplicate std in to pipefd and close it
                        fd = pipefd[0]; //read from child
                }
        }

        //long tid = syscall(SYS_gettid);
        //fprintf(stderr,"Thread %ld: Printed out HTTP 200 OK\n", tid);

        //		close(pipefd[0]);
        close(pipefd[1]); //stop communcation, signal end of data

        /* see http://stackoverflow.com/questions/8057892/epoll-on-regular-files */
        /* It is considered correct to block on disk I/O requests */

        // set to listen to epollout events only.
        while ((readed = read(fd, buf, BUF_COUNT)) > 0)
        {
                ssize_t written;
                written = 0;
                fprintf(stderr, "reading file now\n");
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

        s->event.events = EPOLLIN | EPOLLET; // if we happen to have switched to EPOLLOUT in the meantime and we succeed this time, switch back to EPOLLIN
        if(epoll_ctl(s->server->efd, EPOLL_CTL_MOD, s->fd, &s->event) < 0) {
                //fprintf(stderr, "Thread %ld: epoll_ctl failed to modify session fd %d to listen for only EPOLLOUT", tid, s->fd);
        }

        close(fd);
        close(pipefd[0]);
        //perror("close() requested file");

        return 0;
}
