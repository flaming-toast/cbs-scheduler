#include "compat.h"
#include <stdio.h>

#define SEGMENT_BLOCK_COUNT = 2048

static u64 pointer_to_byte_addr(struct disk *d, void *p)
{
	return ((u64) p) - ((u64) d->buffer);
}

/* A program to add premounted
 * files to the disk image for testing purposes. */
int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "Usage: populate_fs <mounted loop device>";
    exit(1);
  }
  const char* loop = argv[1];
  struct disk d;
  mount_disk(&d, loop);
  char* head = d->buffer;
  struct lp_superblock_fmt *sb = (void*)head;
  struct lp_snapshot_fmt *snap0 = head + (2 * LP_SEGSZ);
  struct lp_data_seg_fmt *data1 = head + (3 * LP_SEGSZ);
  struct lp_inode_map_fmt *in_ent = ((u8*) snap0) + LP_SNAP_IMAP_OFF;

  void* dPayload = ((u8*) data1) + (2 * LP_BLOCKSZ);
  struct lp_inode_fmt* iPayload = ((u8*) data1) + (1 * LP_BLOCKSZ) + sizeof(lp_inode_fmt);
  char* fString1 = "This is a file that you can read";
  iPayload->ino = 3;
  iPayload->size = (strlen(fString1) + 1);
  iPayload->version = 0;
  iPayload->ctime = snap0->timestamp;
  iPayload->mtime = snap0->timestamp;
  iPayload->uid = 0; //A god am I
  iPayload->gid = 0;
  iPayload->mode = (u16) 00000755;
  iPayload->link_count = 1;
  iPayload->bmap[0] = dPayload;

  memset(head + (3 * LP_SEGSZ) + (2 * LP_BLOCKSZ), 0, LP_BLOCKSZ);
  strcpy(dPayload, fString1);
  
  // Update the data header
  data1->hdr.seg_len = (3 * LP_BLOCKSZ);
  data1->nr_blocks_used = 2;
  data1->block_util[0] = 2;
  data1->block_util[1] = 255;
  data1->hdr.checksum = __lpfs_fnv(data1, data1->hdr.seg_len); 

  //Make an inode entry
  in_ent->inode_number = 3;
  in_ent->inode_byte_addr = pointer_to_byte_addr(d, iPayload);

  //Update the snapshot
  snap0->snap_hdr.nr_imap_ents = 2;
  
  snap0->hdr.checksum = __lpfs_fnv(snap0, snap0->hdr.seg_len);

  u32* SUT = (void *)(head + sb->sut_off);
  SUT[1] = data1->hdr.seg_len;
  

  sb->next_inode_num = 4;
  
  //Party's over
  unmount_disk(&d);
}
