# Notes
# =====
# 
# - .lpfs/disk.img
# 
# 	This image backs a loopback device (/dev/loop0), which looks just like
# 	a hard disk to Qemu. All I/Os are transparently synced to this image.
# 
# - .lpfs/mkfs-lp
#
#   	A lpfs formatter.
#
# - .lpfs/fsdb
#
#   	A userspace read-eval-print-loop for filesystems.

LPFS_HDRS = $(wildcard lpfs/*.h) $(wildcard userspace/*.h)
LPFS_OBJS = $(patsubst lpfs/%.c, .lpfs/%.o, $(wildcard lpfs/*.c))
LPFS_CFLAGS = -Wall -Wextra -std=gnu99 -g -D_USERSPACE -I.
LPFS_LDFLAGS = -lpthread

.lpfs/%.o: lpfs/%.c $(LPFS_HDRS)
	gcc $(LPFS_CFLAGS) -c -o $@ $<

.lpfs/mkfs-lp: userspace/mkfs-lp.c .lpfs/compat.o 
	gcc $(LPFS_CFLAGS) $(LPFS_LDFLAGS) -o $@ $^ 

.lpfs/fsdb: userspace/fsdb.c $(LPFS_OBJS)
	gcc $(LPFS_CFLAGS) $(LPFS_LDFLAGS) -o $@ $^

.lpfs/disk.img: .lpfs/mkfs-lp 
	dd if=/dev/zero of=.lpfs/disk.img bs=1M count=128
	make reset_loop
	.lpfs/mkfs-lp /dev/loop0
	sync
	chmod 777 .lpfs/disk.img

all: .lpfs/mkfs-lp .lpfs/fsdb

# Ensure that .lpfs/disk.img is attached to /dev/loop0.
.PHONY: reset_loop
reset_loop:
	lsmod | grep -q loop || modprobe loop
	losetup -d /dev/loop0 2> /dev/null || true
	losetup /dev/loop0 .lpfs/disk.img

.PHONY: fsdb
fsdb: .lpfs/fsdb .lpfs/disk.img
	make reset_loop
	.lpfs/fsdb /dev/loop0 snapshot=0

.PHONY: install_lpfs
install_lpfs: $(LPFS_OBJS)
	mkdir -p linux/fs/lpfs
	cp -ru lpfs/* linux/fs/lpfs/
	cp -u userspace/kernel.config linux/.config
	cp -u userspace/fs.Kconfig linux/fs/Kconfig
	cp -u userspace/fs.Makefile linux/fs/Makefile

linux: install_lpfs
