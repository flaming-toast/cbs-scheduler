/* cs194-24 Lab 1 */

#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include "palloc.h"

#define CACHE_SIZE 1024

struct cache_entry {
    const char *request;
    const char *response;
    const char *expires;
    const char *etag;
    int reference_count;
};

void cache_init(palloc_env env);

int cache_add(const char *request, const char *response, const char* expires, const char* etag);

struct cache_entry* cache_get(const char *request);

int cache_remove(const char *request);

static void decrement_and_free(struct cache_entry *entry);

#endif
