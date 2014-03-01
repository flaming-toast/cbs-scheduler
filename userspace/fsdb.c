/*
 * fsdb.c
 *
 * A userspace filesystem testing repl
 */

#include <lpfs/compat.h>

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
}

void do_mkdir(char *cmd)
{
	// |fsdb> mkdir <path>
	(void) cmd;
	puts("Not implemented.");
}

void do_read(char *cmd)
{
	// |fsdb> read <fd> <size> <off> [<hostpath>]
	(void) cmd;
	puts("Not implemented.");
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
	puts("Not implemented.");
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
