/*
 * mkfs-lp.c
 *
 * Create an lpfs-formatted image
 */

#include <lpfs/lpfs.h>
#include <lpfs/compat.h>

static void usage()
{
	puts("Usage: mkfs-lp /dev/<disk>");
	exit(1);
}

static void format_disk(struct disk *d);

int main(int argc, char **argv)
{
	if (argc != 2) {
		usage();
	}

	struct disk d;
	char *path = argv[1];

	mount_disk(&d, path);
	format_disk(&d);
	unmount_disk(&d);

	puts("Disk formatted successfully.");

	return 0;
}

static void *segment_of(char *head, u32 seg_addr)
{
	return head + (2 * LP_SEGSZ) + (seg_addr * LP_SEGSZ);
}

static void *inode_map_of(void *snap)
{
	return ((u8 *) snap) + LP_SNAP_IMAP_OFF;
}

static void *payload_of(struct lp_data_seg_fmt *data, u32 k)
{
	return ((u8 *) data) + ((k + 1) * LP_BLKSZ);
}

static u64 pointer_to_byte_addr(struct disk *d, void *p)
{
	return ((u64) p) - ((u64) d->buffer);
}

void format_disk(struct disk *d)
{
	char *head = d->buffer;
	struct lp_superblock_fmt *sb = (void *)head;
	struct lp_snapshot_fmt *snap0 = segment_of(head, 0);
	struct lp_data_seg_fmt *data1 = segment_of(head, 1);
	struct lp_inode_map_fmt *imap_ent = inode_map_of(snap0);
	struct lp_inode_fmt *i_root = payload_of(data1, 0);
	struct lp_inode_fmt *i_file0 = i_root + 1;
	u32 *uuid = (void *)&sb->fs_uuid;
	struct timeval tp;
	u32 *SUT;

	if (d->buf_size < 4 * LP_SEGSZ) {
		fprintf(stderr, "This disk is too small. The minimum size is "
			"%d bytes, or %d megabytes.\n",
			4 * LP_SEGSZ, 4 * LP_SEGSZ / (1 << 20));
		exit(1);
	}

	if (LP_SEGSZ % LP_BLKSZ != 0 || !(LP_BLKSZ < LP_SEGSZ)) {
		fprintf(stderr, "The block and segment sizes aren't set "
			"properly.\n");
		exit(1);
	}

	if (LP_SNAP_IMAP_OFF < sizeof(struct lp_snapshot_fmt)) {
		fprintf(stderr, "The inode map isn't properly aligned.\n");
		exit(1);
	}

	gettimeofday(&tp, NULL);
	srand(tp.tv_usec);

	memset(sb, 0, LP_BLKSZ);

	sb->magic = LPFS_MAGIC;
	sb->major = LPFS_MAJOR;
	sb->minor = LPFS_MINOR;

	sb->disk_size = d->buf_size;
	sb->nr_segments = (sb->disk_size / LP_SEGSZ) - 2;
	sb->min_free_segs = sb->nr_segments / 8;
	sb->block_size = LP_BLKSZ;
	sb->segment_size = LP_SEGSZ;

	sb->sut_off = LP_BLKSZ;
	sb->sut_len = 4 * sb->nr_segments;
	u32 sut_end = sb->sut_off + sb->sut_len;
	sb->journal_off = sut_end + LP_BLKSZ - (sut_end % LP_BLKSZ);
	sb->journal_len = 2 * LP_SEGSZ - sb->journal_off;
	sb->journal_data_len = 0;

	sb->root_snap_id = 0;
	sb->root_snap_seg = 0;
	sb->last_snap_id = 0;
	sb->last_snap_seg = 0;

	sb->next_inode_num = LP_ROOT_INO + 2;

	sb->fs_name[0] = 'l';
	sb->fs_name[1] = 'p';
	sb->fs_name[2] = 'f';
	sb->fs_name[3] = 's';
	uuid[0] = (u32) rand();
	uuid[1] = (u32) rand();
	uuid[2] = (u32) rand();
	uuid[3] = (u32) rand();

	if (!(sb->sut_off < sb->journal_off
	      && sb->sut_off % LP_BLKSZ == 0
	      && sb->journal_off % LP_BLKSZ == 0
	      && sb->journal_off < (2 * LP_SEGSZ))) {
		fprintf(stderr, "Error: The disk format is broken...\n");
		exit(1);
	}

	sb->checksum = __lpfs_fnv(sb, LP_BLKSZ);

	memset(snap0, 0, LP_BLKSZ);

	SUT = (void *)(head + sb->sut_off);
	memset(SUT, 0, sb->sut_len);

	snap0->hdr.checksum = 0;
	snap0->hdr.seg_len = LP_SNAP_IMAP_OFF +
		(2 * sizeof(struct lp_inode_map_fmt));
	snap0->hdr.seg_prev = LP_SEG_NONE;
	snap0->hdr.seg_next = 1;
	snap0->hdr.seg_flags = LP_SEG_SNAP;

	snap0->snap_hdr.snap_id = 0;
	snap0->snap_hdr.snap_ext_seg = LP_SEG_NONE;
	snap0->snap_hdr.nr_imap_ents = 2;
	snap0->timestamp = (tp.tv_sec * 1000000) + tp.tv_usec;

	snap0->snap_next_seg = LP_SEG_NONE;
	snap0->snap_prev_seg = LP_SEG_NONE;

	imap_ent->inode_number = LP_ROOT_INO;
	imap_ent->inode_byte_addr = pointer_to_byte_addr(d, i_root);

	struct lp_inode_map_fmt *imap0_ent = imap_ent + 1;
	imap0_ent->inode_number = LP_ROOT_INO + 1;
	imap0_ent->inode_byte_addr = pointer_to_byte_addr(d, i_file0);

	snap0->hdr.checksum = __lpfs_fnv(snap0, snap0->hdr.seg_len);
	SUT[0] = snap0->hdr.seg_len;

	memset(data1, 0, 3 * LP_BLKSZ);

	data1->hdr.checksum = 0;
	data1->hdr.seg_len = 3 * LP_BLKSZ;
	data1->hdr.seg_prev = 0;
	data1->hdr.seg_next = LP_SEG_NONE;
	data1->hdr.seg_flags = LP_SEG_DATA;
	data1->nr_blocks_used = 3;

	u8 *data1_util = data1->block_util;
	data1_util[0] = 2;
	data1_util[1] = 255;

	i_root->ino = LP_ROOT_INO;
	i_root->size = 4096;
	i_root->version = 1;
	i_root->ctime_usec = snap0->timestamp;
	i_root->mtime_usec = snap0->timestamp;
	i_root->uid = 0;
	i_root->gid = 0;
	i_root->mode = (u16) 0040755;
	i_root->link_count = 1;
	i_root->bmap[0] = pointer_to_byte_addr(d, payload_of(data1, 1))
		/ LP_BLKSZ;

	i_file0->ino = LP_ROOT_INO + 1;
	i_file0->size = 0;
	i_file0->version = 1;
	i_file0->ctime_usec = snap0->timestamp;
	i_file0->mtime_usec = snap0->timestamp;
	i_file0->uid = i_file0->gid = 0;
	i_file0->mode = (u16) 00755;
	i_file0->link_count = 1;

	struct lp_dentry_fmt *dirent0 = payload_of(data1, 1);
	dirent0->inode_number = i_file0->ino;
	dirent0->name_length = strlen("hello.world");
	memcpy(&dirent0->name, "hello.world", dirent0->name_length);

	data1->hdr.checksum = __lpfs_fnv(data1, data1->hdr.seg_len);
	SUT[1] = data1->hdr.seg_len;
}
