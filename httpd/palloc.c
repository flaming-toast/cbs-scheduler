/* cs194-24 Lab 1 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "palloc.h"
#include "mm_alloc.h"

/** The list of children. */
struct child_list
{
    struct child_list *next;
    struct block *blk;
};

/** Stores meta-data about the memory. */
struct block
{
    struct block *parent;
    struct child_list *children;
    const char *pool_name;
    const char *type;
    int (*destructor)(void *ptr);
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/** Get the block object from an environment (which points to start of data). */
static inline struct block *ENV_BLK(const palloc_env env)
{
	if (env == NULL) {
		return NULL;
	}
    return (struct block *)(((char *)env) - sizeof(struct block));
}

/**
 * Get the block object from a pointer to the start of the data.
 */
static inline struct block *PTR_BLK(const void *ptr)
{
	if (ptr == NULL) {
		return NULL;
	}
    return (struct block *)(((char *)ptr) - sizeof(struct block));
}

/** Get the environment object which points to the start of the data from a block. */
static inline palloc_env BLK_ENV(const struct block *blk)
{
	if (blk == NULL) {
		return NULL;
	}
    return (palloc_env)(((char *)blk) + sizeof(struct block));
}

/** Get the pointer to the start of the data from a block. */
static inline void *BLK_PTR(const struct block *blk)
{
	if (blk == NULL) {
		return NULL;
	}
    return (void *)(((char *)blk) + sizeof(struct block));
}

/** Returns a new block with given SIZE. */
static struct block *block_new(int size);

/** Implementation of free. */
static int _pfree(const void *ptr, bool external);

/** Implementation of print tree. */
static void _palloc_print_tree(struct block *blk, int level);

/** Initialize a new top level palloc_env. No locks necessary. */
palloc_env palloc_init(const char *format, ...)
{
    va_list args;
    struct block *blk;
    char *pool_name;

    blk = block_new(0);
    if (blk == NULL) {
    	return NULL;
    }

    va_start(args, format);
    vasprintf(&pool_name, format, args);
    va_end(args);

    blk->pool_name = pool_name;

    return BLK_ENV(blk);
}

/* Some changes are to ensure compatibility with man 3 malloc. */
void *prealloc(const void *ptr, size_t size)
{
    struct block *oblk, *nblk;
    pthread_mutex_lock(&lock);
    if (size != 0) { 
    	oblk = PTR_BLK(ptr);
    	nblk = mm_realloc(oblk, size + sizeof(*oblk));
    } else if (ptr == NULL) {
    	pthread_mutex_unlock(&lock);
    	return NULL;
    } else { //ptr != NULL, size == 0
    	nblk = NULL;
    }
    if (nblk == NULL) {
		_pfree(oblk, true);
		pthread_mutex_unlock(&lock);
		return NULL;
    }


    if (nblk != oblk) {
		struct child_list *cur;
	    /** Updates parents. */
		cur = nblk->parent->children;
		while (cur != NULL)	{
			if (cur->blk == oblk) {
				cur->blk = nblk;
			}
			cur = cur->next;
		}
		/* Updates children. */
		cur = nblk->children;
		while (cur != NULL) {
			if (cur->blk->parent == oblk) {
				cur->blk->parent = nblk;
			}
			cur = cur->next;
		}
    }

	pthread_mutex_unlock(&lock);
    return BLK_PTR(nblk);
}

/** Needs locks since called by macro defines, also since modifies env. */
void *_palloc(palloc_env env, size_t size, const char *type)
{
    struct block *pblk;
    struct block *cblk;
    struct child_list *clist;

    pthread_mutex_lock(&lock);
    pblk = ENV_BLK(env);
    
    cblk = block_new(size);
    if (cblk == NULL) {
    	pthread_mutex_unlock(&lock);
    	return NULL;
    }

    //TODO: Ask about sizeof if this is correct.
    clist = mm_malloc(sizeof(*clist));
    if (clist == NULL) {
		mm_free(cblk);
		pthread_mutex_unlock(&lock);
		return NULL;
    }

    /** What it does is inset self into front of child list. */
    cblk->parent = pblk;
    cblk->type = type;
    clist->blk = cblk;
    clist->next = pblk->children;
    pblk->children = clist;

	pthread_mutex_unlock(&lock);
    return BLK_ENV(cblk);
}

/** Public API for pfree, includes lock here. */
int pfree(const void *ptr)
{
	int res;
    pthread_mutex_lock(&lock);
    res = _pfree(ptr, true);
	pthread_mutex_unlock(&lock);
	return res;
}

void * _palloc_cast(const void *ptr, const char *type)
{
    struct block *blk;

    blk = PTR_BLK(ptr);

    if (blk->type != type && (strcmp(blk->type, type) != 0)) {
    	return NULL;
    }

    return (void *)ptr;
}

/** No lock required (?) since palloc_array acquires locks. */
char *palloc_strdup(palloc_env env, const char *str)
{
    char *out;

    out = palloc_array(env, char, strlen(str) + 2);
    if (out != NULL) {
		pthread_mutex_lock(&lock);
		strcpy(out, str);
		pthread_mutex_unlock(&lock);
    }
    return out;
}

/** 
 * Requires a lock due to macro call. 
 * TODO: Not sure if this should return an int instead.
 */
void _palloc_destructor(const void *ptr, int (*dest)(void *))
{
    struct block *blk;
    pthread_mutex_lock(&lock);
    blk = PTR_BLK(ptr);
    blk->destructor = dest;
	pthread_mutex_unlock(&lock);
    //Probably don't return something, since would require modifying method signature.
	return;
}


/** 
 * Lock in case we are traversing and data gets modified, because we want
 * to avoid having a segfault.
 */
void palloc_print_tree(const void *ptr)
{
    pthread_mutex_lock(&lock);
    _palloc_print_tree(PTR_BLK(ptr), 0);
	pthread_mutex_unlock(&lock);
    return;
}

struct block *block_new(int size)
{
    struct block *b;

    b = mm_malloc(sizeof(*b) + size);
    if (b == NULL) {
    	//Should never enter here ever
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

		ret |= _pfree(BLK_ENV(cur->blk), false);
		if (ret == -1) {
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
    //TODO: Why do we not free type, does the macro not alloc this?

    /** Only should run for top level. */
    if (cblk->parent != NULL && external) {
		prev = NULL;
		cur = cblk->parent->children;
		/** 
		 * So this while loop ends up with the child_list instance with blk->cblk in cur
		 * and it's parent in prev.
		 */
		while (cur != NULL && cur->blk != cblk) {
			prev = cur;
			cur = cur->next;
		}

		if (cur == NULL) {
			fprintf(stderr, "Inconsistant palloc tree during pfree()\n");
			abort(); //TODO Is this supposed to abort? Why not return -1?
		} else if (prev == NULL) {
			cblk->parent->children = cur->next;
			mm_free(cur);
		} else {
			prev->next = cur->next;
			mm_free(cur);
		}
    }

    if (cblk->destructor != NULL) {
    	cblk->destructor(BLK_PTR(cblk));
    }

    mm_free(cblk);

    return ret;
}

/** 
 * No lock here due to the fact that we have recursive calls.
 */
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
