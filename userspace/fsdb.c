/*
 * fsdb.c
 *
 * A userspace filesystem testing repl
 */

#include <lpfs/compat.h>
#include "uthash.h"
#include <stdio.h>
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
	return cmd_argv[1];
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

    token = strtok(path, "/");

    while (token != NULL) {
	/* Look up child dentry by name (token), and fill in d_tmp */    	
    	HASH_FIND_STR(d->d_child_ht, token, d_tmp); 
    	if (d_tmp == NULL) {
    		return NULL; // invalid path!
    	}
    	token = strtok(path, "/"); // next path component
    	d = d_tmp; // new parent dentry

	puts("token:");
	puts(token);
	token = strtok(NULL, "/");
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
	puts("Not implemented.");
}

void do_open(char *cmd)
{
	// |fsdb> open <path>
	(void) cmd;
	puts("Not implemented.");
	puts(fsdb.d_root->d_name.name);
	puts(cmd);

        char *path;
        int cmd_argc = 2;

        path = parse_cmd(cmd,cmd_argc);

        puts(path);
        // get the dentry of the path
        // construct a struct file
        // HASH_ADD_INT(next_fd++, file);

}

void do_mkdir(char *cmd)
{
	// |fsdb> mkdir <path>
	(void) cmd;

	/* Path parsing */
        char *path;
        int cmd_argc = 2;
        path = parse_cmd(cmd,cmd_argc);

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
        }
        

	/* Now that we have the parent dentry, construct one for the new subdirectory
	 * we are making. 
	 */
        struct dentry *new_dir_dentry;
	// This should call ramfs_mkdir.
        parent_dentry->d_inode->i_op->mkdir(parent_dentry->d_inode, new_dir_dentry, S_IFDIR); 
        // If successful new_dir_dentry should have been instantiated with a new inode...
        new_dir_dentry->d_parent = parent_dentry; // does d_instantiate do this?
        struct qstr d_name_qstr = { 
        	.len = sizeof(new_dir_name),
        	.name = new_dir_name
        };
	new_dir_dentry->d_name = d_name_qstr;
	new_dir_dentry->name = new_dir_name; // careful here...we use this as the key to parent_dentry->d_child_ht.

//	HASH_ADD_KEYPTR(hh, hashtable location, string pointer as key(the child name), sizeof(new_dir_name), the value the key hashes to(struct dentry));
//	HASH_ADD_KEYPTR(hh, parent_dentry->d_child_ht, new_dir_name, sizeof(new_dir_name), new_dir_dentry);

	struct dentry *temp;
	HASH_FIND_STR(parent_dentry->d_child_ht, "var", temp);
	if (temp != NULL) puts("SUCCESSFULLY PUT NEW DENTRY");




}

void do_read(char *cmd)
{
	// |fsdb> read <fd> <size> <off> [<hostpath>]
	(void) cmd;
	puts("Not implemented.");
	//do_sync_read(struct file *filp, , char __user *buf, ssize_t len,  loff_t *pos);
	// so some sort of fd->file mapping	

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

     	char *new_dir_name = strdup(basename(path));
     	char *rest_of_path = strdup(dirname(path));

        struct dentry *parent_dentry;
        parent_dentry = dentry_lookup(rest_of_path);
        if (parent_dentry == NULL) {
        puts("Error: Invalid path");
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
}

void do_rename(char *cmd)
{
	// |fsdb> rename <fd> <path>
	(void) cmd;
	puts("Not implemented.");
}

void do_stat(char *cmd)
{
	// |fsdb> stat <fd>
	(void) cmd;
	puts("Not implemented.");
}

void do_statfs(char *cmd)
{
	// |fsdb> statfs
	(void) cmd;
	puts("Not implemented.");
}

void do_close(char *cmd)
{
	// |fsdb> close <fd>
	(void) cmd;
	puts("Not implemented.");
}

void do_exit(char *cmd)
{
	(void) cmd;
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
