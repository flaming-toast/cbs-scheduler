/*
 * lpfs.h
 *
 * On-disk structures for lpfs
 */

#pragma once

#include "compat.h"

#define LPFS_MAGIC	0xb100dfed
#define LPFS_MAJOR	0
#define LPFS_MINOR	1
#define LP_BLKSZ_BITS	12
#define LP_BLKSZ	(1 << LP_BLKSZ_BITS)
#define LP_SEGSZ_BITS	23
#define LP_SEGSZ	(1 << LP_SEGSZ_BITS)
#define LP_SEG_NONE	((u32) ~0)
#define LP_ROOT_INO	2
#define LP_LINK_MAX	32000
#define LP_BLKS_PER_SEG (LP_SEGSZ / LP_BLKSZ)

/*
 *				    Layout
 *	-------------------------------------------------------------------
 *	| SB | SUT | Journal | Segment #0 | Segment #1 | ... | Segment #n |
 *	-------------------------------------------------------------------
 *	0    4K             16MB         24MB         32MB  ...
 */

struct lp_superblock_fmt {
	u32 magic;		// FS magic number
	u16 major;		// Major version (format compatibility)
	u16 minor;		// Minor version (software compatibility)
	u32 checksum;		// Superblock checksum

	u64 disk_size;		// Size of the disk, in bytes
	u32 nr_segments;	// Number of usable segments in the disk
	u32 min_free_segs;	// Number of segments reserved for cleaning
	u32 block_size;		// Size of a block, in bytes
	u32 segment_size;	// Size of a segment, in bytes

	u32 sut_off;		// Offset to start-of-SUT, in bytes
	u32 sut_len;		// Length of the SUT, in bytes

	u32 journal_off;	// Offset to start-of-journal, bytes
	u32 journal_len;	// Length of the journal, in bytes
	u32 journal_data_len;	// Payload bytes written to the journal

	u32 root_snap_id;	// Id of the earliest snapshot in the system
	u32 root_snap_seg;	// Segment address of the earlist snapshot
	u32 last_snap_id;	// Id of the newest snapshot in the system
	u32 last_snap_seg;	// Segment address of the newest snapshot

	u64 next_inode_num;	// Store the next unused inode number

	char fs_name[32];	// FS name
	u8 fs_uuid[16];		// FS uuid
};

enum lp_segment_flags {
	LP_SEG_SNAP = 1 << 0,
	LP_SEG_SNAP_EXT = 1 << 1,
	LP_SEG_DATA = 1 << 2,
};

struct lp_segment_info_fmt {
	u32 checksum;		// Segment checksum
	u32 seg_len;		// Length of the segment, in bytes
	u32 seg_prev;		// Segment address of the next segment
	u32 seg_next;		// Segment address of the previous segment
	u32 seg_flags;		// enum lp_segment_flags
};

struct lp_inode_map_fmt {
	u64 inode_number;	// Inode identifier
	u64 inode_byte_addr;	// Byte address of the inode
};

struct lp_snapshot_hdr_fmt {
	u32 snap_id;		// Snapshot identifier
	u32 snap_ext_seg;	// Snapshot extension segment address
	u32 nr_imap_ents;	// Number of entries in the snapshot inode map
};

#define LP_SNAP_IMAP_OFF	64
struct lp_snapshot_fmt {
	struct lp_segment_info_fmt hdr;
	struct lp_snapshot_hdr_fmt snap_hdr;

	u64 timestamp;		// Creation timestamp, microseconds past epoch

	u32 snap_next_seg;	// Segment address of the next snapshot
	u32 snap_prev_seg;	// Segment address of the previous snapshot
};

struct lp_snapshot_ext_fmt {
	struct lp_segment_info_fmt hdr;
	struct lp_snapshot_hdr_fmt snap_hdr;
};

struct lp_data_seg_fmt {
	struct lp_segment_info_fmt hdr;

	u32 nr_blocks_used;	// Number of blocks used in this segment
	u8 block_util[0];	// Map a block number to its utilization
};

#define LP_INODE_BMAP_SIZE	11
struct lp_inode_fmt {
	u64 ino;		// Inode identifier
	u64 size;		// Size of the file, in bytes
	u32 version;		// Inode version (for gc)
	u32 ctime_usec;		// File creation time (microseconds)
	u32 mtime_usec;		// File modification time (microseconds)
	u32 uid;		// User id
	u32 gid;		// Group id
	u16 mode;		// File mode bits
	u16 link_count;		// File link count
	u64 bmap[LP_INODE_BMAP_SIZE];	// First "n" file blocks
};

#define LP_PATH_NAME_LEN	119
struct lp_dentry_fmt {
	u64 inode_number;	// Inode identifier
	u8 name_length;		// Length of the name (without NULL byte)
	char name[LP_PATH_NAME_LEN];
};

struct lp_journal_entry_fmt {
	u16 ent_len;		// Length of the ent_data array, in bytes
	u16 ent_type;		// Type of the journal entry (see tx layer)
	u32 ent_checksum;	// Checksum of the ent_data array
	u64 ent_byte_addr;	// Target placement address
	char ent_data[0];	// Journal entry payload
};
