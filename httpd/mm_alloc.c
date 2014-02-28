/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines. Remove this comment and provide
 * a summary of your allocator's design here.
 */

#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "mm_alloc.h"

/* Your final implementation should comment out this macro. */
//#define MM_USE_STUBS

void *mm_malloc(size_t size)
{
#ifdef MM_USE_STUBS
        return calloc(1, size);
#else
        return mm_malloc_ll(size);
#endif
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

//////////////////////// Linked List Malloc Implementation /////////////////////
//////////////////////// {A} GLOBALS ////////////////////////
#define FREE 1
#define USED 0
#define ERROR -1
#define SUCCESS 1

const size_t  INIT_MEM_SIZE     = 4090;
const int     NODE_HEADER_SIZE  = sizeof(MM_node);
const int     SPLIT_MINIMUM     = 2 * sizeof(MM_node);
const int     MMAP_PROT         = (PROT_READ | PROT_WRITE);
const int     MMAP_FLAGS        = (MAP_ANONYMOUS | MAP_PRIVATE);

MM_node         *malloc_head    = NULL;
MM_node         *malloc_tail    = NULL;
pthread_mutex_t lock            = PTHREAD_MUTEX_INITIALIZER;
//////////////////////// {A} GLOBALS ////////////////////////


//////////////////////// {B} CORE FUNCTIONS  ////////////////////////
void *mm_malloc_ll(size_t size)
{
        if(size == 0)
        {
                printf("SIZE REQUEST 0, RETURNING NULL\n");
                return NULL;
        }
        else
        {
                void *ptr;
                int status = SUCCESS;

                size = pad_mem_size(size);

                pthread_mutex_lock(&lock);
                if(malloc_head == NULL)
                {
                        status = initialize(size);
                }
                if(status == ERROR)
                {
                        pthread_mutex_unlock(&lock);
                        return NULL;
                }

                //print_free_blocks();
                ptr = req_free_mem(size);
                if(ptr == NULL)
                {
                        status = add_new_mem(size);
                        ptr = req_free_mem(size);
                }
                if(status == ERROR)
                {
                        printf("Failure to allocate.\n");
                        pthread_mutex_unlock(&lock);
                        return NULL;
                }

                zero_mem(ptr);
                pthread_mutex_unlock(&lock);
                return ptr;
        }
}

void *mm_realloc_ll(void *ptr, size_t size)
{
        //fprintf(stderr, "[Realloc] %lu\n", size);
        // <edge cases>
        if(ptr == NULL)
        {
                return mm_malloc(size);
        }
        else if(size == 0)
        {
                mm_free(ptr);
                return NULL;
        }
        // </edge cases>
        else if(get_header(ptr)->size > size)
        {
                return ptr;
        }
        else
        {
                void *new_ptr = mm_malloc(size);
                // failure to allocate new memory
                if(new_ptr == NULL)
                {
                        return ptr;
                }
                else
                {
                        //print_free_blocks();
                        memmove(new_ptr, ptr, get_header(ptr)->size);
                        mm_free(ptr);
                        //print_free_blocks();
                        return new_ptr;
                }
        }
}

void mm_free_ll(void *ptr)
{
        pthread_mutex_lock(&lock);

        MM_node *node = get_header(ptr);

        // Issue because next_free needs to be set to NULL when it is USED
        if(node->next_free != NULL)
                mm_malloc_had_a_problem();

        append_node(node);
        node->status = FREE;

        sanity_free_head();
        pthread_mutex_unlock(&lock);
}
//////////////////////// {B} CORE FUNCTIONS  ////////////////////////




//////////////////////// {C} CORE HELPERS ////////////////////////
/*
 * Initializes malloc_head / tail from first call to malloc.
 * Mmaps for initial memory. Returns ERROR if mmap fails.
 */
int initialize(size_t req_mem_size)
{
        void *addr;
        req_mem_size = get_mem_size(req_mem_size);

        if ((addr = mmap(NULL, req_mem_size, MMAP_PROT, MMAP_FLAGS, -1, 0)) == MAP_FAILED)
        {
                return ERROR;
        }
        else
        {
                malloc_head = construct_node(addr);
                // the actual first node, so malloc_head will always point to a 'free node'
                malloc_tail = construct_node(addr + NODE_HEADER_SIZE);
                malloc_head->next_free = malloc_tail;
                malloc_head->size = 0;
                malloc_tail->next_free = NULL;
                malloc_tail->size = req_mem_size - 2 * NODE_HEADER_SIZE;

                return SUCCESS;
        }
}

/*
 * Tries to grab a ptr to a free chunk of memory that exists in the current free list.
 * On failure, return NULL
 */
void *req_free_mem(size_t req_size)
{
        if(req_size < 1)
        {
                return NULL;
        }
        MM_node *prev_node = malloc_head;
        MM_node *cur_node = malloc_head->next_free;

        while(cur_node != NULL)
        {
                if(cur_node->status == USED)
                {
                        mm_malloc_had_a_problem();
                }

                // We have enough memory in this chunk to split it into two pieces
                if(cur_node->size > req_size + SPLIT_MINIMUM)
                {
                        MM_node *new_node = split_node(cur_node, req_size);
                        prev_node->next_free = new_node;
                        cur_node->next_free = NULL;

                        return (void *)((char *)cur_node + NODE_HEADER_SIZE);
                }
                // Just enough memory in this chunk to return it.
                else if(cur_node->size > req_size)
                {
                        cur_node->status = USED;
                        cur_node->next_free = NULL;
                        prev_node->next_free = cur_node->next_free;

                        return (void *)((char *)cur_node + NODE_HEADER_SIZE);
                }
                else
                {
                        prev_node = cur_node;
                        cur_node = cur_node->next_free;
                }
        }

        return NULL;
}

int add_new_mem(size_t size)
{
        void *addr;
        MM_node *new_node;

        size = get_mem_size(size) + NODE_HEADER_SIZE;

        if ((addr = mmap(NULL, size, MMAP_PROT, MMAP_FLAGS, -1, 0)) == MAP_FAILED)
        {
                return ERROR;
        }
        else
        {
                new_node = construct_node(addr);
                new_node->status = FREE;
                new_node->size = size;
                append_node(new_node);
                malloc_tail = new_node;

                return SUCCESS;
        }
}

/*
 * Splits a block of memory starting at the address of node,
 * inserting a new node header after the block of memory
 * about to be returned to the user
 * links new node to the original node
 * returns the address to the new node created after the split
 */
MM_node *split_node(MM_node *node, size_t req_size)
{
        if(req_size + NODE_HEADER_SIZE > node->size)
        {
                mm_malloc_had_a_problem();
        }

        size_t new_node_size = node->size - req_size - NODE_HEADER_SIZE;
        MM_node *new_node_addr = (MM_node *)((char *)node + NODE_HEADER_SIZE + req_size);
        MM_node *new_node = construct_node(new_node_addr);

        if(malloc_tail == node)
        {
                malloc_tail = new_node;
        }

        new_node->size = new_node_size;
        new_node->next_free = node->next_free;
        new_node->status = FREE;

        node->size = req_size;
        node->next_free = NULL;
        node->status = USED;


        return new_node;
}

/*
 * appends a node to beginning  of free list with specified addr and size
 * DOES NOT HANDLE LINKING TO PREV NODE
 */
void append_node(MM_node *new_node)
{
        // iterate to end of free list
        MM_node *prev = malloc_head;
        MM_node *cur = malloc_head->next_free;
        while(cur != NULL && (long unsigned)cur < (long unsigned)new_node)
        {
                prev = cur;
                cur = cur->next_free;
        }

        prev->next_free = new_node;
        new_node->next_free = cur;
}

//////////////////////// {C} CORE HELPERS ////////////////////////





//////////////////////// {D} UTILITIES ////////////////////////
/*
 * Constructor for MM_node
 */
MM_node *construct_node(void *addr)
{
        MM_node *node = addr;
        node->size = 0;
        node->next_free = NULL;
        node->status = FREE;
        return node;
}

/*
 * Returns the nearest multiple of INIT_MEM_SIZE larger than req_mem_size
 * we want the calls to sbrk to be a multiple of INIT_MEM_SIZE for uniformity
 */
size_t get_mem_size(size_t req_mem_size)
{
        return INIT_MEM_SIZE * (req_mem_size / INIT_MEM_SIZE + 1);
}

MM_node *get_header(void *ptr)
{
        return (MM_node *)((char *)ptr - NODE_HEADER_SIZE);
}

/*
 * returns the nearest multiple of NODE_HEADER_SIZE larger than provided size
 */
size_t pad_mem_size(size_t size)
{
        if(size % 8 == 0)
                return size;
        else
                return 8 * (size / 8 + 1);
}

void zero_mem(void *ptr)
{
        MM_node *node = get_header(ptr);
        memset((char *)node + NODE_HEADER_SIZE, 0, node->size);

}
//////////////////////// {D} UTILITIES ////////////////////////





//////////////////////// {E} DEBUG TOOLS ////////////////////////
/*
 * Debug tool to print out entire linked list, also stating whether nodes are free or not.
 */
void print_free_blocks(void)
{
        MM_node *cur_node = malloc_head;
        int num = 0;
        while(cur_node != NULL)
        {
                cur_node = (MM_node *)((char *)cur_node + NODE_HEADER_SIZE + cur_node->size);
                num += 1;

                if(cur_node->status == FREE)
                {
                        printf("Node at [%d]:%p - FREE with %lu bytes free.\n", num, cur_node, cur_node->size);
                }
                else
                {
                        printf("Node at [%d]:%p - USED with %lu bytes used.\n", num, cur_node, cur_node->size);
                }
                if(cur_node == malloc_tail)
                        break;
        }
}
void sanity_free_head(void)
{
        if(malloc_head->next_free != NULL && malloc_head->next_free->status != FREE)
        {
                //printf("DANGER THIS SHOULD NEVER BE PRINTED\n");
                mm_malloc_had_a_problem();
        }
}
void mm_malloc_had_a_problem(void)
{
        MM_node *segfault = NULL;
        segfault->size = 9999;
}
//////////////////////// {E} DEBUG TOOLS ////////////////////////
