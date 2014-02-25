/* cs194-24 Lab 1 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "palloc.h"
#include "mm_alloc.h"

struct child_list
{
        struct child_list *next;
        struct block *blk;
};

struct block
{
        struct block *parent;
        struct child_list *children;
        const char *pool_name;
        const char *type;
        int (*destructor)(void *ptr);
        pthread_mutex_t mutex;
};



static inline struct block *ENV_BLK(const palloc_env env)
{
        if (env == NULL) {
                return NULL;
        }
        return (struct block *)(((char *)env) - sizeof(struct block));
}

static inline struct block *PTR_BLK(const void *ptr)
{
        if (ptr == NULL) {
                return NULL;
        }
        return (struct block *)(((char *)ptr) - sizeof(struct block));
}

static inline palloc_env BLK_ENV(const struct block *blk)
{
        if (blk == NULL) {
                return NULL;
        }
        return (palloc_env)(((char *)blk) + sizeof(struct block));
}

static inline void *BLK_PTR(const struct block *blk)
{
        if (blk == NULL) {
                return NULL;
        }
        return (void *)(((char *)blk) + sizeof(struct block));
}


static struct block *block_new(int size);


static int _pfree(const void *ptr, bool external);


static void _palloc_print_tree(struct block *blk, int level);


palloc_env palloc_init(const char *format)
{
        struct block *blk;
        char *pool_name = (char *)mm_malloc(strlen(format) + 1);
        pthread_mutex_t mutex;

        blk = block_new(0);
        if (blk == NULL) {
                return NULL;
        }
        strcpy(pool_name, format);


        blk->pool_name = pool_name;
        pthread_mutex_init(&mutex, NULL);
        blk->mutex = mutex;

        return BLK_ENV(blk);
}


void *prealloc(const void *ptr, size_t size)
{
        struct block *oblk, *nblk;
        if (size != 0) {
                oblk = PTR_BLK(ptr);
                if (oblk != NULL) {
                        pthread_mutex_lock(&(oblk->mutex));
                }
                nblk = mm_realloc(oblk, size + sizeof(*oblk));
        } else if (ptr == NULL) {
                return NULL;
        } else {
                nblk = NULL;
        }
        if (nblk == NULL) {
                _pfree(oblk, true);
                return NULL;
        }


        if (nblk != oblk) {
                struct child_list *cur;

                if (nblk->parent != NULL) {
                        pthread_mutex_lock(&(nblk->parent->mutex));
                        cur = nblk->parent->children;
                        while (cur != NULL)	{
                                if (cur->blk == oblk) {
                                        cur->blk = nblk;
                                }
                                cur = cur->next;
                        }
                        pthread_mutex_unlock(&(nblk->parent->mutex));
                }

                cur = nblk->children;
                while (cur != NULL) {
                        pthread_mutex_lock(&(cur->blk->mutex));
                        if (cur->blk->parent == oblk) {
                                cur->blk->parent = nblk;
                        }
                        pthread_mutex_unlock(&(cur->blk->mutex));
                        cur = cur->next;
                }
        }

        pthread_mutex_unlock(&(nblk->mutex));
        return BLK_PTR(nblk);
}


void *_palloc(palloc_env env, size_t size, const char *type)
{
        struct block *pblk;
        struct block *cblk;
        struct child_list *clist;
        pthread_mutex_t mutex;

        pblk = ENV_BLK(env);
        pthread_mutex_lock(&(pblk->mutex));

        cblk = block_new(size);
        if (cblk == NULL) {
                pthread_mutex_unlock(&(pblk->mutex));
                return NULL;
        }

        clist = mm_malloc(sizeof(*clist));
        if (clist == NULL) {
                mm_free(cblk);
                pthread_mutex_unlock(&(pblk->mutex));
                return NULL;
        }


        cblk->parent = pblk;
        cblk->type = type;
        clist->blk = cblk;

        pthread_mutex_init(&mutex, NULL);
        cblk->mutex = mutex;
        clist->next = pblk->children;
        pblk->children = clist;

        pthread_mutex_unlock(&(pblk->mutex));
        return BLK_ENV(cblk);
}


int pfree(const void *ptr)
{
        int res;
        struct block *lb = PTR_BLK(ptr);
        if (lb != NULL) {
                pthread_mutex_lock(&(lb->mutex));
                res = _pfree(ptr, true);
                return res;
        }
        return -1;

}

void * _palloc_cast(const void *ptr, const char *type)
{
        struct block *blk;

        blk = PTR_BLK(ptr);
        if (blk != NULL) {
                pthread_mutex_lock(&(blk->mutex));
        } else {
                return NULL;
        }
        if (blk->type != type && (strcmp(blk->type, type) != 0)) {
                pthread_mutex_unlock(&(blk->mutex));
                return NULL;
        }
        pthread_mutex_unlock(&(blk->mutex));
        return (void *)ptr;
}


char *palloc_strdup(palloc_env env, const char *str)
{
        char *out;
        struct block *lblk = ENV_BLK(env);
	if (str == NULL) {
	  return NULL;
	}
        out = palloc_array(env, char, strlen(str) + 2);
        if (out != NULL) {
                pthread_mutex_lock(&(lblk->mutex));
                strcpy(out, str);
                pthread_mutex_unlock(&(lblk->mutex));
        }
        return out;
}


void _palloc_destructor(const void *ptr, int (*dest)(void *))
{
        struct block *blk;
        blk = PTR_BLK(ptr);
        if (blk != NULL) {
                pthread_mutex_lock(&(blk->mutex));
                blk->destructor = dest;
                pthread_mutex_unlock(&(blk->mutex));
        } else {
                abort();
        }
        return;
}



void palloc_print_tree(const void *ptr)
{
        struct block *lblk = PTR_BLK(ptr);
        pthread_mutex_lock(&(lblk->mutex));
        _palloc_print_tree(lblk, 0);
        pthread_mutex_unlock(&(lblk->mutex));
        return;
}

struct block *block_new(int size)
{
        struct block *b;

        b = mm_malloc(sizeof(*b) + size);
        if (b == NULL) {

                return NULL;
        }

        b->parent = NULL;
        b->children = NULL;
        b->pool_name = NULL;
        b->type = NULL;
        b->destructor = NULL;


        return b;
}


int _pfree(const void *ptr, bool external)
{
        struct block *cblk;
        struct child_list *cur, *prev;
        int ret;

        if(ptr == NULL) {
                return -1;
        }

        cblk = PTR_BLK(ptr);

        ret = 0;
        cur = cblk->children;
        while (cur != NULL) {
                struct child_list *next;
                pthread_mutex_lock(&(cur->blk->mutex));
                ret |= _pfree(BLK_ENV(cur->blk), false);
                if (ret == -1) {
                        pthread_mutex_unlock(&(cblk->mutex));
                        return -1; //See Piazza 104
                }
                next = cur->next;
                cur->next = NULL;
                mm_free(cur);
                cur = next;
        }

        if (cblk->pool_name != NULL) {
                mm_free((char *)cblk->pool_name);
        }


        if (cblk->parent != NULL && external) {
                prev = NULL;
                pthread_mutex_lock(&(cblk->parent->mutex));
                cur = cblk->parent->children;

                while (cur != NULL && cur->blk != cblk) {
                        pthread_mutex_lock(&(cur->blk->mutex));
                        prev = cur;
                        cur = cur->next;
                        pthread_mutex_unlock(&(prev->blk->mutex));
                }

                if (cur == NULL) {
                        fprintf(stderr, "Inconsistant palloc tree during pfree()\n");
                        abort();
                } else if (prev == NULL) {
                        cblk->parent->children = cur->next;
                        mm_free(cur);
                } else {
                        prev->next = cur->next;
                        mm_free(cur);
                }
                pthread_mutex_unlock(&(cblk->parent->mutex));
        }

        if (cblk->destructor != NULL) {
                cblk->destructor(BLK_PTR(cblk));
        }
        pthread_mutex_unlock(&(cblk->mutex));
        pthread_mutex_destroy(&(cblk->mutex));
        mm_free(cblk);

        return ret;
}


void _palloc_print_tree(struct block *blk, int level)
{
        struct child_list *cur;
        const char *string;

        string = (blk->pool_name == NULL) ? blk->type : blk->pool_name;
        printf("%*s%s\n", level, "", string);

        for (cur = blk->children; cur != NULL; cur = cur->next) {
                _palloc_print_tree(cur->blk, level+1);
        }

}
