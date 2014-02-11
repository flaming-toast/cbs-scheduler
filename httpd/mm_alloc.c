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
//////////////////////////////////////////////////////////////////////////////////
//////////////////////// STANDARD LINKED LIST IMPLEMENTATION /////////////////////
//////////////////////////////////////////////////////////////////////////////////

/*
 * notes
 * max size for single block is smaller since splitting requires a minimum size from header
 * we may need malloc_head to always point to something after first malloc
 * edge cases - calling malloc(0)
 */

#define FREE 1
#define USED 0
#define TRUE 1
#define FALSE 0
#define ERROR -1
#define SUCCESS 1

const size_t  INIT_MEM_SIZE     = 8192;
const int     NODE_HEADER_SIZE  = sizeof(MM_node);
const int     SPLIT_MINIMUM     = 2 * sizeof(MM_node);

MM_node       *malloc_head          = NULL;
MM_node       *malloc_tail               = NULL;


void *req_free_mem(size_t req_size)
{
        MM_node *prev_node = NULL;
        MM_node *new_node = NULL;
        MM_node *cur_node = malloc_head;
        if(cur_node == NULL)
                return NULL;

        while(cur_node != NULL)
        {
                if(cur_node->size > req_size + SPLIT_MINIMUM)
                {
                        //printf("split\n");
                        new_node = split_node(cur_node, req_size);
                        cur_node->status = USED;
                        // link prev node to the new split node
                        prev_node->next_free = new_node;
                        //printf("end split\n");

                        return cur_node + NODE_HEADER_SIZE;
                }
                else // TODO: case where we don't split but it's big enough
                {
                        //printf("traverse\n");
                        prev_node = cur_node;
                        cur_node = cur_node->next_free;
                }
        }

        return NULL;
}

// splits a block of memory starting at the address of node,
// inserting a new node header after the block of memory
// about to be returned to the user
// links new node to the original node
// returns the address to the new node created after the split
MM_node *split_node(MM_node *node, size_t req_size)
{
        if(req_size + NODE_HEADER_SIZE > node->size)
                printf("DANGER THIS SHOULD NEVER BE PRINTED\n");

        size_t new_node_size = node->size - req_size - NODE_HEADER_SIZE;
        MM_node *new_node_addr = (void *)((char *)node + NODE_HEADER_SIZE + req_size);
        MM_node *new_node = append_node(new_node_addr, new_node_size);

        if(malloc_tail == node)
        {
                malloc_tail = new_node;
        }

        node->size = req_size;
        new_node->prev = node;
        return new_node;
}

MM_node *construct_node(void *addr)
{
        MM_node *node = addr;
        node->size = 0;
        node->next_free = NULL;
        node->prev = NULL;
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
        }
        else
        {

                heap_bottom = align_addr(heap_bottom);
                // TODO on address align heap_bottom - req_mem_size may spill over top of heap
                // free head points to a dummy first node of size 0
                malloc_head = construct_node((char *)heap_bottom - req_mem_size);
                // the actual first node, so malloc_head will always point to a 'free node'
                malloc_tail = construct_node((char *)heap_bottom - req_mem_size + NODE_HEADER_SIZE);
                malloc_head->next_free = malloc_tail;
                malloc_tail->prev = malloc_head;
                malloc_tail->size = (size_t)heap_bottom - (size_t)malloc_head - 2 * NODE_HEADER_SIZE;
                printf("heap bottom: %p, %p,  %li\n", heap_bottom, malloc_head, (size_t)heap_bottom % 8);
                printf("top malloc_head: %lu, %p, %p, %d\n", malloc_head->size, malloc_head->next_free, malloc_head->prev, malloc_head->status);
                printf("top malloc_tail: %lu, %p, %p, %d\n", malloc_tail->size, malloc_tail->next_free, malloc_tail->prev, malloc_tail->status);

                return SUCCESS;
        }
}
/*
 * returns the closest address that is a multiple of 8 to the provided address.
 */
void *align_addr(void *addr)
{
        if((unsigned long)addr % 8 != 0)
        {
                // TODO
                printf("Memory was not aligned.\n");
                return addr;
        } else {
                return addr;
        }
}

/*
 * returns the nearest multiple of INIT_MEM_SIZE larger than req_mem_size
 * we want the calls to sbrk to be a multiple of 8K for uniformity
 */
size_t get_mem_size(size_t req_mem_size)
{
        return INIT_MEM_SIZE * (req_mem_size / INIT_MEM_SIZE + 1);
}

/*
 * appends a node to beginning  of free list with specified addr and size
 * DOES NOT HANDLE LINKING TO PREV NODE
 */
MM_node *append_node(void *addr, size_t total_size)
{
        MM_node *old_head = malloc_head->next_free;
        MM_node *new_node = construct_node(addr);
        new_node->size = total_size;
        malloc_head->next_free = new_node;
        new_node->next_free = old_head;
        return new_node;
}
/*
 * sbrk for new memory and add a node to the free list corresponding to said memory
 */
int add_new_mem(size_t size)
{
        void *heap_bottom;
        MM_node *new_node;

        size = get_mem_size(size);

        if((heap_bottom = sbrk(size)) == NULL)
        {
                // unable to sbrk successfully
                return ERROR;
        }
        else
        {
                heap_bottom = align_addr(heap_bottom);
                // TODO on address align heap_bottom - req_mem_size may spill over top of heap
                new_node = append_node((char *)heap_bottom - size, size);
                new_node->prev = malloc_tail;
                malloc_tail = new_node;
                return SUCCESS;
        }
}

// returns the nearest multiple of 8 larger than provided size
size_t pad_mem_size(size_t size)
{
        if(size % 8 == 0)
                return size;
        else
                return 8 * (size / 8 + 1);
}

void *mm_malloc_ll(size_t size)
{
        int status;
        void *ptr;
        printf("requested size: %lu, new size: %lu\n", size, pad_mem_size(size));

        size = pad_mem_size(size);

        if(malloc_head == NULL)
        {
                status = initialize(size);
        }
        if(status == ERROR)
                return NULL;


        ptr = req_free_mem(size);
        if(ptr == NULL)
        {
                printf("failed to find free memory on first try.\n");
                status = add_new_mem(size);
        }
        if(status == ERROR)
                return NULL;

        ptr = req_free_mem(size);

        return ptr;
        //return calloc(1, size);
}

void *mm_realloc_ll(void *ptr, size_t size)
{
        printf("call to unimplemented realloc\n");
        return mm_malloc_ll(size);
        //return realloc(ptr, size);
}

void mm_free_ll(void *ptr)
{
        printf("call to unimplemented free\n");
        return;
        //free(ptr);
}

