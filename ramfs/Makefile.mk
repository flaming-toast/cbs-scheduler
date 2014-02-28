# Notes
# =====
#
# - .ramfs/fsdb
#
#   	A version of fsdb that hooks into ramfs.

RAMFS_HDRS = ramfs/compat.h
RAMFS_OBJS = .ramfs/inode.o .ramfs/compat.o
RAMFS_CFLAGS = -Wall -std=gnu99 -g -D_USERSPACE -I.
RAMFS_LDFLAGS = -lpthread

.ramfs/%.o: ramfs/%.c $(RAMFS_HDRS)
	gcc $(RAMFS_CFLAGS) -c -o $@ $<

.ramfs/fsdb: userspace/fsdb.c $(RAMFS_OBJS)
	gcc $(RAMFS_CFLAGS) $(RAMFS_LDFLAGS) -o $@ $^

.PHONY: fsdb_ramfs
fsdb_ramfs: .ramfs/fsdb
	.ramfs/fsdb /dev/null -ramfs

.PHONY: install_ramfs
install_ramfs:
	cp -u ramfs/inode.c linux/fs/ramfs/inode.c
	cp -u ramfs/compat.h linux/fs/ramfs/compat.h
	cp -u ramfs/Makefile linux/fs/ramfs/Makefile

linux: install_ramfs
