#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define NUM_TESTS 3

const char *fsdb = ".lpfs/fsdb";

enum {
	READ = 0,
	WRITE = 1,
};

int corr[NUM_TESTS];

void send_fsdb_command(const char* inp, char buf[], int writec, int readp) {
        int i;
        for (i = 0; i < sizeof(buf); i++) {
          buf[i] = 0;
        }
        int len = strlen(inp);
        write(writec, (const void*) inp, len);
        read(readp, buf, sizeof(buf));
}

void check_result(const char* exp, char* out, int i) {
        if ( strcmp(exp, out) == 0) {
                corr[i] = 1;
        } else {
                corr[i] = 0;
        }
}

int main()
{
	int err;
	int p_fildes[2];
	int c_fildes[2];

	err = pipe(p_fildes);
	if (err != 0) {
		perror("parent pipe");
		return errno;
	}

	err = pipe(c_fildes);
	if (err != 0) {
		perror("child pipe");
		return errno;
	}

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return errno;
	} else if (pid == 0) {
		puts("In Parent");

		const char *msg = "Pipes running.\n";
		size_t len = strlen(msg);
		err = write(c_fildes[WRITE], (const void *) msg, len);
		if (err < len) {
			perror("write");
			return errno;
		}

		char buf[512] = { 0 };
		err = read(p_fildes[READ], buf, sizeof(buf));
		if (err < 0) {
			perror("read");
			return errno;
		}

		puts(buf);

                send_fsdb_command("ls\n", buf, c_fildes[WRITE], p_fildes[READ]);
                check_result("", buf, 0);
                send_fsdb_command("mkdir test\n", buf, c_fildes[WRITE], p_fildes[READ]);
                check_result("", buf, 1);
                send_fsdb_command("ls\n", buf, c_fildes[WRITE], p_fildes[READ]);
                check_result("test", buf, 2);

		wait(NULL);

		close(p_fildes[READ]);
		close(p_fildes[WRITE]);
		close(c_fildes[READ]);
		close(c_fildes[WRITE]);

                int i = 0;
                for (; i < NUM_TESTS; i++) {
                  if (corr[i]) {
                    printf("Test #%d was correct\n", i);
                  } else {
                    printf("Test #%d failed\n", i);
                  }
                  
                }
	} else {
		puts("In Child");

		if (dup2(c_fildes[READ], 0) < 0) {
			perror("dup2 - READ");
			return errno;
		}

		if (dup2(p_fildes[WRITE], 1) < 0) {
			perror("dup2 - WRITE");
			return errno;
		}

		/* A simple test. */
#if 0
		int ret;
		char *line = NULL;
		size_t len;

		if (getline(&line, &len, stdin) <= 0) {
			puts("Pipe empty.");
			exit(-1);
		}

		printf(line);
#endif

		execl(fsdb, "-ramfs", NULL);
	}

	return 0;
}
