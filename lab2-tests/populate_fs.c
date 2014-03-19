#include <lpfs/compat.h>
#include <lpfs/lpfs.h>
#include <stdio.h>

#define SEGMENT_BLOCK_COUNT = 2048

static u64 pointer_to_byte_addr(struct disk d, void *p)
{
	return ((u64) p) - ((u64) d.buffer);
}

/* A program to add premounted
 * files to the disk image for testing purposes. */
int main(int argc, char** argv) {
  if (argc != 2) {
    exit(1);
  }
  char* loop = argv[1];
  struct disk d;
  mount_disk(&d, loop);
  char* head = d.buffer;
  struct lp_superblock_fmt *sb = (void*)head;
  struct lp_snapshot_fmt *snap0 = (void*)(head + (2 * LP_SEGSZ));
  struct lp_data_seg_fmt *data1 = (void*)(head + (3 * LP_SEGSZ));
  struct lp_inode_map_fmt *in_ent = (void*) ((u8*) snap0) + LP_SNAP_IMAP_OFF + sizeof(struct lp_inode_map_fmt);;

  void* dPayload = ((u8*) data1) + (2 * LP_BLKSZ);
  struct lp_inode_fmt* iPayload = (void*)((u8*) data1) + (1 * LP_BLKSZ) + sizeof(struct lp_inode_fmt);
  char* fString1 = "This is a file that you can read";
  iPayload->ino = 3;
  iPayload->size = (strlen(fString1) + 1);
  iPayload->version = 0;
  iPayload->ctime_usec = snap0->timestamp;
  iPayload->mtime_usec = snap0->timestamp;
  iPayload->uid = 0; //A god am I
  iPayload->gid = 0;
  iPayload->mode = (u16) 00000755;
  iPayload->link_count = 1;
  iPayload->bmap[0] = (u64) dPayload;

  memset(head + (3 * LP_SEGSZ) + (2 * LP_BLKSZ), 0, LP_BLKSZ);
  strcpy(dPayload, fString1);
  
  // Update the data header
  data1->hdr.seg_len = (3 * LP_BLKSZ);
  data1->nr_blocks_used = 2;
  data1->block_util[0] = 2;
  data1->block_util[1] = 255;
  
  data1->hdr.checksum = 0;
  data1->hdr.checksum = __lpfs_fnv(data1, data1->hdr.seg_len); 

  //Make an inode entry
  in_ent->inode_number = 3;
  in_ent->inode_byte_addr = pointer_to_byte_addr(d, iPayload);

  //Update the snapshot
  snap0->snap_hdr.nr_imap_ents = 2;
  snap0->hdr.seg_len += sizeof(struct lp_inode_map_fmt);
  snap0->hdr.checksum = 0;

  snap0->hdr.checksum = __lpfs_fnv(snap0, snap0->hdr.seg_len);

  u32* SUT = (void *)(head + sb->sut_off);
  SUT[0] = snap0->hdr.seg_len;
  SUT[1] = data1->hdr.seg_len;
  

  sb->next_inode_num = 4;
  sb->checksum = 0;
  sb->checksum = __lpfs_fnv(sb, LP_BLKSZ);
  
  //Party's over
  unmount_disk(&d);
}
