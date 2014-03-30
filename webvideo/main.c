#include "cbs.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

int worker(void* arg)
{
	usleep(100000);
	fprintf(stdout, "worker: %p\n", arg);
	return CBS_CONTINUE;
}

int main(int argc, char** argv)
{
	cbs_t task;
	struct timeval period = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	if (cbs_create(&task, CBS_BW, 100, &period, worker, NULL)) {
		fprintf(stderr, "Call to cbs_create failed.\n");
		return -1;
	}

	return cbs_join(&task, NULL);
}
