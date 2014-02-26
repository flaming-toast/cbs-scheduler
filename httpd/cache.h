/* cs194-24 Lab 1 */

#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include "palloc.h"

#define CACHE_SIZE 1024

struct cache_entry {
    const char *request;
    const char *response;
    int reference_count;
};

int cache_init(palloc_env env);

int cache_add(const char *request, const char *response);

char* cache_get(const char *request);

int cache_remove(const char *request);

#endif
