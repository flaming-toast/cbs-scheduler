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

static struct cache_entry *swap_cache_entry(int cache_index, struct cache_entry *new_entry);

void decrement_and_free(struct cache_entry *cur_entry);

void cache_init(palloc_env env)
{
        int i;
        for (i = 0; i < CACHE_SIZE; i++)
        {
                lock_array[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        }
        cache_env = env;
}

int cache_add(const char *request, const char *response, const char *expires, const char* etag)
{
        int cache_index = hash(request) % CACHE_SIZE;
        struct cache_entry *new_entry, *old_entry;

        new_entry = palloc(cache_env, struct cache_entry);
        new_entry->request = palloc_strdup(new_entry, request);
        new_entry->response = palloc_strdup(new_entry, response);
        new_entry->expires = palloc_strdup(new_entry, expires);
        new_entry->etag = palloc_strdup(new_entry, etag);
        new_entry->reference_count = 1;   // Count cache as one of them.
        //new_entry->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

        pthread_mutex_lock(&lock_array[cache_index]);
        old_entry = swap_cache_entry(cache_index, new_entry);
        pthread_mutex_unlock(&lock_array[cache_index]);

        decrement_and_free(old_entry);

        return 0;
}

struct cache_entry* cache_get(const char *request)
{
        int cache_index = hash(request) % CACHE_SIZE;
        struct cache_entry *cur_entry;

        pthread_mutex_lock(&lock_array[cache_index]);
        cur_entry = cache_array[cache_index];
        if (cur_entry == NULL)
        {
                pthread_mutex_unlock(&lock_array[cache_index]);
                return NULL;
        }
        __sync_add_and_fetch(&(cur_entry->reference_count), 1); // The cache lock will not necessarily ensure atomicity on this count since the cache is not the only thing which might change it.
        pthread_mutex_unlock(&lock_array[cache_index]);
        //At this point, it shouldn't be deleted b/c we've already incremented the reference_count
        if (request == NULL || cur_entry->request == NULL) {
                if (request == cur_entry->request) {
                        return cur_entry;
                } else {
                        if (__sync_sub_and_fetch(&(cur_entry->reference_count), 1) == 0) {
                                pfree(cur_entry);
                        }
                }
        }
        if (strcmp(cur_entry->request, request) != 0)
        {
                if (__sync_sub_and_fetch(&(cur_entry->reference_count), 1) == 0) {
                        pfree(cur_entry); //Everyone else let go in the meantime
                }
                return NULL;                // Entry not exist, even if share hash
        }

        if (&(cur_entry->reference_count) <= 0)
        {
                printf("FATAL ERROR IN cache_get: cur_entry->reference_count <= 0\n");
                exit(1);
        }
        //At this point, it shouldn't be deleted b/c reference_count is non-zero

        return cur_entry;
}

int cache_remove(const char *request)
{
        int cache_index = hash(request) % CACHE_SIZE;
        struct cache_entry *old_entry = NULL;
        int result = 0;

        pthread_mutex_lock(&lock_array[cache_index]);
        old_entry = cache_array[cache_index];
        if (old_entry == NULL)
        {
                result = -1;
        }
        else if (request == NULL || old_entry->request == NULL) {
                if (request == old_entry->request) {
                        old_entry = swap_cache_entry(cache_index, NULL);
                        decrement_and_free(old_entry);
                } else {
                        result = -1;
                }
        }
        else if (strcmp(old_entry->request, request) != 0)
        {
                result = -1;
        }
        else
        {
                old_entry = swap_cache_entry(cache_index, NULL);
                decrement_and_free(old_entry);
        }

        pthread_mutex_unlock(&lock_array[cache_index]);

        return result;
}




int hash(const char *str)
{
        int hash = 5381;
        int c;
        if (str == NULL) {
                return 0;
        }
        while ((c = *str++))
        {
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
        if (hash < 0)
        {
                hash *= -1;
        }

        return hash;
}

/* Assuming that lock is already obtained-sort of handwavy, may not need it. */
static struct cache_entry *swap_cache_entry(int cache_index, struct cache_entry *new_entry)
{
        struct cache_entry *old_entry;

        old_entry = __sync_val_compare_and_swap(&(cache_array[cache_index]), cache_array[cache_index], new_entry);
        /*old_entry = cache_array[cache_index];
          cache_array[cache_index] = new_entry; */

        return old_entry;
}

/*Does not require cache lock to be held since this should be called on entries
 * either 1) after they have been removed from the cache or 2) by entities other
 * than the cache. Since the ref_count can only become zero after removal from
 * the cache, no lock is needed to bind atomic_dec_and_test() and pfree() into
 * a critical section. */
void decrement_and_free(struct cache_entry *cur_entry)
{
        if (cur_entry == NULL)
        {
                return;
        }
        if (__sync_sub_and_fetch(&(cur_entry->reference_count), 1) == 0)
        {
                pfree(cur_entry);
        }
}









