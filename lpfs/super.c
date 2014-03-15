/*
 * super.c
 *
 * Mount a lpfs partition
 */

#include "struct.h"

#pragma GCC optimize ("-O0")

struct lpfs *lpfs_ctx_create(struct super_block *sb)
{
	int err;
	struct buffer_head *bh;
	struct lp_superblock_fmt *sb_fmt;
	struct lpfs *ctx = NULL;

	sb->s_blocksize = LP_BLKSZ;
	sb->s_blocksize_bits = LP_BLKSZ_BITS;

	bh = sb_bread(sb, 0);
	if (bh == NULL) {
		printk(KERN_ERR "lpfs: Failed to read superblock");
		return NULL;
	}

	sb_fmt = (struct lp_superblock_fmt *) bh->b_data;

	ctx = (struct lpfs *) kzalloc(sizeof(struct lpfs), GFP_NOIO);
	if (!ctx) {
		printk(KERN_ERR "lpfs: Out of memory");
		brelse(bh);
		return NULL;
	}

	ctx->sb = sb;
	ctx->sb_buf = bh;
	memcpy(&ctx->sb_info, sb_fmt, sizeof(struct lp_superblock_fmt));

	err = lpfs_checksum(sb_fmt, LP_BLKSZ,
			    offsetof(struct lp_superblock_fmt, checksum));
	if (err) {
		printk(KERN_ERR "lpfs: Superblock failed to checksum");
		goto fail;
	}

	if (sb_fmt->block_size != LP_BLKSZ || sb_fmt->segment_size != LP_SEGSZ)
	{
		printk(KERN_ERR "lpfs: Misconfigured block/segment sizes");
		goto fail;
	}

	ctx->inode_map = RB_ROOT;
	ctx->imap_cache = kmem_cache_create("imap_cache",
					    sizeof(struct lpfs_inode_map),
					    0, 0, NULL);
	if (!ctx->imap_cache) {
		printk(KERN_ERR "lpfs: Could not create imap slab cache");
		goto fail;
	}

	INIT_LIST_HEAD(&ctx->txs_list);
	spin_lock_init(&ctx->txs_lock);

	return ctx;

fail:
	lpfs_ctx_destroy(ctx);
	return NULL;
}

void lpfs_ctx_destroy(struct lpfs *ctx)
{
	if (ctx->imap_cache) {
		lpfs_imap_destroy(ctx);
	}

	if (ctx->SUT.blocks) {
		lpfs_darray_destroy(&ctx->SUT);
	}

	if (ctx->journal.blocks) {
		lpfs_darray_destroy(&ctx->journal);
	}

	/*
	if (ctx->syncer != NULL && !IS_ERR(ctx->syncer)) {
		kthread_stop(ctx->syncer);
	}

	if (ctx->cleaner != NULL && !IS_ERR(ctx->cleaner)) {
		kthread_stop(ctx->cleaner);
	}
	*/

	brelse(ctx->sb_buf);
	kfree(ctx);
}

int lpfs_read_segment(struct lpfs *ctx, struct lpfs_darray *d, u32 seg_addr)
{
	int err = 0;
	u64 blk_addr = LP_SEG_TO_BLK(seg_addr);
	struct lp_segment_info_fmt *hdr = NULL;

	err = lpfs_darray_create(ctx, d, blk_addr, LP_BLKS_PER_SEG, 1);
	if (err) {
		return err;
	}

	hdr = (struct lp_segment_info_fmt*) lpfs_darray_buf(d, 0);
	if (hdr->seg_len > LP_SEGSZ) {
		err = -EINVAL;
		goto fail;
	}

	err = lpfs_darray_checksum(d, 0, hdr->seg_len,
				   offsetof(struct lp_segment_info_fmt,
				   	    checksum));
	if (err) {
		goto fail;
	}

	return 0;

fail:
	printk(KERN_ERR "lpfs: Segment %u is corrupted\n", seg_addr);
	lpfs_darray_destroy(d);
	return err;
}

static int lpfs_load_imap_ents(struct lpfs_darray *d,
			       struct lp_snapshot_fmt *snap, u32 seg_addr);

int lpfs_load_snapshot(struct lpfs *ctx, u32 snap_id, struct lpfs_tx **tx)
{
	int err = 0;
	struct lpfs_darray d;
	struct lp_snapshot_fmt *snap = NULL;
	u32 cur_seg = ctx->sb_info.root_snap_seg;
	u32 next_snap_seg = LP_SEG_NONE;

	while (cur_seg != LP_SEG_NONE) {
		err = lpfs_read_segment(ctx, &d, cur_seg);
		if (err) {
			printk(KERN_ERR "lpfs: Failed to read segment");
			return err;
		}

		snap = (struct lp_snapshot_fmt *) lpfs_darray_buf(&d, 0);
		if (!(snap->hdr.seg_flags & (LP_SEG_SNAP | LP_SEG_SNAP_EXT))) {
			err = -EINVAL;
			printk(KERN_ERR "lpfs: Invalid snapshot header");
			goto fail;
		}

		err = lpfs_load_imap_ents(&d, snap, cur_seg);
		if (err) {
			printk(KERN_ERR "lpfs: Failed to load snapshot");
			goto fail;
		}

		if (snap->hdr.seg_flags & LP_SEG_SNAP) {
			struct lp_snapshot_fmt *s = (void *) snap;
			next_snap_seg = s->snap_next_seg;
		}

		cur_seg = snap->snap_hdr.snap_ext_seg;
		if (cur_seg == LP_SEG_NONE
		    && snap->snap_hdr.snap_id < snap_id)
		{
			cur_seg = next_snap_seg;
			next_snap_seg = LP_SEG_NONE;
		}

		if (cur_seg == LP_SEG_NONE
		    && snap_id == ctx->sb_info.last_snap_id)
		{
			*tx = lpfs_tx_new(&d, snap->hdr.seg_flags);
		} else {
			lpfs_darray_destroy(&d);
		}
	}

	return 0;

fail:
	lpfs_darray_destroy(&d);
	return err;
}


int lpfs_load_imap_ents(struct lpfs_darray *d,
			struct lp_snapshot_fmt *snap, u32 seg_addr)
{
	u64 i;
	u64 off;
	struct lp_inode_map_fmt *map_ent;
	struct lpfs_inode_map *imap;

	for (i = 0, off = LP_SNAP_IMAP_OFF;
	     i < snap->snap_hdr.nr_imap_ents;
	     ++i, off += sizeof(struct lp_inode_map_fmt))
	{
		if (off >= LP_SEGSZ) {
			printk(KERN_ERR "lpfs: Invalid snapshot map count");
			return -EINVAL;
		}

		map_ent = (struct lp_inode_map_fmt *) lpfs_darray_buf(d, off);
		imap = lpfs_imap_alloc(d->ctx, map_ent->inode_number,
				       map_ent->inode_byte_addr, seg_addr);
		if (!imap) {
			return -ENOMEM;
		}
		lpfs_imap_insert(d->ctx, &imap);
	}

	return 0;
}

static int lpfs_load_sut(struct lpfs *ctx);

static int lpfs_load_journal(struct lpfs* ctx);

int lpfs_fill_super(struct super_block *sb, void *data, int silent)
{
	int err;
	int snap_id;
	struct lpfs *ctx;
	struct inode *i_root;
	struct lpfs_tx *snap_tx = NULL;
	(void) silent;

	err = 0;
	ctx = lpfs_ctx_create(sb);
	if (!ctx) {
		printk(KERN_ERR "lpfs: Failed to mount disk");
		return -EIO;
	}

	snap_id = (int) ctx->sb_info.last_snap_id;
	if (data) {
		err = sscanf(data, "snapshot=%d", &snap_id);
		if (err != 1 || snap_id < 0
		    || snap_id > (int) ctx->sb_info.last_snap_id)
		{
			snap_id = (int) ctx->sb_info.last_snap_id;
		}
	}

	sb->s_flags |= MS_NODIRATIME | MS_NOATIME;
	if (snap_id < (int) ctx->sb_info.last_snap_id) {
		sb->s_mode = FMODE_READ | FMODE_EXCL;
		sb->s_flags |= MS_RDONLY;
	} else {
		sb->s_mode = FMODE_READ | FMODE_WRITE | FMODE_EXCL;
	}

	sb->s_magic = LPFS_MAGIC;
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_op = &lpfs_super_ops;
	sb->s_max_links = LP_LINK_MAX;
	memcpy((void *) &sb->s_id, (void *) &ctx->sb_info.fs_name, 32);
	memcpy((void *) &sb->s_uuid, (void *) &ctx->sb_info.fs_uuid, 16);
	sb->s_fs_info = ctx;

	err = lpfs_load_snapshot(ctx, snap_id, &snap_tx);
	if (err) {
		printk(KERN_ERR "lpfs: Failed to load snapshot %d", snap_id);
		goto fail;
	}

	if (!(sb->s_flags & MS_RDONLY)) {
		BUG_ON(!snap_tx);

		err = lpfs_load_sut(ctx);
		if (err) {
			goto fail;
		}

		err = lpfs_load_journal(ctx);
		if (err) {
			goto fail;
		}

		/*
		ctx->syncer = kthread_run(lpfs_syncer, ctx, "lpfs_syncer");
		if (IS_ERR(ctx->syncer)) {
			goto fail;
		}

		ctx->cleaner = kthread_run(lpfs_cleaner, ctx, "lpfs_cleaner");
		if (IS_ERR(ctx->cleaner)) {
			goto fail;
		}
		*/
	}

	i_root = lpfs_inode_lookup(ctx, LP_ROOT_INO);
	if (!i_root) {
		err = -EIO;
		printk(KERN_ERR "lpfs: Could not locate root inode");
		goto fail;
	}

	sb->s_root = d_make_root(i_root);

	return 0;

fail:
	lpfs_ctx_destroy(ctx);
	return err;
}

int lpfs_load_sut(struct lpfs *ctx)
{
	int err;
	err = lpfs_darray_create(ctx, &ctx->SUT,
			ctx->sb_info.sut_off / LP_BLKSZ,
			(ctx->sb_info.journal_off - ctx->sb_info.sut_off)
			/ LP_BLKSZ, 1);
	if (err) {
		printk(KERN_ERR "lpfs: Could not read SUT");
		goto exit;
	}

exit:
	return err;
}

int lpfs_load_journal(struct lpfs* ctx)
{
	int err;
	u64 jnl_off;
	struct lp_journal_entry_fmt jnl_ent;
	void *ent_buf;
	u32 cksum;
	u64 final_byte_addr;

	err = lpfs_darray_create(ctx, &ctx->journal,
			ctx->sb_info.journal_off / LP_BLKSZ,
			ctx->sb_info.journal_len / LP_BLKSZ, 1);
	if (err) {
		printk(KERN_ERR "lpfs: Could not read journal");
		goto exit;
	}

	for (jnl_off = 0; jnl_off < ctx->sb_info.journal_data_len;
	     jnl_off += jnl_ent.ent_len + sizeof(struct lp_journal_entry_fmt))
	{
		lpfs_darray_read(&ctx->journal, &jnl_ent,
				 sizeof(struct lp_journal_entry_fmt), jnl_off);

		ent_buf = kmalloc(jnl_ent.ent_len, GFP_NOIO);
		if (!ent_buf) {
			err = -ENOMEM;
			goto exit;
		}

		lpfs_darray_read(&ctx->journal, ent_buf, jnl_ent.ent_len,
			jnl_off + sizeof(struct lp_journal_entry_fmt));

		cksum = __lpfs_fnv(ent_buf, jnl_ent.ent_len);
		if (cksum != jnl_ent.ent_checksum) {
			err = -EIO;
			printk(KERN_ERR "lpfs: Corrupted journal entry");
			goto skip;
		}

		err = lpfs_tx_commit(ctx, ent_buf, jnl_ent.ent_len,
				&final_byte_addr, jnl_ent.ent_byte_addr,
				LP_TX_FROM_JNL | jnl_ent.ent_type);
		if (err) {
			printk(KERN_ERR "lpfs: Could not place journal entry");
			goto exit;
		}

		if (jnl_ent.ent_byte_addr != final_byte_addr) {
			err = -EINVAL;
			printk(KERN_ERR "lpfs: Journal hints have failed");
			err = -EFAULT;
			goto skip;
		}

skip:
		kfree(ent_buf);

		if (err) {
			goto exit;
		}
	}

exit:
	if (err) {
		lpfs_darray_destroy(&ctx->journal);
	}
	return err;
}

static struct dentry *lpfs_mount(struct file_system_type *fs_type, int flags,
				 const char *dev_name, void *data)
{
	printk(KERN_INFO "lpfs: mount %s, %s\n", dev_name, (char *) data);
#ifdef __LPFS_DARRAY_TEST
	return mount_bdev(fs_type, flags, dev_name, data, __lpfs_darray_test);
#else
	return mount_bdev(fs_type, flags, dev_name, data, lpfs_fill_super);
#endif
}

static struct file_system_type lpfs_fs_type = {
	.name = "lpfs",
	.mount = &lpfs_mount,
	.kill_sb = &kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
	.owner = THIS_MODULE,
};

int __init lpfs_init(void)
{
	return register_filesystem(&lpfs_fs_type);
}

void __exit lpfs_exit(void)
{
	unregister_filesystem(&lpfs_fs_type);
}

module_init(lpfs_init);
module_exit(lpfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vedant Kumar <vsk@berkeley.edu>");
MODULE_DESCRIPTION("lpfs");
