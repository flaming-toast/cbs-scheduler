/*
 * inode.c
 *
 * Inode operations
 */

#include "struct.h"

#pragma GCC optimize ("-O0")

static int lpfs_collect_inodes(struct lpfs *ctx, u64 ino, struct inode *inode);

struct inode *lpfs_inode_lookup(struct lpfs *ctx, u64 ino)
{
	struct inode *inode = iget_locked(ctx->sb, ino);
	if (inode->i_state & I_NEW) {
		if (lpfs_collect_inodes(ctx, ino, inode)) {
			iput(inode);
			return NULL;
		} else {
			return inode;
		}
	} else {
		return inode;
	}
}

static void lpfs_fill_inode(struct lpfs *ctx,
			    struct inode *inode,
			    struct lp_inode_fmt *d_inode);

int lpfs_collect_inodes(struct lpfs *ctx, u64 ino, struct inode *inode)
{
	struct buffer_head *bh;
	struct lpfs_inode_map *imap, *i_srch;
	struct lp_inode_fmt *head;
	struct inode *i_probe;
	u64 ino_blk;
	u64 ino_off;
	u64 head_off;

	i_srch = lpfs_imap_lookup(ctx, ino);
	if (!i_srch) {
		return -ENOENT;
	}

	bh = sb_bread(ctx->sb, i_srch->inode_byte_addr / LP_BLKSZ);
	if (bh == NULL) {
		return -EIO;
	}

	ihold(inode);

	for (head = (struct lp_inode_fmt *) bh->b_data;
	     LP_DIFF(bh->b_data, head) < LP_BLKSZ && head->ino > 0; ++head)
	{
		imap = head->ino == ino ? i_srch
					: lpfs_imap_lookup(ctx, head->ino);
		if (!imap) {
			continue;
		}

		if (head->ino == ino) {
			i_probe = inode;
		} else {
			i_probe = ilookup(ctx->sb, ino);
			if (i_probe) {
				goto skip;
			} else {
				i_probe = iget_locked(ctx->sb, head->ino);
			}
		}

		ino_blk = imap->inode_byte_addr / LP_BLKSZ;
		ino_off = imap->inode_byte_addr % LP_BLKSZ;
		head_off = LP_DIFF(bh->b_data, head);
		if (ino_blk == bh->b_blocknr && ino_off == head_off) {
			lpfs_fill_inode(ctx, i_probe, head);
		}

skip:
		iput(i_probe);
	}

	brelse(bh);
	return 0;
}


void lpfs_fill_inode(struct lpfs *ctx, struct inode *inode,
		     struct lp_inode_fmt *d_inode)
{
	inode->i_ino = d_inode->ino;
	set_nlink(inode, d_inode->link_count);
	i_uid_write(inode, d_inode->uid);
	i_gid_write(inode, d_inode->gid);
	inode->i_version = d_inode->version;
	inode->i_size = d_inode->size;
	inode->i_atime = CURRENT_TIME;
	inode->i_mtime = ns_to_timespec(d_inode->mtime_usec * NSEC_PER_USEC);
	inode->i_ctime = ns_to_timespec(d_inode->ctime_usec * NSEC_PER_USEC);
	inode->i_blkbits = LP_BLKSZ_BITS;
	inode->i_blocks = (inode->i_size / LP_BLKSZ)
	    + (blkcnt_t) ((inode->i_size % LP_BLKSZ) != 0);
	inode->i_mode = d_inode->mode;

	inode->i_sb = ctx->sb;

	inode->i_op = &lpfs_inode_ops;

	if (inode->i_mode & S_IFDIR) {
		inode->i_fop = &lpfs_dir_ops;
	} else {
		inode->i_fop = &lpfs_file_ops;
	}

	unlock_new_inode(inode);
	insert_inode_hash(inode);
}

struct super_operations lpfs_super_ops;
struct inode_operations lpfs_inode_ops;
struct file_operations lpfs_file_ops;
struct file_operations lpfs_dir_ops;
