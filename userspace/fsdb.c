/*
 * fsdb.c
 *
 * A userspace filesystem testing repl
 */

#include <lpfs/compat.h>
#include "uthash.h"
#include <stdio.h>
#include <stdlib.h> // atoi
#include <libgen.h> // for basename, dirname

#include <ctype.h>
#include <string.h>
#include <limits.h>
#define ERROR -1
#define SUCCESS 0

/* Delimiter for command parsing */
static const char *WHITESPACE_DELIM = " \f\n\r\t\v";
/* Delimiter for directory parsing */
static const char *SLASH_DELIM = "/";
/* Assign fd's sequentially */
static int next_fd = 1;
/* Keep track of open files */
struct file *fd_ht;

/*
 * Test command parsing. Parsing on spaces, form-feed ('\f'), newline ('\n'),
 * carriage return ('\r'), horizontal tab ('\t'), and vertical tab ('\v').
 *
 * Parameters:
 *     cmd      - Input command string.
 *     expmin_argc - Expected minimum argument count.
 *     expmax_argc - Expected maximum argument count.
 * Returns: SUCCESS if able to be parsed correctly, ERROR if something went
 *          wrong (i.e. wrong number of arguments for command).
 */
int check_cmd(char *orig_cmd, int expmin_argc, int expmax_argc) {
        char *cmd;
        char *token;
        char *cmd_name;
        int cmd_argc = 0;

        /* Sanity checks */
        if (orig_cmd == NULL) {
                printf("Input is NULL\n");
                return ERROR;
        }
        if (expmin_argc <= 0) {
                printf("expmin_argc %d is non-positive\n", expmin_argc);
                return ERROR;
        }
        if (expmax_argc > 7) {
                printf("expmax_argc %d is too large\n", expmax_argc);
                return ERROR;
        }
        if (expmin_argc > expmax_argc) {
                printf("Invalid range: (%d, %d)\n", expmin_argc, expmax_argc);
                return ERROR;
        }

        /* Copy cmd string to prevent modifying original string. */
        cmd = strdup(orig_cmd);
        if (cmd == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return ERROR;
        }

        /* Start parsing/tokenizing */
        token = strtok(cmd, WHITESPACE_DELIM);
        cmd_name = token;
        while (token != NULL && cmd_argc <= expmax_argc) {
                cmd_argc++;
                token = strtok(NULL, WHITESPACE_DELIM);
        }

        /* Check argument counts */
        if (cmd_argc > expmax_argc) {
                printf("Too many arguments for cmd: '%s'\n", cmd_name);
                free(cmd);
                return ERROR;
        }
        if (cmd_argc < expmin_argc) {
                printf("Too few arguments for cmd: '%s'\n", cmd_name);
                free(cmd);
                return ERROR;
        }

        free(cmd);
        return SUCCESS;
}

/*
 * Test string to int conversion.
 *
 * Parameters:
 *     intstr      - Input string representation of integer.
 * Returns: SUCCESS if able to be convert correctly, ERROR if something went
 *          wrong (i.e. overflow or not a number).
 */
int check_atoi(char *orig_intstr) {
        char *intstr;
        char *endptr;
        long int result;

        /* Sanity checks */
        if (orig_intstr == NULL) {
                printf("intstr is NULL\n");
                return ERROR;
        }
        if (strlen(orig_intstr) == 0) {
                printf("intstr is empty\n");
                return ERROR;
        }

        /* Copy cmd string to prevent modifying original string. */
        intstr = strdup(orig_intstr);
        if (intstr == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return ERROR;
        }

        /* Start converting and checking */
        result = strtol(intstr, &endptr, 10);
        if (*endptr != '\0') {
                printf("intstr '%s' is not a valid number\n", intstr);
                free(intstr);
                return ERROR;
        }
        if (result >= INT_MAX) {
                printf("intstr value '%s' too large\n", intstr);
                free(intstr);
                return ERROR;
        }
        if (result <= INT_MIN) {
                printf("intstr value '%s' too small\n", intstr);
                free(intstr);
                return ERROR;
        }

        free(intstr);
        return SUCCESS;
}

/* return dentry of last component of the given path */
/* so....basically the parent dentry */
struct dentry *dentry_lookup(char *orig_path) {
        char *path;
        char *token; 
        struct dentry *d_tmp;      // temp ptr for parent dentry hash lookups
        struct dentry *d = fsdb.d_root;      // begin with the root dentry

        /* Sanity checks */
        if (orig_path == NULL) {
                printf("path is NULL\n");
                return NULL;
        }

        /* Special case - PWD = ROOT */
        if (strcmp(orig_path, ".") == 0) {
                return d;
        }

        /* Copy path to prevent modifying original string. */
        path = strdup(orig_path);
        if (path == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return NULL;
        }

        token = strtok(path, SLASH_DELIM);
        while (token != NULL && d != NULL) { // finish parsing or invalid path
                /* Look up child dentry by name (token), and fill in d_tmp */
                HASH_FIND_STR(d->d_child_ht, token, d_tmp); 
                d = d_tmp;                   // new parent dentry
                token = strtok(NULL, SLASH_DELIM);
        }

        free(path);
        return d;                            // NULL if invalid path!
}

int create_dentry(char *orig_path, umode_t mode, struct file *file) {
        char *path_basecopy;
        char *path_dircopy;
        char *new_dir_name;
        char *rest_of_path;
        struct dentry *parent_dentry;
        struct dentry *new_dentry;
        struct inode_operations *parent_i_op;
        struct inode *parent_d_inode;
        struct qstr p;

        /* Sanity checks */
        if (orig_path == NULL) {
                printf("path is NULL\n");
                return ERROR;
        }
        if (dentry_lookup(orig_path) != NULL) {
                puts("path already exists, cannot create inode");
                return ERROR;
        }

        /* Copy path to prevent modifying original string. */
        path_basecopy = strdup(orig_path);
        if (path_basecopy == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return ERROR;
        }
        path_dircopy = strdup(orig_path);
        if (path_dircopy == NULL) {
                printf("Fatal error: Insufficient memory\n");
                free(path_basecopy);
                return ERROR;
        }

        new_dir_name = strdup(basename(path_basecopy));
        // This is not to be freed if successful (keep in new dentry
        if (new_dir_name == NULL) {
                printf("Fatal error: Insufficient memory\n");
                free(path_basecopy);
                free(path_dircopy);
                return ERROR;
        }
        rest_of_path = dirname(path_dircopy);
        printf("basename of path: %s\n", new_dir_name);
        printf("dirname of path: %s\n", rest_of_path);

        parent_dentry = dentry_lookup(rest_of_path);
        if (parent_dentry == NULL) {
                puts("Error: Invalid path.");
                free(path_basecopy);
                free(path_dircopy);
                free(new_dir_name);
                return ERROR;
        }
        if ((parent_dentry->d_inode->i_mode & S_IFMT) != S_IFDIR) {
                puts("Error: Invalid path.");
                free(path_basecopy);
                free(path_dircopy);
                free(new_dir_name);
                return ERROR;
        }
        parent_d_inode = parent_dentry->d_inode;
        parent_i_op = parent_dentry->d_inode->i_op;
        p = (struct qstr)QSTR_INIT(new_dir_name, sizeof(new_dir_name));

        /* Do this check early, so as to not rely on d_genocide or whatever */
        if (mode != S_IFDIR && mode != S_IFREG) {
                puts("Mode not recognized.");
                free(path_basecopy);
                free(path_dircopy);
                free(new_dir_name);
                return ERROR;
        }

        /* Now with the parent dentry, construct one for new subdirectory. */
        new_dentry = d_alloc(parent_dentry, (const struct qstr *) &p);
        if (new_dentry == NULL) {
                printf("Fatal error: Insufficient memory\n");
                free(path_basecopy);
                free(path_dircopy);
                free(new_dir_name);
                return ERROR;
        }
        new_dentry->d_child_ht = NULL;      // must be NULL initialized
        //new_dentry->name = new_dentry->d_name.name;  // p.name;
        strcpy(new_dentry->name, new_dentry->d_name.name);
        // Do a strcpy because .name is an array, not a pointer.
        // careful here...we use this as the key to parent_dentry->d_child_ht.
        // Also need to free later on when deleting dentry? d_genocide?

        if (mode == S_IFDIR) {
                // This should call ramfs_mkdir.
                // see man 2 stat for modes and macros for checking modes
                parent_i_op->mkdir(parent_d_inode, new_dentry, S_IFDIR);
                // screw permissions and just pass type of file for mode param
                // If success new_dentry should be instantiated with new inode
        } else if (mode == S_IFREG) {
                parent_i_op->create(parent_d_inode, new_dentry, S_IFREG, true);
                // screw permissions and just pass type of file for mode param
                file->i = new_dentry->d_inode;
                file->d = new_dentry;
        }

        HASH_ADD_STR(parent_dentry->d_child_ht, name, new_dentry);
        free(path_basecopy);
        free(path_dircopy);
        // Do NOT free new_dir_name. Keep in new_dentry->d_name.
        return SUCCESS;
}

static void usage()
{
        puts("Usage: fsdb /dev/<disk> [ snapshot=<snap_id> | -ramfs ]");
        exit(1);
}

// QUESTION: Where to create inode? Called by inode_ops
// QUESTION2: Basename and dirname.
// QUESTION3: Unlink fd or path?

void do_ls(char *cmd)
{
        // |fsdb> ls <path>
        char *cmd_name;
        char *path;
        struct dentry *d;
        struct dentry *item_ptr; // for HASH_ITER: actual child dentry returned
        struct dentry *d_tmp;    // for HASH_ITER: just used as a cursor
        char cmd_alt[] = "ls /";
        int has_children = 0;
        if (strcmp(cmd, "ls\n") == 0) {
                cmd = cmd_alt;
        }
        if (check_cmd(cmd, 2, 2) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        path = strtok(NULL, WHITESPACE_DELIM);
        (void)cmd_name;

        // get path dentry
        // if dentry->mode is file just print the file name
        // if mode is directory hash_itr over d_child_ht and print their names
        d = dentry_lookup(path);
        if (d == NULL) {
                puts("Invalid path");
                return;
        }

        switch(d->d_inode->i_mode & S_IFMT) {
        case S_IFREG:
                puts(path);
                break;
        case S_IFDIR:
                puts("do_ls: path is a directory, listing children");
                HASH_ITER(hh, d->d_child_ht, item_ptr, d_tmp) {
                        puts(item_ptr->name);
                        has_children = 1;
                }
                if (has_children == 0) {
                        puts("<No files or subdirectories exist.>");
                }
                break;
        default:
                puts("do_ls: Cannot determine type of path");
                break;
        }
}

void do_open(char *cmd)
{
        // |fsdb> open <path>
        char *cmd_name;
        char *path;
        struct dentry *file_dentry;
        struct file *open_file;
        if (check_cmd(cmd, 2, 2) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        path = strtok(NULL, WHITESPACE_DELIM);
        (void)cmd_name;

        open_file = (struct file *)kzalloc(sizeof(struct file), 0);
        if (open_file == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return;
        }

        // get the dentry of the path
        file_dentry = dentry_lookup(path);
        if (file_dentry == NULL) {
                // create_dentry also sets the two fields below.
                if (create_dentry(path, S_IFREG, open_file) == ERROR) {
                        printf("Failed to open new file %s.\n", path);
                        kfree(open_file);
                        return;
                }
        } else {
                open_file->i = file_dentry->d_inode;
                open_file->d = file_dentry;
        }
        open_file->fd = next_fd++;
        //hash_add_int(head, keyfield name, value ptr)
        //why does the key HAVE to be in the struct being added to hashtable?
        HASH_ADD_INT(fd_ht, fd, open_file);  // last arg (value) has to be ptr
        printf("Open ");
        if (file_dentry == NULL) {
                printf("new ");
        }
        printf("file '%s' with fd %d\n", path, open_file->fd);
}

void do_mkdir(char *cmd)
{
        // |fsdb> mkdir <path>
        char *cmd_name;
        char *path;
        if (check_cmd(cmd, 2, 2) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        path = strtok(NULL, WHITESPACE_DELIM);
        (void)cmd_name;

        if (create_dentry(path, S_IFDIR, NULL) == ERROR) {
                puts("Could not create new directory.");
                return;
        }
        puts("SUCCESSFULLY PUT NEW DENTRY");
}

void do_read(char *cmd)
{
        // |fsdb> read <fd> <size> <off> [<hostpath>]
        char *cmd_name;
        char *fd_char;
        char *size_char;
        char *offset_char;
        char *hostpath;
        int fd;
        int size;
        int offset;
        struct file *f;
        if (check_cmd(cmd, 4, 5) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        fd_char = strtok(NULL, WHITESPACE_DELIM);
        size_char = strtok(NULL, WHITESPACE_DELIM);
        offset_char = strtok(NULL, WHITESPACE_DELIM);
        hostpath = strtok(NULL, WHITESPACE_DELIM);
        if (check_atoi(fd_char) == ERROR) {
                return;
        }
        if (check_atoi(size_char) == ERROR) {
                return;
        }
        if (check_atoi(offset_char) == ERROR) {
                return;
        }
        fd = atoi(fd_char);
        size = atoi(size_char);
        offset = atoi(offset_char);
        (void)cmd_name;

        HASH_FIND_INT(fd_ht, &fd, f);
        puts("Not implemented.");
        (void)offset; (void)size; (void)hostpath;
        //do_sync_read(struct file *filp, , char __user *buf, 
        //                                       ssize_t len,  loff_t *pos);
        // so some sort of fd->file mapping

        //for nonexistent files, create them?
        //ramfs_mknod(inode of parent dir, dentry to fill, mode, 0);
}

void do_write(char *cmd)
{
        // |fsdb> write <fd> <size> <off> [<hostpath>] [rand | zero]
        char *cmd_name;
        char *fd_char;
        char *size_char;
        char *offset_char;
        char *hostpath;
        char *rand_or_zero;
        int fd;
        int size;
        int offset;
        struct file *f;
        if (check_cmd(cmd, 4, 6) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        fd_char = strtok(NULL, WHITESPACE_DELIM);
        size_char = strtok(NULL, WHITESPACE_DELIM);
        offset_char = strtok(NULL, WHITESPACE_DELIM);
        hostpath = strtok(NULL, WHITESPACE_DELIM);
        rand_or_zero = strtok(NULL, WHITESPACE_DELIM);
        if (check_atoi(fd_char) == ERROR) {
                return;
        }
        if (check_atoi(size_char) == ERROR) {
                return;
        }
        if (check_atoi(offset_char) == ERROR) {
                return;
        }
        fd = atoi(fd_char);
        size = atoi(size_char);
        offset = atoi(offset_char);
        (void)cmd_name;

        HASH_FIND_INT(fd_ht, &fd, f);
        if (f == NULL) {
                puts("do_write: Invalid file descriptor");
        } else {
                printf("do_write: File '%s'\n", f->d->d_name.name);
        }
        puts("Not implemented.");
        (void)offset; (void)size; (void)hostpath; (void)rand_or_zero;
}

void do_unlink(char *cmd)
{
        // |fsdb> unlink <fd>
        char *cmd_name;
        char *path;
        char *path_basecopy;
        char *path_dircopy;
        char *file_to_remove;
        char *rest_of_path;
        struct dentry *parent_dentry;
        struct dentry *d_tmp;
        //char *fd_char;
        //int fd;
        if (check_cmd(cmd, 2, 2) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        path = strtok(NULL, WHITESPACE_DELIM);
        //fd_char = strtok(NULL, WHITESPACE_DELIM);
        //if (check_atoi(fd_char) == ERROR) {
        //        return;
        //}
        //fd = atoi(fd_char);
        (void)cmd_name;

        puts("Not implemented.");
        puts("1) Is unlink <fd> or unlink <path>?");
        puts("2) Find out i_nlink problem (files with simple_unlink");
        return;

        path_basecopy = strdup(path);
        if (path_basecopy == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return;
        }
        path_dircopy = strdup(path);
        if (path_dircopy == NULL) {
                printf("Fatal error: Insufficient memory\n");
                free(path_basecopy);
                return;
        }

        file_to_remove = basename(path_basecopy);
        rest_of_path = dirname(path_dircopy);
        parent_dentry = dentry_lookup(rest_of_path);
        if (parent_dentry == NULL) {
                puts("Error: Invalid path");
                free(path_basecopy);
                free(path_dircopy);
                return;
        }

        HASH_FIND_STR(parent_dentry->d_child_ht, file_to_remove, d_tmp);
        if (d_tmp == NULL) { // couldn't find that file in the parent dir
                puts("Could not delete requested file, it does not exist");
                free(path_basecopy);
                free(path_dircopy);
                return;
        }

        // parent directory inode is only needed to set inode's ctime and mtime
        // d_tmp is the dentry associated with the file we want to unlink
        parent_dentry->d_inode->i_op->unlink(parent_dentry->d_inode, d_tmp);
        HASH_DEL(parent_dentry->d_child_ht, d_tmp);
        free(path_basecopy);
        free(path_dircopy);
        return;
}

void do_truncate(char *cmd)
{
        // |fsdb> truncate <fd> <len>
        (void) cmd;
        puts("Not implemented.");
}

void do_flush(char *cmd)
{
        // |fsdb> flush <fd>
        (void) cmd;
        puts("Nothing happened.");
        // should call noop_fsync
}

void do_rename(char *cmd)
{
        // |fsdb> rename <fd> <path>
        char *cmd_name;
        char *fd_char;
        char *path;
        int fd;
        struct file *f;
        if (check_cmd(cmd, 3, 3) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        fd_char = strtok(NULL, WHITESPACE_DELIM);
        path = strtok(NULL, WHITESPACE_DELIM);
        if (check_atoi(fd_char) == ERROR) {
                return;
        }
        fd = atoi(fd_char);
        (void)cmd_name;

        HASH_FIND_INT(fd_ht, &fd, f);
        if (f == NULL) {
                puts("do_stat: Invalid file descriptor");
                return;
        }

        puts("The below is not complete at all.");
        //new inode: (needed?) struct inode *i = NULL;
        //new dentry
        //if it is dir, inode->i_op is ramfs_dir_inode_operations
        //simple_rename is only a valid op defined for *directories*

        struct dentry *new = dentry_lookup(path);
        if (new == NULL) {  // If new name available.
                // not supported..fail for rest.
                puts("Returning because of error");
                puts("Not implmented.");
                return;
        }

        if (S_ISDIR(f->i->i_mode)){
                if (!f->i->i_op->rename(f->i, f->d,new->d_inode,new)) {
                        puts("OK");
                        return;
                }
                else {
                        puts("not OK");
                }
        }
}

void do_stat(char *cmd)
{
        // |fsdb> stat <fd>
        char *cmd_name;
        char *fd_char;
        int fd;
        struct file *f;
        char *file_type;
        if (check_cmd(cmd, 2, 2) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        fd_char = strtok(NULL, WHITESPACE_DELIM);
        if (check_atoi(fd_char) == ERROR) {
                return;
        }
        fd = atoi(fd_char);
        (void)cmd_name;

        HASH_FIND_INT(fd_ht, &fd, f);
        if (f == NULL) {
                puts("do_stat: Invalid file descriptor");
                return;
        }

        printf("File: '%s'\n", f->d->d_name.name);
        printf("Size: %d\n", f->i->i_size);
        printf("Blocks: %d\n", f->i->i_blocks);
        printf("IO Block: Unimplemented\n");
        printf("Link count: %d\n", f->i->i_nlink);
        switch (f->i->i_mode & S_IFMT) {
        default:
                file_type = "unknown file type";
                break;
        case S_IFREG:
                file_type = "regular file";
                break;
        case S_IFDIR:
                file_type = "directory";
                break;
        case S_IFLNK:
                file_type = "symbolic link";
                break;
        }
        printf("File type: '%s'\n", file_type);
        // TODO more print stuff?
}

void do_statfs(char *cmd)
{
        // |fsdb> statfs
        (void) cmd;

        struct kstatfs *buf;
        buf = (struct kstatfs *)kzalloc(sizeof(struct kstatfs), 0);
        if (buf == NULL) {
                printf("Fatal error: Insufficient memory\n");
                return;
        }
        fsdb.sb->s_op->statfs(fsdb.d_root, buf);
        printf("f_type: %lu\n", buf->f_type);
        printf("f_bsize: %lu\n", buf->f_bsize);
        printf("f_namelen: %lu\n", buf->f_namelen);
        puts("");
        kfree(buf);    // Used kzalloc above.
}

void do_close(char *cmd)
{
        // |fsdb> close <fd>
        char *cmd_name;
        char *fd_char;
        int fd;
        struct file *f;
        if (check_cmd(cmd, 2, 2) == ERROR) {
                return;
        }
        cmd_name = strtok(cmd, WHITESPACE_DELIM);
        fd_char = strtok(NULL, WHITESPACE_DELIM);
        if (check_atoi(fd_char) == ERROR) {
                return;
        }
        fd = atoi(fd_char);
        (void)cmd_name;

        HASH_FIND_INT(fd_ht, &fd, f);
        if (f == NULL) {
                puts("do_close: Invalid file descriptor");
                return;
        }

        //will this work? 
        //do I need to pass in orig pointer I had in HASH_ADD_INT???
        HASH_DEL(fd_ht, f);
        kfree(f);      // Used kzalloc above.
        puts("Deleted fd from fd_ht");
}

void do_exit(char *cmd)
{
        (void) cmd;
        // Do nothing. main() handles this.
}

static struct {
        const char *name;
        void (*handler)(char *cmd);
} commands[] = {
        { "ls", do_ls },
        { "open", do_open },
        { "mkdir", do_mkdir },
        { "read", do_read },
        { "write", do_write },
        { "unlink", do_unlink },
        { "truncate", do_truncate },
        { "flush", do_flush },
        { "rename", do_rename },
        { "statfs", do_statfs },
        { "stat", do_stat },
        { "close", do_close },
        { "exit", do_exit },
};

static int repl()
{
        size_t cmd_len;
        char *cmd = NULL;
        int err = 0;
        int matched = 0;

        printf("|fsdb> ");

        err = (int) getline(&cmd, &cmd_len, stdin);
        if (err == -1) {
                return err;
        }

        u32 i;
        err = 0;
        for (i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i) {
                const char *needle = commands[i].name;
                if (strstr(cmd, needle) != NULL) {
                        matched = 1;
                        commands[i].handler(cmd);

                        if (strcmp(needle, "exit") == 0) {
                                err = 1;
                                goto exit;
                        }

                        break;
                }
        }

        if (!matched) {
                puts("No matching commands.");
        }
        puts("");
exit:
        free(cmd);
        return err;
}

void init_lpfs(void)
{
        int err;

        // should call module_init and mount_nodev for ramfs....
        err = __init_func__();
        if (err) {
                fprintf(stderr, "Failed to initialize lpfs\n");
                exit(1);
        }
}

void exit_lpfs(void)
{
        __exit_func__();
}

int main(int argc, char **argv)
{
        if (argc != 3) {
                usage();
        }

        memset(&fsdb, 0, sizeof(struct fsdb));

        fsdb.disk_path = argv[1];
        fsdb.mnt_opts = argv[2];

        if (strstr(fsdb.mnt_opts, "snapshot")) {
                if (sscanf(fsdb.mnt_opts, "snapshot=%d", &fsdb.snap_id) != 1) {
                        usage();
                }
                mount_disk(&fsdb.disk, fsdb.disk_path);
        } else if (strstr("-ramfs", fsdb.mnt_opts)) {
                /* No setup needed here. */
        } else {
                usage();
        }

        init_lpfs();

        for (;;) {
                if (repl()) break;
        }

        exit_lpfs();

        if (fsdb.disk.buffer) {
                if (strstr(fsdb.mnt_opts, "snapshot")) {
                        unmount_disk(&fsdb.disk);
                } else if (strstr("-ramfs", fsdb.mnt_opts)) {
                        /* No unsetup needed here. */
                        /* Can free, but unnecessary since end of program */
                        //kfree(fsdb.sb->__disk->buffer);
                }
        }

        return 0;
}
