/*
 * compat.c
 */

#include "compat.h"

#ifdef _USERSPACE

struct fsdb fsdb;

void mutex_init(struct mutex *m)
{
	pthread_mutex_init(&m->m, NULL);
}

void mutex_lock(struct mutex *m)
{
	pthread_mutex_lock(&m->m);
}

void mutex_unlock(struct mutex *m)
{
	pthread_mutex_unlock(&m->m);
}

void spin_lock_init(spinlock_t *s)
{
	pthread_spin_init(s, PTHREAD_PROCESS_SHARED);
}

void spin_lock(spinlock_t *s)
{
	pthread_spin_lock(s);
}

void spin_unlock(spinlock_t *s)
{
	pthread_spin_unlock(s);
}

struct timespec ns_to_timespec(u64 nsec)
{
	struct timespec ts;
	ts.tv_sec = nsec / 1000000000;
	ts.tv_nsec = nsec % 1000000000;
	return ts;
}

static struct mutex ino_hash_lock;
static struct inode *inode_hashes;

int inode_init_always(struct super_block *sb, struct inode *inode)
{
	inode->i_state = I_NEW;
	inode->i_blkbits = sb->s_blocksize_bits;
	inode->i_sb = sb;
	spin_lock_init(&inode->i_lock);
	return 0;
}

struct inode *alloc_inode(struct super_block *sb)
{
	struct inode *inode;

	if (sb->s_op->alloc_inode)
		inode = sb->s_op->alloc_inode(sb);
	else
		inode = kzalloc(sizeof(struct inode), GFP_KERNEL);


	if (!inode)
		return NULL;

	if (unlikely(inode_init_always(sb, inode))) {
		if (inode->i_sb->s_op->destroy_inode)
			inode->i_sb->s_op->destroy_inode(inode);
		else
			kfree(inode);
		return NULL;
	}

	return inode;
}

struct inode *new_inode(struct super_block *sb)
{
	struct inode *inode = alloc_inode(sb);

	if (inode) {
		spin_lock(&inode->i_lock);
		inode->i_state = 0;
		spin_unlock(&inode->i_lock);
		INIT_LIST_HEAD(&inode->i_sb_list);
	}

	return inode;
}

static u64 last_ino = 2;

u64 get_next_ino()
{
	return atomic_inc(&last_ino);
}

void inc_nlink(struct inode *inode)
{
	atomic_inc(&inode->i_nlink);
}

void inode_init_owner(struct inode *inode, const struct inode *dir,
		umode_t mode)
{
	inode->i_uid = 0;
	if (dir && dir->i_mode & S_ISGID) {
		inode->i_gid = dir->i_gid;
		if (S_ISDIR(mode))
			mode |= S_ISGID;
	} else
		inode->i_gid = 0;
	inode->i_mode = mode;
}

void init_special_inode(struct inode *inode, umode_t mode, dev_t dev)
{
	(void) inode;
	(void) mode;
	(void) dev;
	BUG_ON("Not currently supported.");
}

struct inode *iget_locked(struct super_block *sb, u64 ino)
{
	struct inode *ent = NULL;

	mutex_lock(&ino_hash_lock);
	HASH_FIND(hh, inode_hashes, &ino, sizeof(u64), ent);

	if (ent == NULL) {
		ent = alloc_inode(sb);
		if (!ent) {
			goto exit;
		}

		if (ino > last_ino) {
			atomic_set(&last_ino, ino);
		}

		ent->i_ino = ino;
		HASH_ADD(hh, inode_hashes, i_ino, sizeof(u64), ent);
		mutex_lock(&ent->__lock);
	} else {
		BUG_ON(ent->i_state & I_NEW);
		spin_lock(&ent->i_lock);
		__iget(ent);
		spin_unlock(&ent->i_lock);
	}

exit:
	mutex_unlock(&ino_hash_lock);
	return ent;
}

void __iget(struct inode *inode)
{
	atomic_inc(&inode->i_count);
}

void iput(struct inode *inode)
{
	BUG_ON(atomic_dec(&inode->i_count) < 0);
}

void ihold(struct inode *inode)
{
	BUG_ON(atomic_inc(&inode->i_count) >= 2);
}

struct inode *ilookup(struct super_block *sb, u64 ino)
{
	(void) sb;
	struct inode *ent = NULL;

	mutex_lock(&ino_hash_lock);
	HASH_FIND(hh, inode_hashes, &ino, sizeof(u64), ent);
	mutex_unlock(&ino_hash_lock);

	if (ent) {
		spin_lock(&ent->i_lock);
		__iget(ent);
		spin_unlock(&ent->i_lock);
	}

	return ent;
}

void unlock_new_inode(struct inode *inode)
{
	BUG_ON(!(inode->i_state & I_NEW));
	spin_lock(&inode->i_lock);
	inode->i_state &= ~I_NEW;
	spin_unlock(&inode->i_lock);
	mutex_unlock(&inode->__lock);
}

void insert_inode_hash(struct inode *inode)
{
	struct inode *ent = NULL;
	mutex_lock(&ino_hash_lock);

	HASH_FIND(hh, inode_hashes, &inode->i_ino, sizeof(u64), ent);
	if (ent == NULL) {
		HASH_ADD(hh, inode_hashes, i_ino, sizeof(u64), ent);
	}

	mutex_unlock(&ino_hash_lock);
}

struct buffer_head *sb_bread(struct super_block *sb, u32 blk)
{
	BUG_ON(sb->s_blocksize < 512);
	char *head = ((char *) sb->__disk->buffer) + (blk * sb->s_blocksize);
	struct buffer_head *bh = kmalloc(sizeof(struct buffer_head), GFP_NOIO);
	assert(blk * sb->s_blocksize < sb->__disk->buf_size);

	if (!bh) {
		return NULL;
	}

	bh->b_data = (void *) head;
	bh->b_blocknr = blk;
	bh->__count = 1;
	bh->__len = sb->s_blocksize;
	spin_lock_init(&bh->__lock);
	bh->__locked = 0;
	bh->__dirty = 0;
	bh->__uptodate = 1;
	return bh;
}

struct buffer_head *sb_getblk(struct super_block *sb, u32 blk)
{
	return sb_bread(sb, blk);
}

void sb_breadahead(struct super_block *sb, u32 blk)
{
	(void) sb;
	(void) blk;
}

void brelse(struct buffer_head *bh)
{
	__sync_synchronize();
	int count = atomic_dec(&bh->__count);
	BUG_ON(count < 0);
	if (!count) {
		kfree(bh);
	}
}

int buffer_locked(struct buffer_head *bh)
{
	__sync_synchronize();
	return bh->__locked;
}

int buffer_dirty(struct buffer_head *bh)
{
	__sync_synchronize();
	return bh->__dirty;
}

void mark_buffer_dirty(struct buffer_head *bh)
{
	bh->__dirty = 1;
}

void set_buffer_uptodate(struct buffer_head *bh)
{
	bh->__uptodate = 1;
}

int sync_dirty_buffer(struct buffer_head *bh)
{
	int err;

	__sync_synchronize();
	BUG_ON(!bh->__uptodate);
	BUG_ON(!bh->__locked);

	err = msync(bh->b_data, bh->__len, MS_SYNC);
	if (err == 0) {
		set_buffer_uptodate(bh);
		bh->__dirty = 0;
	}

	return err;
}

void lock_buffer(struct buffer_head *bh)
{
	spin_lock(&bh->__lock);
	bh->__locked = 1;
}

void unlock_buffer(struct buffer_head *bh)
{
	BUG_ON(!bh->__locked);
	spin_unlock(&bh->__lock);
	bh->__locked = 0;
}

int register_filesystem(struct file_system_type *fs_type)
{
	inode_hashes = NULL;
	mutex_init(&ino_hash_lock);

	fsdb.fs_type = fs_type;
	fsdb.fs_type->mount(fsdb.fs_type, fsdb.fs_type->fs_flags, "/dev/sda",
			fsdb.mnt_opts);

	puts("Registered filesystem");
	return 0;
}

void unregister_filesystem(struct file_system_type *fs_type)
{
	fs_type->kill_sb(fsdb.sb);
}

void kill_block_super(struct super_block *sb)
{
	BUG_ON(sb != fsdb.sb);
	if (sb) {
		kfree(sb);
	}
}

void kill_anon_super(struct super_block *sb)
{
	BUG_ON(sb != fsdb.sb);
	kfree(sb->__disk->buffer);
	kfree(sb);
	fsdb.sb = NULL;
}

void kill_litter_super(struct super_block *sb)
{
	if (sb->s_root) {
		d_genocide(sb->s_root);
	}
	kill_anon_super(sb);
}

struct kmem_cache *kmem_cache_create(const char *name, size_t size,
	size_t align, unsigned long flags, void (*ctor)(void *))
{
	struct kmem_cache *k = kzalloc(sizeof(struct kmem_cache), GFP_NOIO);
	if (!k) {
		return k;
	}

	(void) name;
	(void) align;
	(void) flags;
	k->obj_len = size;
	k->live_count = 0;
	k->ctor = ctor;
	return k;
}

void *kmem_cache_alloc(struct kmem_cache *cache, int mask)
{
	void *p = kzalloc(cache->obj_len, mask);
	if (p) {
		if (cache->ctor) {
			cache->ctor(p);
		}
		atomic_inc(&cache->live_count);
	}
	return p;
}

void kmem_cache_free(struct kmem_cache *cache, void *p)
{
	kfree(p);
	atomic_dec(&cache->live_count);
}

void kmem_cache_destroy(struct kmem_cache *cache)
{
	BUG_ON(cache->live_count);
	kfree(cache);
}

void *kmalloc(size_t n, int mask)
{
	(void) mask;
	return malloc(n);
}

void *kzalloc(size_t n, int mask)
{
	(void) mask;
	return calloc(1, n);
}

void kfree(void *p)
{
	free(p);
}

struct task_struct *kthread_run(int (*f)(void *d), void *d, const char* name)
{
	struct task_struct *t = kmalloc(sizeof(struct task_struct), GFP_NOIO);
	if (!t) {
		return NULL;
	}

	int err = pthread_create(&t->thd, NULL, (void *(*)(void *)) f, d);
	if (err) {
		kfree(t);
		return NULL;
	}

	(void) name;

	return t;
}

void kthread_stop(struct task_struct *t)
{
	pthread_kill(t->thd, SIGSTOP);
}

void printk(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

int simple_readpage(struct file *filp, struct page *page)
{
	BUG_ON("We probably should not be here.");
	BUG_ON(filp || page);
	return -1;
}

int simple_write_begin(struct file *filp, struct address_space *map,
		loff_t off, unsigned a, unsigned b,
		struct page **pagep, void **pp)
{
	BUG_ON("We probably should not be here.");
	BUG_ON(filp || map || off || a || b || pagep || pp);
	return -1;
}

int simple_write_end(struct file *filp, struct address_space *map,
		loff_t off, unsigned a, unsigned b,
		struct page *page, void *p)
{
	BUG_ON("We probably should not be here.");
	BUG_ON(filp || map || off || a || b || page || p);
	return -1;
}

int __set_page_dirty_no_writeback(struct page *page)
{
	BUG_ON("We probably should not be here.");
	BUG_ON(page);
	return -1;
}

struct dentry *d_alloc(struct dentry *parent, const struct qstr *name)
{
	struct dentry *d = kzalloc(sizeof(struct dentry), GFP_KERNEL);
	if (!d) {
		return d;
	}

	spin_lock_init(&d->d_lock);
	memcpy(&d->d_name, name, sizeof(struct qstr));
	d->d_parent = (parent == NULL) ? d : parent;
	return d;
}

void d_instantiate(struct dentry *d, struct inode *inode)
{
	BUG_ON(!d);
	BUG_ON(!inode);
	(void) d;
}

struct dentry *d_make_root(struct inode *inode)
{
	struct qstr path = QSTR_INIT("/", 1);
	struct dentry *d = d_alloc(NULL, (const struct qstr *) &path);
	if (!d) {
		iput(inode);
	} else {
		d->d_inode = inode;
		d->d_inode->i_mode |= S_IFDIR;
	}
	d->d_child_ht = NULL;
	// no need to put root dentry in cache, as we already have a direct reference 
	// to it in fsdb.d_root. 
	// As new dentries are created under root, they should be added to the root's d_child_ht

	// for statfs
	// set d_sb so can be able to call d->d_sb->statfs (superblock operation)
	d->d_sb = fsdb.sb;

	return d;
}

void dget(struct dentry *d)
{
	(void) d;
}

void d_genocide(struct dentry *d)
{
	(void) d;
}

void mount_disk(struct disk *d, char *path)
{
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("Could not open device");
		exit(1);
	}

	u64 dev_size;
	if (ioctl(fd, BLKGETSIZE64, &dev_size) == -1) {
		perror("Could not determine device size");
		exit(1);
	}

	printf("Opened a %d byte image.\n", (int) dev_size);

	char *buf = mmap(NULL, dev_size, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED) {
		perror("Could not mmap device");
		exit(1);
	}

	d->buffer = buf;
	d->buf_size = dev_size;
}

void unmount_disk(struct disk *d)
{
	if (msync(d->buffer, d->buf_size, MS_SYNC) != 0) {
		perror("Could not sync changes");
		exit(1);
	}

	if (munmap(d->buffer, d->buf_size) != 0) {
		perror("Could not munmap buffer");
		exit(1);
	}
}

struct dentry *mount_bdev(struct file_system_type *fs_type, int flags,
		const char *dev_name, void *data,
		int (*fill)(struct super_block *, void *, int))
{
	(void) flags;
	(void) fs_type;
	(void) dev_name;

	fsdb.sb = kzalloc(sizeof(struct super_block), GFP_NOIO);
	if (!fsdb.sb) {
		fprintf(stderr, "Failed to alloc super block\n");
		exit(1);
	}

	fsdb.sb->__disk = &fsdb.disk;
	int err = fill(fsdb.sb, data, 0);
	if (err) {
		exit(1);
	}

	fsdb.d_root = fsdb.sb->s_root;
	assert(fsdb.d_root);

	return fsdb.sb->s_root;
}

struct dentry *mount_nodev(struct file_system_type *fs_type, int flags,
		void *data, int (*fill)(struct super_block *, void *, int))
{
	(void) flags;
	(void) fs_type;

	fsdb.sb = kzalloc(sizeof(struct super_block), GFP_NOIO);
	if (!fsdb.sb) {
		fprintf(stderr, "Failed to alloc super block\n");
		exit(1);
	}

	fsdb.sb->__disk = &fsdb.disk;
	fsdb.sb->__disk->buf_size = (1 << 20) * 8;
	fsdb.sb->__disk->buffer = kzalloc(fsdb.sb->__disk->buf_size, GFP_NOIO);
	if (!fsdb.sb->__disk->buffer) {
		fprintf(stderr, "Failed to allocate backing memory\n");
		exit(1);
	}

	int err = fill(fsdb.sb, data, 0);
	if (err) {
		exit(1);
	}

	fsdb.d_root = fsdb.sb->s_root;
	assert(fsdb.d_root);

	return fsdb.sb->s_root;
}

void save_mount_options(struct super_block *sb, char *options)
{
	BUG_ON(sb->sb_options);
	sb->sb_options = options;
}

#else /* ! _USERSPACE */

#ifndef ____ilog2_NaN

/*
 * XXX: This is an evil hack to get around a gcc bug.
 *
 * We occasionally disable optimization via `#pragma GCC optimize ("-O0")' in
 * order to facilitate debugging. Unfortunately, this causes gcc to hide the
 * ____ilog2_NaN symbol, which freaks the linker out ("undefined reference!").
 * The simplest solution here is to.. well.. define the reference.
 *
 * -vk, 02/23/2014
 */

int ____ilog2_NaN(void) /* __attribute__((const, noreturn)) */
{
	for (;;);
}

#endif /* ____ilog2_NaN */

#endif /* _USERSPACE */

u32 __lpfs_fnv(void *val, u32 len)
{
	u8 *p = (u8 *) val;
	unsigned h = 2166136261;
	u32 i;

	for (i = 0; i < len; ++i) {
		h = (h * 16777619) ^ p[i];
	}

	return h;
}

int lpfs_checksum(void *buf, size_t len, size_t cksum_off)
{
	int result;
	u32 computed;
	u32 *p = (u32 *) (((char *) buf) + cksum_off);
	u32 checksum = *p;
	*p = 0;
	computed = __lpfs_fnv(buf, len);
	result = checksum != computed;
	*p = checksum;

	if (result) {
		printk(KERN_WARNING "Checksum mismatch, saw %x, got %x\n",
		       computed, checksum);
	}

	return result;
}
