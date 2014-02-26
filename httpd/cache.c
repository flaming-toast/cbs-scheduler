/* cs194-24 Lab 1 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "cache.h"
#include "palloc.h"

static palloc_env cache_env = NULL;
static struct cache_entry *cache_array[CACHE_SIZE];
static pthread_mutex_t lock_array[CACHE_SIZE];

static int hash(const char *str);

static int replace_and_free_old(int cache_index, struct cache_entry *new_entry);

int cache_init(palloc_env env)
{
        int i;
        for (i = 0; i < CACHE_SIZE; i++)
        {
                lock_array[i] = PTHREAD_MUTEX_INITIALIZER;
        }
        cache_env = env;
}

int cache_add(const char *request, const char *response)
{
        int cache_index = hash(request) % CACHE_SIZE;
        struct cache_entry *new_entry, *old_entry;

        new_entry = palloc(cache_env, struct cache_entry);
        new_entry->request = palloc_strdup(new_entry, request);
        new_entry->response = palloc_strdup(new_entry, response);
        new_entry->reference_count = 1;    // Count cache as one of them.
        new_entry->lock = PTHREAD_MUTEX_INITIALIZER;

        pthread_mutex_lock(&lock_array[cache_index]);
        old_entry = swap_cache_entry(cache_index, new_entry);
        pthread_mutex_unlock(&lock_array[cache_index]);

        pthread_mutex_lock(&lock_array[cache_index]);
        decrement_and_free(old_entry);
        pthread_mutex_unlock(&lock_array[cache_index]);

        return 0;
}

char* cache_get(const char *request)
{
        int cache_index = hash(request) % CACHE_SIZE;
        struct cache_entry *cur_entry;
        char *response;

        pthread_mutex_lock(&lock_array[cache_index]);
        cur_entry = cache_array[cache_index];
        if (cur_entry == NULL)
        {
                pthread_mutex_unlock(&lock_array[cache_index]);
                return NULL;
        }
        if (strcmp(cur_entry->request, request) != 0)
        {
                pthread_mutex_unlock(&lock_array[cache_index]);
                return NULL;                // Entry not exist, even if share hash
        }
        cur_entry->reference_count++;   // Atomic increment
        if (cur_entry->reference_count <= 0) {
                printf("FATAL ERROR IN cache_get: cur_entry->reference_count <= 0\n");
                exit(1);
        }
        pthread_mutex_unlock(&lock_array[cache_index]);
        //At this point, it shouldn't be deleted b/c reference_count is non-zero

        response = palloc_strdup(cache_env, cur_entry->response);

        pthread_mutex_lock(&lock_array[cache_index]);
        decrement_and_free(old_entry);
        pthread_mutex_unlock(&lock_array[cache_index]);

        return response;
}

int cache_remove(const char *request)
{
        int cache_index = hash(request) % CACHE_SIZE;
        struct cache_entry *old_entry = NULL;
        int result = 0;

        pthread_mutex_lock(&lock_array[cache_index]);
        old_entry = cache_array[cache_index];
        if (old_entry == NULL)
                result = -1;
        else if (strcmp(old_entry->request, request) != 0)
                result = -1;
        else
                old_entry = swap_cache_entry(cache_index, NULL);
        pthread_mutex_unlock(&lock_array[cache_index]);

        pthread_mutex_lock(&lock_array[cache_index]);
        decrement_and_free(old_entry);
        pthread_mutex_unlock(&lock_array[cache_index]);

        return result;
}




int hash(const char *str);
{
        int hash = 5381;
        int c;

        while (c = *str++)
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        if (hash < 0)
                hash *= -1;

        return hash;
}

/* Assuming that lock is already obtained. */
static struct cache_entry *swap_cache_entry(int cache_index, struct cache_entry *new_entry)
{
        struct cache_entry *old_entry;

        old_entry = cache_array[cache_index];
        cache_array[cache_index] = new_entry;

        return old_entry;
}

/* Assuming that lock is already obtained. */
static void decrement_and_free(struct cache_entry *cur_entry)
{
        if (cur_entry == NULL)
                return;

        cur_entry->reference_count--;
        if (cur_entry->reference_count < 0) {
                printf("FATAL ERROR IN cache_get: cur_entry->reference_count < 0\n");
                exit(1);
        }

        if (cur_entry->reference_count == 0)
                pfree(cur_entry);
}









