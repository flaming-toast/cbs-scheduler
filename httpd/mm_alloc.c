/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines. Remove this comment and provide
 * a summary of your allocator's design here.
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/* Your final implementation should comment out this macro. */
#define MM_USE_STUBS

void *mm_malloc(size_t size)
{
//        return calloc(1, size);
        return mm_malloc_ll(size);
}

void *mm_realloc(void *ptr, size_t size)
{
#ifdef MM_USE_STUBS
        return realloc(ptr, size);
#else
        return mm_realloc_ll(ptr, size);
#endif
}

void mm_free(void *ptr)
{
#ifdef MM_USE_STUBS
        free(ptr);
#else
        mm_free_ll(ptr);
#endif
}
//////////////////////////////////////////////////////////////////////////////////
//////////////////////// STANDARD LINKED LIST IMPLEMENTATION /////////////////////
//////////////////////////////////////////////////////////////////////////////////
#define FREE 1
#define USED 0
#define TRUE 1
#define FALSE 0
#define ERROR -1
#define SUCCESS 1

const size_t  INIT_MEM_SIZE     = 8192;
const int     NODE_HEADER_SIZE  = 32;

MM_node *mm_head = NULL;

MM_node *construct_node(void *addr, MM_node *prev)
{
        MM_node *node = addr;
        node->free_size = 0;
        node->next_free = NULL;
        node->prev = prev;
        node->status = FREE;
        return node;
}

int initialize(size_t req_mem_size)
{
        void *heap_bottom;

        req_mem_size = get_mem_size(req_mem_size);

        if((heap_bottom = sbrk(req_mem_size)) == NULL)
        {
                return ERROR;
        } else {
                heap_bottom = align_addr(heap_bottom);
                mm_head = construct_node(heap_bottom - req_mem_size, NULL);
                printf("heap bottom: %p, %p,  %li\n", heap_bottom, mm_head, (long)heap_bottom % 8);
                printf("top mm_head: %d, %p, %p, %d\n", mm_head->free_size, mm_head->next_free, mm_head->prev, mm_head->status);

                return SUCCESS;
        }
}
/*
 * returns the closest address that is a multiple of 8 to the provided address.
 */
void *align_addr(void *addr)
{
        if((long)addr % 8 != 0)
        {
                // TODO
                return addr;
        } else {
                return addr;
        }
}

int get_mem_size(size_t req_mem_size)
{
        // we want the inital sbrk to be a multiple of 8K for uniformity
        return INIT_MEM_SIZE * (req_mem_size / INIT_MEM_SIZE + 1);
}

void *mm_malloc_ll(size_t size)
{
        int status;

        if(mm_head == NULL)
        {
                status = initialize(size);
        }
        if(status == ERROR)
                return NULL;

        return calloc(1, size);
}

void *mm_realloc_ll(void *ptr, size_t size)
{
        return realloc(ptr, size);
}

void mm_free_ll(void *ptr)
{
        free(ptr);
}

