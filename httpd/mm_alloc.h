/*
 * mm_alloc.h
 *
 * Exports a clone of the interface documented in "man 3 malloc".
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);
void mm_free(void *ptr);

/*
 * mm_malloc node in memory list
 */
typedef struct mm_node
{
// size of this block's free space (not counting the header)
        int free_size;
// pointer to next free block
        struct mm_node *next_free;
// pointer to immediately previous block
        struct mm_node *prev;
// information regarding if this block is free or taken
        int status;
} MM_node;

MM_node *construct_node(void *addr, MM_node *prev);
int initialize(size_t mem_init_size);
void *mm_malloc_ll(size_t size);
void *mm_realloc_ll(void *ptr, size_t size);
void mm_free_ll(void *ptr);
int get_mem_size(size_t req_mem_size);
void *align_addr(void *addr);


#ifdef __cplusplus
}
#endif
