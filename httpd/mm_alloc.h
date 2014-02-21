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
        size_t size;
// pointer to next free block
        struct mm_node *next_free;
// pointer to immediately previous block
        struct mm_node *prev;
// information regarding if this block is free or taken
        int status;
} MM_node;

// {B} CORE FUNCTIONS //
void *mm_malloc_ll(size_t size);
void *mm_realloc_ll(void *ptr, size_t size);
void mm_free_ll(void *ptr);
void zero_mem(void *ptr);
// {C} CORE HELPERS //
int initialize(size_t mem_init_size);
void *req_free_mem(size_t size);
int add_new_mem(size_t size);
MM_node *split_node(MM_node *node, size_t req_size);
MM_node *append_node(MM_node *new_node, size_t total_size);
void coalesce_left(MM_node *node);
void coalesce_right(MM_node *node);
// {D} UTILITIES //
MM_node *construct_node(void *addr);
size_t get_mem_size(size_t req_mem_size);
void *align_addr(void *addr);
MM_node *get_header(void *ptr);
size_t pad_mem_size(size_t size);
// {E} DEBUG TOOLS //
void print_free_blocks(void);
void sanity_free_head(void);
void mm_malloc_had_a_problem(void);

#ifdef __cplusplus
}
#endif
