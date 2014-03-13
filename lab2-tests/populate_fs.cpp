#include "compat.h"
#include <stdio.h>

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
  

}
