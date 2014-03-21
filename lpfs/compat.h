/*
 * compat.h
 *
 * A lpfs to userspace compatability layer
 */

#pragma once

#ifdef _USERSPACE

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stropts.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>

#include <unistd.h>
#include <pthread.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <userspace/list.h>
#include <userspace/rbtree.h>

#include <userspace/uthash.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint32_t fmode_t;

#ifdef _NEED_UMODE_T
typedef uint32_t umode_t;
#endif

#ifdef NDEBUG
#error Setting NDEBUG breaks BUG_ON, please disable it.
#endif

#define BUG_ON(x) (assert(!(x)))
#define WARN_ON(x) BUG_ON(x)
#define IS_ERR(x) (((u64) x) >= ((u64) -4095))

#define likely(x) x
#define unlikely(x) x

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define CURRENT_TIME ({						\
	struct timespec __tspec;				\
	clock_gettime(CLOCK_REALTIME, &__tspec);		\
	__tspec; })

#define min(a, b) (((a) < (b)) ? (a) : (b))

#define KERN_INFO "(info) "
#define KERN_WARNING "(warn) "
#define KERN_ERR "(err) "

#define GFP_NOIO 0
#define GFP_HIGHUSER 0
#define GFP_KERNEL 0

#define I_NEW (1 << 3)

#define THIS_MODULE NULL
#define MODULE_AUTHOR(x)
#define MODULE_LICENSE(x)
#define MODULE_DESCRIPTION(x)
#define __init
#define __exit
#define __user

#define MS_RDONLY        1      /* Mount read-only */
#define MS_NOSUID        2      /* Ignore suid and sgid bits */
#define MS_NODEV         4      /* Disallow access to device special files */
#define MS_NOEXEC        8      /* Disallow program execution */
#define MS_SYNCHRONOUS  16      /* Writes are synced at once */
#define MS_REMOUNT      32      /* Alter flags of a mounted FS */
#define MS_MANDLOCK     64      /* Allow mandatory locks on an FS */
#define MS_DIRSYNC      128     /* Directory modifications are synchronous */
#define MS_NOATIME      1024    /* Do not update access times. */
#define MS_NODIRATIME   2048    /* Do not update directory access times */
#define MS_VERBOSE      32768   /* War is peace. Verbosity is silence.
                                   MS_VERBOSE is deprecated. */
#define MS_SILENT       32768
#define MS_POSIXACL     (1<<16) /* VFS does not apply the umask */
#define MS_PRIVATE      (1<<18) /* change to private */
#define MS_KERNMOUNT    (1<<22) /* this is a kern_mount call */
#define MS_I_VERSION    (1<<23) /* Update inode I_version field */
#define MS_STRICTATIME  (1<<24) /* Always perform atime updates */
#define MS_ACTIVE       (1<<30)

#define FS_REQUIRES_DEV         1
#define FS_BINARY_MOUNTDATA     2
#define FS_USERNS_MOUNT		8

#ifndef FMODE_READ

/* file is open for reading */
#define FMODE_READ              ((u32)0x1)
/* file is open for writing */
#define FMODE_WRITE             ((u32)0x2)
/* file is seekable */
#define FMODE_LSEEK             ((u32)0x4)
/* file can be accessed using pread */
#define FMODE_PREAD             ((u32)0x8)
/* file can be accessed using pwrite */
#define FMODE_PWRITE            ((u32)0x10)
/* File is opened for execution with sys_execve / sys_uselib */
#define FMODE_EXEC              ((u32)0x20)
/* File is opened with O_NDELAY (only set for block devices) */
#define FMODE_NDELAY            ((u32)0x40)
/* File is opened with O_EXCL (only set for block devices) */
#define FMODE_EXCL              ((u32)0x80)
/* File is opened using open(.., 3, ..) and is writeable only for ioctls
   (specialy hack for floppy.c) */
#define FMODE_WRITE_IOCTL       ((u32)0x100)
/* 32bit hashes as llseek() offset (for directories) */
#define FMODE_32BITHASH         ((u32)0x200)
/* 64bit hashes as llseek() offset (for directories) */
#define FMODE_64BITHASH         ((u32)0x400)

#endif /* FMODE_READ */

#ifndef S_IRWXUGO

#define S_IRWXUGO       (S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO       (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO         (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO         (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO         (S_IXUSR|S_IXGRP|S_IXOTH)

#endif /* S_IRWXUGO */

#define MAX_LFS_FILESIZE ((u64) 0x7fffffffffffffffLL)
#define PAGE_CACHE_SHIFT 12
#define PAGE_CACHE_SIZE (1 << PAGE_CACHE_SHIFT)

struct module;
struct kstatfs;
struct super_operations;
struct file_operations;
struct inode_operations;
struct address_space_operations;
struct seq_file;
struct dentry;
struct file;
struct kiocb;
struct dir_context;
struct file_lock;
struct iattr;
struct iovec;
struct kstat;
struct nameidata;
struct page;
struct pipe_inode_info;
struct poll_table_struct;
struct vfsmount;
struct vm_area_struct;
struct backing_dev_info;
struct address_space;

struct kstatfs {
	long f_type;
	long f_bsize;
  u64 f_blocks;
  u64 f_bfree;
  u64 f_bavail;
  u64 f_files;
  u64 f_ffree;
	long f_namelen;
  //fsid ??
  // Defined in header posix_types.h also included by linux/fs.h
  __kernel_fsid_t f_fsid;
};
struct mutex {
	pthread_mutex_t m;
};

typedef pthread_spinlock_t spinlock_t;

struct kmem_cache {
	size_t obj_len;
	int live_count;
	void (*ctor)(void *ctx);
};

struct task_struct {
	pthread_t thd;
};

struct disk {
	void* buffer;
	u64 buf_size;
};

struct buffer_head {
	void *b_data;
	u64 b_blocknr;

	/* We need these fields to track and sleep on state bits. */
	int __count;
	u32 __len;
	spinlock_t __lock;
	u32 __locked;
	u32 __dirty;
	u32 __uptodate;
};

struct super_block {
	u32 s_blocksize;
	u32 s_blocksize_bits;
	u32 s_flags;
	u32 s_mode;
	u32 s_magic;
	u64 s_maxbytes;
	u32 s_max_links;
	struct dentry *s_root;
	char s_id[32];
	char s_uuid[16];
	void *s_fs_info;
	struct super_operations *s_op;
	char *sb_options;
	u32 s_time_gran;

	struct disk *__disk;
};

struct inode {
	int i_count;
	u32 i_state;
	u64 i_ino;
	u32 i_nlink;
	u32 i_uid;
	u32 i_gid;
	u32 i_version;
	u32 i_size;
	struct timespec i_atime;
	struct timespec i_mtime;
	struct timespec i_ctime;
	u32 i_blkbits;
	u32 i_blocks;
	u32 i_mode;
	struct super_block *i_sb;
	struct file_operations *i_fop;
	struct inode_operations *i_op;
	void *i_private;
	struct address_space *i_mapping;
	struct list_head i_sb_list;

	/* See fs/inode.c for the inode locking rules. */
	spinlock_t i_lock;

	/* This lock is used to sleep on the I_NEW bit in i_state. */
	struct mutex __lock;

        /* Node in dentry's alias list...I think? Check with Js */
	struct hlist_node i_dentry;

	UT_hash_handle hh;
};

struct qstr {
#define QSTR_INIT(n,l) { { { .len = l } }, .name = n }
	union {
		struct {
			u32 hash;
			u32 len;
		};
		u64 hash_len;
	};
	const char *name;
};

struct dentry {
	int d_flags;
	spinlock_t d_lock;
	struct qstr d_name;
	struct inode *d_inode;
	struct dentry *d_parent;

	/* pointer to superblock for call to fsdb.sb->s_op->statfs(dentry, kstatfs buf)
	 * which uses dentry->d_sb */
	struct super_block *d_sb;

	/* apparently uthash requires string keys to be in the struct itself? */
	char name[80]; // this will be the key to search this dentry's parent's d_child_ht
	struct dentry *d_child_ht;
//	d_child_ht = NULL; // hash child names -> their dentries. *Must* be NULL initialized.

	/* list of alias inodes, for d_instantiate */
        // Is this hlist_head or list_head?
	struct hlist_head d_alias;

	/* dget increases this */
	int refcount;

	UT_hash_handle hh; // make this struct hashable


};
/* vfs.txt: A file object represents a file opened by a process. */
struct file {
	struct inode *i;
	struct file_operations *f_op;
	spinlock_t f_lock;

	/* Added in addition to vedant's suggested file struct */
	struct dentry *d;

	/* For open fd management...*/
	int fd;
	UT_hash_handle hh;
};
/* Stolen from stat.h and statfs.h for do_stat and do_statfs */
// for do_stat let's list ino,nlink, mode, ctime, mtime
struct kstat {
    u64             ino;
    dev_t           dev;
    umode_t         mode;
    unsigned int    nlink;
    dev_t           rdev;
    loff_t          size;
    struct timespec  atime;
    struct timespec mtime;
    struct timespec ctime;
    unsigned long   blksize;
    unsigned long long      blocks;
};

enum {
	BDI_CAP_NO_ACCT_AND_WRITEBACK,
	BDI_CAP_MAP_DIRECT,
	BDI_CAP_MAP_COPY,
	BDI_CAP_READ_MAP,
	BDI_CAP_WRITE_MAP,
	BDI_CAP_EXEC_MAP,
};

struct backing_dev_info {
	const char *name;
	int ra_pages;
	int capabilities;
};

struct address_space {
	struct address_space_operations *a_ops;
	struct backing_dev_info *backing_dev_info;
};

struct address_space_operations {
	int (*readpage)(struct file *, struct page *);
	int (*write_begin)(struct file *, struct address_space *mapping,
                        loff_t pos, unsigned len, unsigned flags,
                        struct page **pagep, void **fsdata);
	int (*write_end)(struct file *, struct address_space *mapping,
                        loff_t pos, unsigned len, unsigned copied,
                        struct page *page, void *fsdata);
	int (*set_page_dirty)(struct page *page);
};

struct super_operations {
        struct inode *(*alloc_inode)(struct super_block *sb);
        void (*destroy_inode)(struct inode *);
        void (*dirty_inode) (struct inode *, int flags);
        int (*write_inode) (struct inode *, int);
        void (*drop_inode) (struct inode *);
        void (*delete_inode) (struct inode *);
        void (*put_super) (struct super_block *);
        int (*sync_fs)(struct super_block *sb, int wait);
        int (*statfs) (struct dentry *, struct kstatfs *);
        int (*remount_fs) (struct super_block *, int *, char *);
        void (*clear_inode) (struct inode *);
        void (*umount_begin) (struct super_block *);
        int (*show_options)(struct seq_file *, struct dentry *);
};

struct inode_operations {
	int (*create) (struct inode *,struct dentry *, umode_t, bool);
	struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
	int (*link) (struct dentry *,struct inode *,struct dentry *);
	int (*unlink) (struct inode *,struct dentry *);
	int (*symlink) (struct inode *,struct dentry *,const char *);
	int (*mkdir) (struct inode *,struct dentry *,umode_t);
	int (*rmdir) (struct inode *,struct dentry *);
	int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
	int (*rename) (struct inode *, struct dentry *,
			struct inode *, struct dentry *);
	int (*readlink) (struct dentry *, char __user *,int);
        void * (*follow_link) (struct dentry *, struct nameidata *);
        void (*put_link) (struct dentry *, struct nameidata *, void *);
	int (*permission) (struct inode *, int);
	int (*get_acl)(struct inode *, int);
	int (*setattr) (struct dentry *, struct iattr *);
	int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
	int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
	ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
	ssize_t (*listxattr) (struct dentry *, char *, size_t);
	int (*removexattr) (struct dentry *, const char *);
	void (*update_time)(struct inode *, struct timespec *, int);
	int (*atomic_open)(struct inode *, struct dentry *, struct file *,
			unsigned open_flag, umode_t create_mode, int *opened);
	int (*tmpfile) (struct inode *, struct dentry *, umode_t);
};

struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	int (*iterate) (struct file *, struct dir_context *);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, loff_t, loff_t, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*readv) (struct file *, const struct iovec *, unsigned long, loff_t *);
	ssize_t (*writev) (struct file *, const struct iovec *, unsigned long, loff_t *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long arg, struct file_lock **);
	long (*fallocate)(struct file *, int mode, loff_t offset, loff_t len);
};

struct file_system_type {
        struct module *owner;
        const char *name;
        int fs_flags;
        struct dentry *(*mount) (struct file_system_type *, int,
                       const char *, void *);
        void (*kill_sb) (struct super_block *);
};

struct fsdb {
	int snap_id;
	char *disk_path;
	char *mnt_opts;
	struct disk disk;
	struct file_system_type *fs_type;
	struct super_block *sb;
	struct dentry *d_root;
};

extern struct fsdb fsdb;

#define atomic_set(p, v) (*(p) = (v))
#define atomic_add_return(v, p) __sync_fetch_and_add(p, v)
#define atomic_inc(p) atomic_add_return(1, p)
#define atomic_sub_return(v, p) __sync_fetch_and_sub(p, v)
#define atomic_dec(p) atomic_sub_return(1, p)
#define test_and_set_bit(v, p) __sync_lock_test_and_set(p, v)

void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

void spin_lock_init(spinlock_t *s);
void spin_lock(spinlock_t *s);
void spin_unlock(spinlock_t *s);

#define NSEC_PER_USEC 1000
struct timespec ns_to_timespec(u64 nsec);

int inode_init_always(struct super_block *sb, struct inode *inode);
struct inode *alloc_inode(struct super_block *sb);
struct inode *new_inode(struct super_block *sb);
u64 get_next_ino();
void inc_nlink(struct inode *inode);
void inode_init_owner(struct inode *inode, const struct inode *dir,
		umode_t mode);
void init_special_inode(struct inode *inode, umode_t mode, dev_t dev);
struct inode *iget_locked(struct super_block *sb, u64 ino);
#define set_nlink(inode, nlink) { inode->i_nlink = (nlink); }
#define i_uid_write(inode, uid) { inode->i_uid = (uid); }
#define i_gid_write(inode, gid) { inode->i_gid = (gid); }
void __iget(struct inode *inode);
void iput(struct inode *inode);
void ihold(struct inode *inode);
struct inode *ilookup(struct super_block *sb, u64 ino);
void unlock_new_inode(struct inode *inode);
void insert_inode_hash(struct inode *inode);

struct buffer_head *sb_bread(struct super_block *sb, u32 blk);
struct buffer_head *sb_getblk(struct super_block *sb, u32 blk);
void sb_breadahead(struct super_block *sb, u32 blk);
void brelse(struct buffer_head *bh);
int buffer_locked(struct buffer_head *bh);
int buffer_dirty(struct buffer_head *bh);
void mark_buffer_dirty(struct buffer_head *bh);
void set_buffer_uptodate(struct buffer_head *bh);
int sync_dirty_buffer(struct buffer_head *bh);
void lock_buffer(struct buffer_head *bh);
void unlock_buffer(struct buffer_head *bh);

#define module_init(f) int (*__init_func__)(void) = f;
#define module_exit(f) void (*__exit_func__)(void) = f;
int register_filesystem(struct file_system_type *fs_type);
void unregister_filesystem(struct file_system_type *fs_type);

void kill_block_super(struct super_block *sb);
void kill_anon_super(struct super_block *sb);
void kill_litter_super(struct super_block *sb);

struct kmem_cache *kmem_cache_create(const char *name, size_t size,
	size_t align, unsigned long flags, void (*ctor)(void *));
void *kmem_cache_alloc(struct kmem_cache *cache, int mask);
void kmem_cache_free(struct kmem_cache *cache, void *p);
void kmem_cache_destroy(struct kmem_cache *cache);

void *kmalloc(size_t n, int mask);
void *kzalloc(size_t n, int mask);
void kfree(void *p);

struct task_struct *kthread_run(int (*f)(void *d), void *d, const char* name);
void kthread_stop(struct task_struct *t);

void printk(const char *fmt, ...);

#define bdi_init(p) 0
#define bdi_destroy(p) {}

#define mapping_set_gfp_mask(map, mask) {}
#define mapping_set_unevictable(map) {}
int simple_readpage(struct file *, struct page *);
int simple_write_begin(struct file *, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned flags,
		struct page **pagep, void **fsdata);
int simple_write_end(struct file *, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned copied,
		struct page *page, void *fsdata);
int __set_page_dirty_no_writeback(struct page *page);

extern struct file_operations simple_dir_operations;
extern struct inode_operations page_symlink_inode_operations;
int page_symlink(struct inode *inode, const char *symname, int len);

int simple_link(struct dentry *,struct inode *,struct dentry *);
int simple_statfs(struct dentry *, struct kstatfs *);
void generic_delete_inode(struct inode *);
int generic_show_options(struct seq_file *, struct dentry *);

struct dentry *simple_lookup(struct inode *,struct dentry *, unsigned int);
int simple_unlink(struct inode *,struct dentry *);
int simple_rmdir(struct inode *,struct dentry *);
int simple_rename(struct inode *, struct dentry *,
		struct inode *, struct dentry *);
int simple_setattr(struct dentry *, struct iattr *);
int simple_getattr(struct vfsmount *mnt, struct dentry *, struct kstat *);

ssize_t generic_file_aio_read(struct kiocb *, const struct iovec *,
		unsigned long, loff_t);
ssize_t generic_file_aio_write(struct kiocb *, const struct iovec *,
		unsigned long, loff_t);
ssize_t do_sync_read(struct file *, char __user *, size_t, loff_t *);
ssize_t do_sync_write(struct file *, const char __user *, size_t, loff_t *);
int generic_file_mmap(struct file *, struct vm_area_struct *);
int noop_fsync(struct file *, loff_t, loff_t, int datasync);
ssize_t generic_file_splice_read(struct file *, struct pipe_inode_info *,
		size_t, unsigned int);
ssize_t generic_file_splice_write(struct pipe_inode_info *, struct file *,
		size_t, unsigned int);
loff_t generic_file_llseek(struct file *, loff_t, int);

struct dentry *d_alloc(struct dentry *parent, const struct qstr *name);
void d_instantiate(struct dentry *d, struct inode *inode);
struct dentry *d_make_root(struct inode *inode);
void dget(struct dentry *d);
void d_genocide(struct dentry *d);

void drop_nlink(struct inode *i);

int dquot_file_open(struct inode *inode, struct file *file);
int generic_file_open(struct inode * inode, struct file * filp);
ssize_t generic_read_dir(struct file *filp, char __user *buf, size_t siz, loff_t *ppos);

extern int (*__init_func__)(void);
extern void (*__exit_func__)(void);

void mount_disk(struct disk *d, char *path);
void unmount_disk(struct disk *d);

struct dentry *mount_bdev(struct file_system_type *fs_type, int flags,
		const char *dev_name, void *data,
		int (*fill)(struct super_block *, void *, int));
struct dentry *mount_nodev(struct file_system_type *fs_type, int flags,
		void *data, int (*fill)(struct super_block *, void *, int));
void save_mount_options(struct super_block *sb, char *options);

#else /* ! _USERSPACE */

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/rbtree.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/printk.h>
#include <linux/quotaops.h>
#include <linux/statfs.h>
#include <linux/kdev_t.h>
//Already included by linux/fs.h?
//#include <asm-generic/atomic.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/dirent.h>

#endif /* _USERSPACE */

u32 __lpfs_fnv(void* val, u32 len);

int lpfs_checksum(void *buf, size_t len, size_t cksum_off);
