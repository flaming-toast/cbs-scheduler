/*
 * txn.c
 *
 * Transaction support
 */

#include "struct.h"

#pragma GCC optimize ("-O0")

struct lpfs_tx *lpfs_tx_new(struct lpfs_darray *d, enum lp_segment_flags type)
{
	struct lpfs_tx *tx;
	struct lpfs *ctx = d->ctx;

	BUG_ON(d->nr_blocks != LP_BLKS_PER_SEG);
	tx = (struct lpfs_tx *) kzalloc(sizeof(struct lpfs_tx), GFP_NOIO);
	if (!tx) {
		return NULL;
	}

	tx->sut_updates = (int *) kzalloc(ctx->sb_info.sut_len, GFP_NOIO);
	if (!tx->sut_updates) {
		kfree(tx);
		return NULL;
	}

	mutex_init(&tx->lock);
	tx->state = LP_TX_OPEN;
	tx->seg_addr = LP_BLK_TO_SEG(d->blk_addr);
	tx->seg_type = type;
	memcpy(&tx->seg, d, sizeof(struct lpfs_darray));

	INIT_LIST_HEAD(&tx->list);
	spin_lock(&d->ctx->txs_lock);
	list_add_tail(&tx->list, &d->ctx->txs_list);
	spin_unlock(&d->ctx->txs_lock);

	return tx;
}

int lpfs_tx_commit(struct lpfs *ctx, void *buf, u32 len, u64 *byte_addr,
	u64 byte_addr_hint, enum lpfs_tx_flags flags)
{
	/*
	 * 1) Determine placement
	 *	- Use "block_util" given a payload segment
	 *	- Use "nr_imap_ents" given a snapshot segment
	 *	- Always prefer @byte_addr_hint if possible
	 * 2) If the current transaction cannot accommodate @buf
	 *	- Create a new transaction, try again
	 * 3) Append everything to the journal, unless flags & FROM_JNL
	 * 4) Carefully update the SUT
	 *	- Inode map updates require up to 4 SUT updates
	 * 5) Keep the transaction state updated at all times
	 * 6) Update the superblock as needed
	 */

	return -EINVAL;
}

int lpfs_tx_sync(struct lpfs_tx *tx)
{
	return -EINVAL;
}

void lpfs_tx_destroy(struct lpfs_tx *tx)
{
	struct lpfs *ctx = tx->seg.ctx;

	spin_lock(&ctx->txs_lock);
	list_del(&tx->list);
	spin_unlock(&ctx->txs_lock);

	kfree(tx->sut_updates);
	kfree(tx);
}

int lpfs_syncer(void *data)
{
	return -EINVAL;
}

int lpfs_cleaner(void *data)
{
	return -EINVAL;
}
