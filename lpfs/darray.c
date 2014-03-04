/*
 * darray.c
 *
 * An array abstraction for disks
 */

#include "struct.h"

#pragma GCC optimize ("-O0")

int lpfs_darray_create(struct lpfs *ctx, struct lpfs_darray *d,
		       u64 blk_addr, u32 nblks, int do_read)
{
	int i;

	d->ctx = ctx;
	mutex_init(&d->lock);
	d->dirty = 0;
	d->blk_addr = blk_addr;
	d->nr_blocks = nblks;

	if (LP_BLKSZ * (blk_addr + nblks) > ctx->sb_info.disk_size) {
		return -ENOMEM;
	}

	d->blocks = kmalloc(nblks * sizeof(void *), GFP_NOIO);
	if (!d->blocks) {
		return -ENOMEM;
	}

	if (do_read) {
		for (i = 0; i < ((int) nblks); ++i) {
			sb_breadahead(ctx->sb, blk_addr + i);
		}
	}

	for (i = 0; i < ((int) nblks); ++i) {
		u64 blk = blk_addr + i;
		if (do_read) {
			d->blocks[i] = sb_bread(ctx->sb, blk);
		} else {
			d->blocks[i] = sb_getblk(ctx->sb, blk);
			memset(d->blocks[i]->b_data, 0, LP_BLKSZ);
		}

		if (!d->blocks[i]) {
			for (i = i - 1; i >= 0; --i) {
				brelse(d->blocks[i]);
			}
			kfree(d->blocks);
			return -EIO;
		}
	}

	return 0;
}

void lpfs_darray_lock(struct lpfs_darray *d)
{
	u32 i;
	mutex_lock(&d->lock);
	for (i = 0; d->blocks && i < d->nr_blocks; ++i) {
		lock_buffer(d->blocks[i]);
	}
}

void lpfs_darray_unlock(struct lpfs_darray *d)
{
	u32 i;
	mutex_unlock(&d->lock);
	for (i = 0; d->blocks && i < d->nr_blocks; ++i) {
		unlock_buffer(d->blocks[i]);
	}
}

char *lpfs_darray_buf(struct lpfs_darray *d, u64 d_off)
{
	u64 blk = d_off / LP_BLKSZ;
	BUG_ON(blk >= d->nr_blocks);
	return ((char*) d->blocks[blk]->b_data) + (d_off % LP_BLKSZ);
}

int lpfs_darray_checksum(struct lpfs_darray *d, u64 a, u64 b, u64 cksum_off)
{
	u8 c;
	u64 probe;
	u32 cksum;
	unsigned hash = 2166136261;
	u32 *d_cksum = (u32 *) lpfs_darray_buf(d, cksum_off);

	cksum = *d_cksum;
	*d_cksum = 0;

	for (probe = a; probe < b; ++probe) {
		c = lpfs_darray_buf(d, probe)[0];
		hash = (hash * 16777619) ^ c;
	}

	*d_cksum = cksum;

	if (cksum != hash) {
		printk(KERN_WARNING "Checksum mismatch, saw %x, got %x\n",
		       hash, cksum);
	}

	return cksum != hash;
}

static void lpfs_darray_io(struct lpfs_darray *d, void *buf,
			   u64 len, u64 d_off, int read)
{
	u64 blk;
	u64 blk_off;
	u64 to_write;
	char *dst;
	char *src = buf;

	while (len > 0) {
		blk = d_off / LP_BLKSZ;
		blk_off = d_off % LP_BLKSZ;
		BUG_ON(blk >= d->nr_blocks);

		to_write = min(len, LP_BLKSZ - blk_off);
		dst = ((char *) d->blocks[blk]->b_data) + blk_off;

		if (read) {
			memcpy(src, dst, to_write);
		} else {
			memcpy(dst, src, to_write);
			set_buffer_uptodate(d->blocks[blk]);
			mark_buffer_dirty(d->blocks[blk]);
		}

		len -= to_write;
		src += to_write;
		d_off += to_write;
	}

	if (!read && len > 0) {
		d->dirty = 1;
	}
}

void lpfs_darray_read(struct lpfs_darray *d, void *buf, u64 len, u64 d_off)
{
	lpfs_darray_io(d, buf, len, d_off, 1);
}

void lpfs_darray_write(struct lpfs_darray *d, void *buf, u64 len, u64 d_off)
{
	lpfs_darray_io(d, buf, len, d_off, 0);
}

int lpfs_darray_sync(struct lpfs_darray *d)
{
	u32 i;
	int err = 0;

	if (!d->dirty) {
		return err;
	}

	for (i = 0; i < d->nr_blocks; ++i) {
		if (buffer_dirty(d->blocks[i])) {
			err = sync_dirty_buffer(d->blocks[i]);

			if (err) {
				return err;
			}
		}
	}

	d->dirty = 0;

	return err;
}

void lpfs_darray_destroy(struct lpfs_darray *d)
{
	u32 i;
	WARN_ON(d->dirty);
	for (i = 0; i < d->nr_blocks; ++i) {
		BUG_ON(buffer_locked(d->blocks[i]));
		brelse(d->blocks[i]);
	}
	kfree(d->blocks);
	d->blocks = NULL;
}

#ifdef __LPFS_DARRAY_TEST
int __lpfs_darray_test(struct super_block *sb, void *data, int silent)
{
	int err;
	u64 i, j;
	void *head;
	void *probe;
	struct lpfs *ctx;
	struct lpfs_darray d;
	const u32 SEG = 3;
	(void) silent;
	(void) data;

	printk(KERN_INFO "lpfs: Starting darray tests.\n");

	err = 0;
	ctx = lpfs_ctx_create(sb);
	if (!ctx) {
		printk(KERN_ERR "lpfs: Failed to mount disk");
		return -EIO;
	}

	/* 1) Read a segment-sized array. */
	err = lpfs_darray_create(ctx, &d,
			LP_SEG_TO_BLK(SEG), LP_BLKS_PER_SEG, 1);
	BUG_ON(err && "lpfs: Failed test 1");

	/* 2) Write zeros to the array and sync it. */
	head = (void *) lpfs_darray_buf(&d, 0);
	memset(head, 0, LP_BLKSZ);
	lpfs_darray_lock(&d);
	for (j = LP_BLKSZ; j < LP_SEGSZ; j += LP_BLKSZ) {
		lpfs_darray_write(&d, head, LP_BLKSZ, j);
	}
	err = lpfs_darray_sync(&d);
	BUG_ON(err && "lpfs: Failed test 2");
	lpfs_darray_unlock(&d);

	/* 3a) Checksum a block, insert the checksum, sync. */
	j = 64;
	lpfs_darray_lock(&d);
	i = __lpfs_fnv(head, LP_BLKSZ);
	lpfs_darray_write(&d, &i, sizeof(u64), j);

	err = lpfs_darray_sync(&d);
	BUG_ON(err && "lpfs: Failed test 3a");

	lpfs_darray_unlock(&d);
	lpfs_darray_destroy(&d);

	/* 3b) Read the segment from (3a), verify the checksum. */
	j = 64;
	err = lpfs_darray_create(ctx, &d,
			LP_SEG_TO_BLK(SEG), LP_BLKS_PER_SEG, 1);
	BUG_ON(err && "lpfs: Failed test 3b (1)");

	err = lpfs_darray_checksum(&d, 0, LP_BLKSZ, j);
	BUG_ON(err && "lpfs: Failed test 3b (2)");

	/* 4) Test non-block-aligned IO/s. Alters blocks 0 - 5. */
	head = (void *) lpfs_darray_buf(&d, 0);
	lpfs_darray_lock(&d);
	for (i = 0; i < LP_BLKSZ; i += sizeof(u64)) {
		lpfs_darray_write(&d, &i, sizeof(u64), i);
	}
	lpfs_darray_write(&d, head, LP_BLKSZ, LP_BLKSZ + 107);
	lpfs_darray_write(&d, head, LP_BLKSZ, (3 * LP_BLKSZ) + 311);

	probe = (void *) lpfs_darray_buf(&d, 5 * LP_BLKSZ);
	lpfs_darray_read(&d, probe, LP_BLKSZ, LP_BLKSZ + 107);
	BUG_ON(memcmp(head, probe, LP_BLKSZ) && "lpfs: Failed test 4 (1)");
	lpfs_darray_read(&d, probe, LP_BLKSZ, (3 * LP_BLKSZ) + 311);
	BUG_ON(memcmp(head, probe, LP_BLKSZ) && "lpfs: Failed test 4 (2)");

	err = lpfs_darray_sync(&d);
	BUG_ON(err && "lpfs: Failed test 4 (3)");
	lpfs_darray_unlock(&d);
	lpfs_darray_destroy(&d);

	/* 5a) Create, update, and sync an array (over getblk). */
	j = 912;
	err = lpfs_darray_create(ctx, &d, LP_SEG_TO_BLK(SEG + 1), 1, 0);
	BUG_ON(err && "lpfs: Failed test 5a (1)");
	head = (void *) lpfs_darray_buf(&d, 0);
	lpfs_darray_lock(&d);
	lpfs_darray_write(&d, &j, sizeof(u64), 0);
	err = lpfs_darray_sync(&d);
	BUG_ON(err && "lpfs: Failed test 5a (2)");
	lpfs_darray_unlock(&d);
	lpfs_darray_destroy(&d);

	/* 5b) Read and verify the new segment from (5a). */
	err = lpfs_darray_create(ctx, &d, LP_SEG_TO_BLK(SEG + 1), 1, 1);
	BUG_ON(err && "lpfs: Failed test 5b (1)");
	lpfs_darray_lock(&d);
	lpfs_darray_read(&d, &i, sizeof(u64), 0);
	BUG_ON(i != j && "lpfs: Failed test 5b (2)");
	lpfs_darray_unlock(&d);
	lpfs_darray_destroy(&d);

	printk(KERN_INFO "lpfs: darray tests passed!\n");
	printk(KERN_INFO "Note: proceeding to graceful crash...\n");

	/* End of tests. */

	lpfs_ctx_destroy(ctx);
	return -EINVAL;
}
#endif
