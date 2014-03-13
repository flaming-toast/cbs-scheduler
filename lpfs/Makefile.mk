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
UMODE_FLAG = $(shell grep -q umode_t /usr/include/asm-generic/types.h \
	     || echo "-D_NEED_UMODE_T")
LPFS_CFLAGS = -Wall -Wextra -std=gnu99 -g -D_USERSPACE -I. $(UMODE_FLAG)
LPFS_LDFLAGS = -lpthread -lrt

.lpfs/%.o: lpfs/%.c $(LPFS_HDRS)
	gcc $(LPFS_CFLAGS) -c -o $@ $<

.lpfs/mkfs-lp: userspace/mkfs-lp.c .lpfs/compat.o 
	gcc $(LPFS_CFLAGS) -o $@ $^ $(LPFS_LDFLAGS)

.lpfs/populate_fs: lab2-tests/populate_fs.c
	gcc $(LPFS_CFLAGS) -o $@ $^ $(LPFS_LDFLAGS) 

.lpfs/fsdb: userspace/fsdb.c $(LPFS_OBJS)
	gcc $(LPFS_CFLAGS) -o $@ $^ $(LPFS_LDFLAGS) 

.lpfs/disk.img: .lpfs/mkfs-lp .lpfs/populate_fs
	dd if=/dev/zero of=.lpfs/disk.img bs=1M count=128
	make reset_loop
	.lpfs/mkfs-lp /dev/loop0
	.lpfs/populate_fs /dev/loop0
	sync
	chmod 777 .lpfs/disk.img

all: .lpfs/mkfs-lp .lpfs/fsdb

# Ensure that .lpfs/disk.img is attached to /dev/loop0.
.PHONY: reset_loop
reset_loop:
	lsmod | grep -q loop || modprobe loop
	losetup -d /dev/loop0 2> /dev/null || true
	# Associate a loopback device with the file
	losetup /dev/loop0 .lpfs/disk.img
	# Encrypt storage in the device. cryptsetup will use the Linux
	# device mapper to create, in this case, /dev/mapper/lpfs.
	
	# If you want to use LUKS, you should use the following two
	# commands (optionally with additional) parameters. The first
	# command initializes the volume, and sets an initial key. The
	# second command opens the partition, and creates a mapping
	# (in this case /dev/mapper/secretfs).
	cryptsetup -y luksFormat /dev/loop0
	cryptsetup luksOpen /dev/loop0 lpfs'

.PHONY: fsdb
fsdb: .lpfs/fsdb .lpfs/disk.img
	make reset_loop
	.lpfs/fsdb /dev/loop0 snapshot=0

.PHONY: install_lpfs
install_lpfs: # $(LPFS_OBJS)
	mkdir -p linux/fs/lpfs
	cp -ru lpfs/* linux/fs/lpfs/
	cp -u userspace/kernel.config linux/.config
	cp -u userspace/fs.Kconfig linux/fs/Kconfig
	cp -u userspace/fs.Makefile linux/fs/Makefile
	lab2-tests/initrd.gz

lab2-tests/initrd.gz: lab2-tests/interactive_config busybox/busybox \
	linux/usr/gen_init_cpio lab2-tests/interactive_config
	linux/usr/gen_init_cpio "$<" | gzip > "$@"

linux: install_lpfs
