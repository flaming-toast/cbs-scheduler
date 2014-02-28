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

typedef unsigned char* bitmap_t;

void set_bitmap(bitmap_t b, int i) {
    b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
    b[i / 8] &= ~(1 << (i & 7));
}

int get_bitmap(bitmap_t b, int i) {
    return b[i / 8] & (i & 7) ? 1 : 0;
}

bitmap_t create_bitmap(int n) {
    return mm_malloc((n + 7) / 8);
}
//////////////////////// Preset Block Size Optimized Malloc /////////////////////
//////////////////////////////////////////////////////////////////////////////////

/*
 * notes
 * max size for single block is smaller since splitting requires a minimum size from header
 * we may need malloc_head to always point to something after first malloc
 * edge cases - calling malloc(0)
 */

//////////////////////// {A} GLOBALS ////////////////////////
#define FREE 1
#define USED 0
#define ERROR -1
#define SUCCESS 1

const size_t  INIT_MEM_SIZE     = 2048;
const int     NODE_HEADER_SIZE  = sizeof(MM_node);
const int     BLOCK_HEADER_SIZE  = sizeof(MM_block);
const int     SPLIT_MINIMUM     = 2 * sizeof(MM_node);

int prot = (PROT_READ | PROT_WRITE);
int flags = (MAP_ANONYMOUS | MAP_PRIVATE);
MM_node       *malloc_head      = NULL;
MM_node       *malloc_tail      = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//////////////////////// {A} GLOBALS ////////////////////////


//////////////////////// {B} CORE FUNCTIONS  ////////////////////////
void *mm_malloc_ll(size_t size)
{
        //perror("[ALLOCATE] %lu\n", size);
        if(size == 0)
        {
                printf("SIZE REQUEST 0\n");
                return NULL;
        }
        else
        {
                //printf("requested size: %lu, new size: %lu\n", size, pad_mem_size(size));
                int status = SUCCESS;
                void *ptr;

                size = pad_mem_size(size);
                //fprintf(stderr, "[Malloc] %lu\n", size);

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
                        return NULL;
                        pthread_mutex_unlock(&lock);
                }

                sanity_free_head();
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
                        new_ptr = memcpy(new_ptr, ptr, get_header(ptr)->size);
                        mm_free(ptr);
                        return new_ptr;
                }
        }
}

void mm_free_ll(void *ptr)
{
        ////printf("head pointed to %p\n", malloc_head->next_free);
        ////printf("call to free on %p\n", get_header(ptr));
        pthread_mutex_lock(&lock);

        MM_node *node = get_header(ptr);
        //fprintf(stderr, "[Free] %lu\n", node->size);

        // Issue because next_free needs to be set to NULL when it is USED
        if(node->next_free != NULL)
                mm_malloc_had_a_problem();

        append_node(node);
        node->status = FREE;

        sanity_free_head();
        pthread_mutex_unlock(&lock);
        return;
}
//////////////////////// {B} CORE FUNCTIONS  ////////////////////////




//////////////////////// {C} CORE HELPERS ////////////////////////
/*
 * Initializes malloc_head / tail from first call to malloc.
 * Sbrks for initial memory. Returns ERROR if sbrk fails.
 */
int initialize(size_t req_mem_size)
{
        void *addr;
        //printf("First call to mm_alloc. Node header size: %d\n", NODE_HEADER_SIZE);

        req_mem_size = get_mem_size(req_mem_size);

        //printf("allocating %lu bytes of memory...\n", req_mem_size);
        if ((addr = mmap(NULL, req_mem_size, prot, flags, -1, 0)) == MAP_FAILED)
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
                //printf("heap bottom: %p, %p,  %li\n", heap_bottom, malloc_head, (size_t)heap_bottom % 8);

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
                        //printf("DANGER THIS SHOULD NEVER BE PRINTED\n");
                        mm_malloc_had_a_problem();
                }
                // We have enough memory in this chunk to split it into two pieces

                if(cur_node->size > req_size + SPLIT_MINIMUM)
                {
                        MM_node *new_node = split_node(cur_node, req_size);
                        prev_node->next_free = new_node;
                        //printf("Split: old node with size %lu, split node with size %lu\n", cur_node->size, new_node->size);

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
        ////printf("allocating %lu bytes of memory...\n", size);
        //printf("%p, %p", sbrk(0), (char *)malloc_tail + malloc_tail->size + NODE_HEADER_SIZE);

        if ((addr = mmap(NULL, size, prot, flags, -1, 0)) == MAP_FAILED)
        {
                // unable to sbrk successfully
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
                //printf("DANGER THIS SHOULD NEVER BE PRINTED\n");
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

MM_node *get_next(MM_node *node)
{
        return (MM_node *)((char *)node + NODE_HEADER_SIZE + node->size);
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
                        //printf("Node at [%d]:%p - FREE with %lu bytes free.\n", num, cur_node, cur_node->size);
                }
                else
                {
                        //printf("Node at [%d]:%p - USED with %lu bytes used.\n", num, cur_node, cur_node->size);
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
