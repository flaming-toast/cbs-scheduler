/*
 * struct.h
 *
 * Structures and interfaces for lpfs
 */

#pragma once

#include "lpfs.h"
#include "compat.h"

#define LP_DIFF(p0, pf) (((u64) (pf)) - ((u64) (p0)))
#define LP_OFFSET(p, off) ((void*) (((char*) (p)) + (off)))
#define LP_SEG_TO_BLK(seg) (((seg) + 2) * LP_BLKS_PER_SEG)
#define LP_BLK_TO_SEG(blk) ((((blk) * LP_BLKSZ) / LP_SEGSZ) - 2)

struct lpfs;

struct lpfs_darray {
	struct lpfs *ctx;
	struct mutex lock;
	int dirty;
	u64 blk_addr;
	u32 nr_blocks;
	struct buffer_head **blocks;
};

struct lpfs {
	struct super_block *sb;
	struct buffer_head *sb_buf;
	struct lp_superblock_fmt sb_info;

	struct rb_root inode_map;
	struct kmem_cache *imap_cache;

	spinlock_t txs_lock;
	struct list_head txs_list;

	struct lpfs_darray SUT;
	struct lpfs_darray journal;

	struct task_struct *syncer;
	struct task_struct *cleaner;
};

struct lpfs_inode_map {
	u64 inode_number;
	u64 inode_byte_addr;
	u32 source_segment;
	struct rb_node rb;
};

enum lpfs_tx_state {
	LP_TX_OPEN,
	LP_TX_DIRTY,
	LP_TX_SYNC,
	LP_TX_STALE,
};

struct lpfs_tx {
	struct mutex lock;
	enum lpfs_tx_state state;
	struct list_head list;

	u32 seg_addr;
	enum lp_segment_flags seg_type;
	int *sut_updates;
	struct lpfs_darray seg;
};

enum lpfs_tx_flags {
	/* Mark buffers sourced from the journal. */
	LP_TX_FROM_JNL = 1 << 0,

	/* Mark new inode map entries. */
	LP_TX_IMAP_ADD_ENT = 1 << 1,

	/* Mark deleted inode map entries. */
	LP_TX_IMAP_DEL_ENT = 1 << 2,

	/* Mark an inode entry. */
	LP_TX_INODE_ENT = 1 << 3,

	/* Mark a block-sized payload entry. */
	LP_TX_PAYLOAD_ENT = 1 << 4,
};

/* super.c */

/*
 * Create a context from the superblock @sb.
 */
struct lpfs *lpfs_ctx_create(struct super_block *sb);

/*
 * Release the filesystem context @ctx.
 */
void lpfs_ctx_destroy(struct lpfs *ctx);

/*
 * Load segment @seg_addr into @d and checksum it.
 */
int lpfs_read_segment(struct lpfs *ctx, struct lpfs_darray *d, u32 seg_addr);

/*
 * Load snapshot number @snap_id, creating a transaction in @tx if needed.
 */
int lpfs_load_snapshot(struct lpfs *ctx, u32 snap_id, struct lpfs_tx **tx);

/* txn.c */

/*
 * Register a new transaction to manage a segment @d of type @type.
 */
struct lpfs_tx *lpfs_tx_new(struct lpfs_darray *d, enum lp_segment_flags type);

/*
 * Put a buffer (@buf, @len) into a transaction.
 * Request the final on-disk location of the buffer in @byte_addr.
 * Suggest that the final on-disk location should be @byte_addr_hint.
 * Respect any transaction hints in @flags.
 */
int lpfs_tx_commit(struct lpfs *ctx, void *buf, u32 len, u64 *byte_addr,
	u64 byte_addr_hint, enum lpfs_tx_flags flags);

/*
 * Sync a transaction @tx to disk.
 */
int lpfs_tx_sync(struct lpfs_tx *tx);

/*
 * Unregister and free a transaction @tx.
 */
void lpfs_tx_destroy(struct lpfs_tx *tx);

/*
 * The syncer thread.
 */
int lpfs_syncer(void *data);

/*
 * The cleaner thread.
 */
int lpfs_cleaner(void *data);

/* inode_map.c */

/*
 * Allocate an inode mapping from inode number @ino to byte address @addr.
 * Record the source segment address (@seg) of this mapping.
 */
struct lpfs_inode_map *lpfs_imap_alloc(struct lpfs *ctx, u64 ino, u64 addr,
				       u32 seg);

/*
 * Record an allocated mapping @(*imap). Set @(*imap) to NULL on overwrites.
 */
void lpfs_imap_insert(struct lpfs *ctx, struct lpfs_inode_map **imap);

/*
 * Find the inode mapping for inode number @ino.
 */
struct lpfs_inode_map *lpfs_imap_lookup(struct lpfs *ctx, u64 ino);

/*
 * Remove the inode mapping for inode number @ino.
 */
void lpfs_imap_delete(struct lpfs *ctx, u64 ino);

/*
 * Destroy the inode map.
 */
void lpfs_imap_destroy(struct lpfs *ctx);

/* we could put these in potentially better places, like mirror the layout
 * in ext2. i.e., have super_operations declared in super.c, file_operations declared 
 * in file.c, etc -jyu */

/* inode.c */
extern struct super_operations lpfs_super_ops;
extern struct inode_operations lpfs_inode_ops;
extern struct file_operations lpfs_file_ops;
extern struct file_operations lpfs_dir_ops;
extern struct address_space_operations lpfs_aops;

/* For any non-static functions, add their declarations here 
 * for the operations in the above ops tables 
 * let's put the actual function definition functions 
 * in the appropriate file -jyu */

/* Example -- for fsync */
/* file.c */
extern int lpfs_fsync(struct file *file, loff_t start, loff_t end,
		      int datasync);

/*
 * Scan the disk for inode number @ino.
 */
struct inode *lpfs_inode_lookup(struct lpfs *ctx, u64 ino);

/* darray.c */

/*
 * Initialize a disk array @d starting at block @blk_addr with @ctx.
 * Load @nblks blocks using bread() if @do_read, or using getblk() otherwise.
 */
int lpfs_darray_create(struct lpfs *ctx, struct lpfs_darray *d,
		       u64 blk_addr, u32 nblks, int do_read);

/*
 * Lock all buffers in @d.
 */
void lpfs_darray_lock(struct lpfs_darray *d);

/*
 * Unlock all buffers in @d.
 */
void lpfs_darray_unlock(struct lpfs_darray *d);

/*
 * Get a pointer to byte offset @d_off in @d.
 */
char *lpfs_darray_buf(struct lpfs_darray *d, u64 d_off);

/*
 * Checksum bytes [@a, @b) of @d, starting at byte offset @cksum_off.
 */
int lpfs_darray_checksum(struct lpfs_darray *d, u64 a, u64 b, u64 cksum_off);

/*
 * Read @len bytes from @d into @buf, starting at byte offset @d_off.
 */
void lpfs_darray_read(struct lpfs_darray *d, void *buf, u64 len, u64 d_off);

/*
 * Write @len bytes of @buf into @d, starting at byte offset @d_off.
 */
void lpfs_darray_write(struct lpfs_darray *d, void *buf, u64 len, u64 d_off);

/*
 * Sync @d to disk.
 */
int lpfs_darray_sync(struct lpfs_darray *d);

/*
 * Release and free @d.
 */
void lpfs_darray_destroy(struct lpfs_darray *d);

/* Enable/disable a darray test which replaces the mount logic.
#define X__LPFS_DARRAY_TEST
*/

#ifdef __LPFS_DARRAY_TEST
int __lpfs_darray_test(struct super_block *sb, void *data, int silent);
#endif
