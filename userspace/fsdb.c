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
#define MAX_CMD_ARGC 7
#define ERROR -1
#define SUCCESS 0

/* Assign fd's sequentially */
static int next_fd = 0;
/* Keep track of open files */
struct file *fd_ht;

/*
 * Parses the command and extracts the input arguments.
 *
 * Parameters:
 *     cmd      - Input command string.
 *     exp_argc - Expected argument count.
 * Returns: SUCCESS if parsed correctly, ERROR if something went wrong
 *          (i.e. wrong number of arguments for command).
 */

char *parse_cmd(char *cmd, int exp_argc) {
	char *cmd_argv[MAX_CMD_ARGC];
        int i;
        int cmd_argc = 0;

        /* Sanity checks */
        if (cmd == NULL) {
                printf("Input is NULL\n");
                return NULL;
        }
        if (exp_argc <= 0) {
                printf("Expected arg count %d is non-positive\n", exp_argc);
                return NULL;
        }
        if (exp_argc > MAX_CMD_ARGC) {
                printf("Expected arg count %d is too large\n", exp_argc);
                return NULL;
        }

        /* Clear command argument vector */
        for (i = 0; i < MAX_CMD_ARGC; i++) {
                cmd_argv[i] = NULL;
        }

        /* Clear spaces before and after command */
        while (strlen(cmd) > 0 && isspace(cmd[0])) {
                cmd++;
        }
        while (strlen(cmd) > 0 && isspace(cmd[strlen(cmd) - 1])) {
                cmd[strlen(cmd) - 1] = '\0';
        }
        if (strlen(cmd) == 0) {
                printf("Input is empty\n");
                return NULL;
        }

        while (cmd != NULL && cmd_argc < MAX_CMD_ARGC) {
                /* Get current command arg */
                cmd_argv[cmd_argc++] = cmd;

                /* Find placement for next command arg */
                cmd = strstr(cmd+1, " ");
                if (cmd != NULL) {  /* ...if there exists another arg */
                        while (strlen(cmd) > 0 && isspace(cmd[0])) {
                                *(cmd++) = '\0';
                        }
                        if (strlen(cmd) == 0) {
                                /* This should not happen as there should be
                                   another non-space argument after this */
                                printf("Logic error: missing next argument\n");
                                exit(1);
                        }
                }
        }
        if (cmd_argc <= 0) {
                /* There should be at least one argument if no blank input */
                printf("Logic error: illogical arg count %d\n", cmd_argc);
                exit(1);
        }
        if (cmd != NULL) {
                /* Too many args to fit into argv, so didn't parse all. */
                printf("Too many arguments for cmd: '%s'\n", cmd_argv[0]);
                return NULL;
        }
        if (cmd_argc > exp_argc) {
                printf("Too many arguments for cmd: '%s'\n", cmd_argv[0]);
                return NULL;
        }
        if (cmd_argc < exp_argc) {
                printf("Too few arguments for cmd: '%s'\n", cmd_argv[0]);
                return NULL;
        }

	//        return SUCCESS;
	return strdup(cmd_argv[1]);
}

/* return dentry of last component of the given path */
/* so....basically the parent dentry */
struct dentry *dentry_lookup(char *path) {
	/*
    	   if (strcmp((path+(strlen(path)-1)), "/") == 0) {
    	   puts("It ends with a /");
    	 *(path+strlen(path)-1) = '\0';
    	 }
    	 puts(path);
    	 */


    	char *token; 
    	struct dentry *d_tmp; // temp placeholder for parent dentry hash lookups
    	struct dentry *d = fsdb.d_root; // begin with the root dentry

    	char *tokenize_string = strdup(path);
    	char delim[2] = "/";
    	token = strtok(tokenize_string, "/");

    	while (token != NULL) {
		/* Look up child dentry by name (token), and fill in d_tmp */    	
    		HASH_FIND_STR(d->d_child_ht, token, d_tmp); 
    		if (d_tmp == NULL) {
    	        	puts("returning null");
    			return NULL; // invalid path!
    		}
    		d = d_tmp; // new parent dentry

		token = strtok(NULL, delim);
    	}
    	return d;
}


static void usage()
{
	puts("Usage: fsdb /dev/<disk> [ snapshot=<snap_id> | -ramfs ]");
	exit(1);
}

void do_ls(char *cmd)
{
	// |fsdb> ls <path>
	(void) cmd;
	// get path dentry
	// if dentry->mode is file just print the file name
	// if mode is directory hash_itr over d_child_ht and print their names

        char *path;
        int cmd_argc = 2;
        path = parse_cmd(cmd,cmd_argc);

        if (path == NULL) {
            	puts("do_ls: Could not parse cmd");
            	return;
        }


        struct dentry *d;
        d = dentry_lookup(path);
        if (d == NULL) {
        	puts("Invalid path");
        	return;
        }

        /* gcc complains if I put these declarations in case S_IFDIR, 
         * so I guess I'll put them out here...
         */
        // for HASH_ITER
        //item_ptr is the actual child dentry returned
        //d_tmp is just used as a cursor.
        struct dentry *item_ptr;
        struct dentry *d_tmp;

        switch(d->d_inode->i_mode & S_IFMT) {
            	case S_IFREG:
            		puts(path);
            		break;
            	case S_IFDIR:
            		puts("do_ls: path is a directory, listing children");
            		HASH_ITER(hh, d->d_child_ht, item_ptr, d_tmp){
            	    		puts(item_ptr->name);
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
	(void) cmd;

        char *path;
        int cmd_argc = 2;
        path = parse_cmd(cmd,cmd_argc);

        // get the dentry of the path
        struct dentry *file_dentry;
        file_dentry = dentry_lookup(path);
        if (file_dentry == NULL) {
        	puts("Error: Invalid path or file");
        }
        // construct a struct file
        struct file open_file = {
            	.i = file_dentry->d_inode,
            	.d = file_dentry,
            	.fd = next_fd++
        };

	//hash_add_int(head, keyfield name, value ptr)
	//why does the key HAVE to be in the struct being added to the hashtable?
        HASH_ADD_INT(fd_ht, fd, &open_file); // last arg (value) has to be a ptr

}

void do_mkdir(char *cmd)
{
	// |fsdb> mkdir <path>
	(void) cmd;

	/* Path parsing */
        char *path;
        int cmd_argc = 2;
        path = parse_cmd(cmd,cmd_argc);

        // before we do anything else 

        struct dentry *check_exist;
	check_exist = dentry_lookup(path); 
        if (check_exist != NULL) {
       	 	puts("do_mkdir: path already exists, cannot mkdir");
       	 	return;
        }

     	char *new_dir_name = strdup(basename(path));
     	char *rest_of_path = strdup(dirname(path));
     	puts("basename of path:");
     	puts(new_dir_name);
     	puts("dirname of path:");
     	puts(rest_of_path);


        struct dentry *parent_dentry;
        parent_dentry = dentry_lookup(rest_of_path);
        if (parent_dentry == NULL) {
            	puts("Error: Invalid path.");
            	return;
        }


	/* Now that we have the parent dentry, construct one for the new subdirectory
	 * we are making. 
	 */
	struct qstr p = QSTR_INIT(new_dir_name, sizeof(new_dir_name));
        struct dentry *new_dir_dentry = d_alloc(NULL, (const struct qstr *) &p);
        new_dir_dentry->d_parent = parent_dentry;
	//new_dir_dentry->name = new_dir_name; // careful here...we use this as the key to parent_dentry->d_child_ht.
	strcpy(new_dir_dentry->name,new_dir_name);

	new_dir_dentry->d_child_ht = NULL; // must be NULL initialized
	// This should call ramfs_mkdir.
	// see man 2 stat for modes and macros for checking modes
        parent_dentry->d_inode->i_op->mkdir(parent_dentry->d_inode, new_dir_dentry, S_IFDIR);  // screw permissions and just pass in type of file for mode param
        // If successful new_dir_dentry should have been instantiated with a new inode...

	//	HASH_ADD_KEYPTR(hh, hashtable location, string pointer as key(the child name), sizeof(new_dir_name), the value the key hashes to(struct dentry));
	//	HASH_ADD_KEYPTR(hh, parent_dentry->d_child_ht, new_dir_dentry->name, sizeof(new_dir_dentry->name), new_dir_dentry);
	HASH_ADD_STR(parent_dentry->d_child_ht, name, new_dir_dentry);

	struct dentry *temp = NULL;
	HASH_FIND_STR(parent_dentry->d_child_ht, new_dir_dentry->name, temp);
	if (temp != NULL) {
	    	puts("SUCCESSFULLY PUT NEW DENTRY");
	} else {
	    	puts("Could not find new directory we just made...");
	}
}

void do_read(char *cmd)
{
	// |fsdb> read <fd> <size> <off> [<hostpath>]
	(void) cmd;
	puts("Not implemented.");
	//do_sync_read(struct file *filp, , char __user *buf, ssize_t len,  loff_t *pos);
	// so some sort of fd->file mapping	

	//for nonexistent files, create them? 
	//ramfs_mknod(inode of parent dir, dentry to fill, mode, 0);

}

void do_write(char *cmd)
{
	// |fsdb> write <fd> <size> <off> [<hostpath>] [rand | zero]
	(void) cmd;
	puts("Not implemented.");
}

void do_unlink(char *cmd)
{
	// |fsdb> unlink <fd>
	(void) cmd;

        char *path;
        int cmd_argc = 2;
        path = parse_cmd(cmd,cmd_argc);

     	char *file_to_remove = strdup(basename(path));
     	char *rest_of_path = strdup(dirname(path));

        struct dentry *parent_dentry;
        struct dentry *d_tmp;

        parent_dentry = dentry_lookup(rest_of_path);
        if (parent_dentry == NULL) {
            	puts("Error: Invalid path");
            	return;
        }
    	HASH_FIND_STR(parent_dentry->d_child_ht, file_to_remove, d_tmp); 
    	if (d_tmp == NULL) { // couldn't find that file in the parent dir 
    	    	puts("Could not delete requested file, it does not exist");
    	} else {
    	   	// parent directory inode is only needed to set inode's ctime and mtime
    	   	// d_tmp is the dentry associated with the file we want to unlink
    	   	parent_dentry->d_inode->i_op->unlink(parent_dentry->d_inode, d_tmp);
    	}
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
	puts("Not implemented.");
	// should call noop_fsync
}

void do_rename(char *cmd)
{
	// |fsdb> rename <fd> <path>
	(void) cmd;
	puts("Not implemented.");
	/* hash_find fd_ht with given fd -> returns file struct
	 * get file->d->d_parent (dentry of parent)
	 * wait...rename file at fd to path???
	 */

}

void do_stat(char *cmd)
{
	// |fsdb> stat <fd>
	(void) cmd;

        char *char_fd;
        int fd_key;
        int cmd_argc = 2;
        char_fd = parse_cmd(cmd,cmd_argc);
        fd_key = atoi(char_fd);

	struct file *f;
	HASH_FIND_INT(fd_ht, &fd_key, f);
	if (f == NULL) {
		puts("do_stat: Invalid file descriptor");
	} else {
	    	// something like....
	    	// f->i->i_op->getattr(vfsmount, inode, kstat k);
	    	// then print kstat struct?
	    	// our simple_getattr could just give us inode->i_mode.
	}
}

void do_statfs(char *cmd)
{
	// |fsdb> statfs
	(void) cmd;

	struct kstatfs *buf;
	fsdb.sb->s_op->statfs(fsdb.d_root, buf);
}

void do_close(char *cmd)
{
	// |fsdb> close <fd>
	(void) cmd;

        char *char_fd;
        int fd_key;
        int cmd_argc = 2;
        char_fd = parse_cmd(cmd,cmd_argc);
        fd_key = atoi(char_fd);

	struct file *f;
	HASH_FIND_INT(fd_ht, &fd_key, f);
	if (f == NULL) {
		puts("do_close: Invalid file descriptor");
	} else {
	    	//will this work? do I need to pass in the original pointer I had in HASH_ADD_INT???
	    	HASH_DEL(fd_ht, f);
	    	puts("Deleted fd from fd_ht");
	}

}

void do_exit(char *cmd)
{
	(void) cmd;
	exit_lpfs(); // I think?
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
	{ "stat", do_stat },
	{ "statfs", do_statfs },
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
		unmount_disk(&fsdb.disk);
	}

	return 0;
}
