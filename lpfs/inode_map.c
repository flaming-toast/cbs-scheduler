/*
 * inode_map.c
 *
 * Inode map routines
 */

#include "struct.h"

#pragma GCC optimize ("-O0")

struct lpfs_inode_map *lpfs_imap_alloc(struct lpfs *ctx, u64 ino, u64 addr,
				       u32 seg)
{
	struct lpfs_inode_map *imap;
	imap = kmem_cache_alloc(ctx->imap_cache, GFP_NOIO);
	if (!imap) {
		return NULL;
	}
	imap->inode_number = ino;
	imap->inode_byte_addr = addr;
	imap->source_segment = seg;
	return imap;
}

static void lpfs_imap_discard(struct lpfs *ctx, struct lpfs_inode_map **imap);

void lpfs_imap_insert(struct lpfs *ctx, struct lpfs_inode_map **imap)
{
	struct rb_node *parent = NULL;
	struct rb_node **new = &(ctx->inode_map.rb_node);
	struct lpfs_inode_map *this = NULL;
	struct lpfs_inode_map *tgt = *imap;

	BUG_ON(!tgt);
	if (tgt->inode_byte_addr == 0) {
		lpfs_imap_discard(ctx, imap);
		return;
	}

	while (*new) {
		parent = *new;
		this = container_of(*new, struct lpfs_inode_map, rb);

		if (tgt->inode_number < this->inode_number) {
			new = &((*new)->rb_left);
		} else if (tgt->inode_number > this->inode_number) {
			new = &((*new)->rb_right);
		} else {
			this->source_segment = tgt->source_segment;
			this->inode_byte_addr = tgt->inode_byte_addr;
			lpfs_imap_discard(ctx, imap);
			return;
		}
	}

	rb_link_node(&(tgt->rb), parent, new);
	rb_insert_color(&(tgt->rb), &(ctx->inode_map));
}

void lpfs_imap_discard(struct lpfs *ctx, struct lpfs_inode_map **imap)
{
	kmem_cache_free(ctx->imap_cache, *imap);
	*imap = NULL;
}

struct lpfs_inode_map *lpfs_imap_lookup(struct lpfs *ctx, u64 ino)
{
	struct lpfs_inode_map *imap;
	struct rb_node *node = ctx->inode_map.rb_node;

	while (node) {
		imap = container_of(node, struct lpfs_inode_map, rb);

		if (ino < imap->inode_number) {
			node = node->rb_left;
		} else if (ino > imap->inode_number) {
			node = node->rb_right;
		} else {
			return imap;
		}
	}
	return NULL;
}

void lpfs_imap_delete(struct lpfs *ctx, u64 ino)
{
	struct lpfs_inode_map *imap;

	imap = lpfs_imap_lookup(ctx, ino);
	if (!imap) {
		return;
	}

	rb_erase(&imap->rb, &ctx->inode_map);
	kmem_cache_free(ctx->imap_cache, imap);
}

void lpfs_imap_destroy(struct lpfs *ctx)
{
	struct rb_node *cur;
	struct lpfs_inode_map *imap;

	for (cur = rb_first(&(ctx->inode_map)); cur; cur = rb_next(cur)) {
		imap = rb_entry(cur, struct lpfs_inode_map, rb);
		kmem_cache_free(ctx->imap_cache, imap);
	}
}
