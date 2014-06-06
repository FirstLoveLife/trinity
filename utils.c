#include <errno.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "utils.h"

/*
 * Use this allocator if you have an object a child writes to that you want
 * all other processes to see.
 */
void * alloc_shared(unsigned int size)
{
	void *ret;

	ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
	if (ret == MAP_FAILED) {
		printf("mmap %u failure\n", size);
		exit(EXIT_FAILURE);
	}
	return ret;
}

void * __zmalloc(size_t size, const char *func)
{
	void *p;

	p = malloc(size);
	if (p == NULL) {
		/* Maybe we mlockall'd everything. Try and undo that, and retry. */
		munlockall();
		p = malloc(size);
		if (p != NULL)
			goto done;

		printf("%s: malloc(%zu) failure.\n", func, size);
		exit(EXIT_FAILURE);
	}

done:
	memset(p, 0, size);
	return p;
}

void sizeunit(unsigned long size, char *buf)
{
	if (size < 1024 * 1024) {
		sprintf(buf, "%lu bytes", size);
		return;
	}

	if (size < (1024 * 1024 * 1024)) {
		sprintf(buf, "%ldMB", (size / 1024) / 1024);
		return;
	}

	sprintf(buf, "%ldGB", ((size / 1024) / 1024) / 1024);
}

void kill_pid(pid_t pid)
{
	int ret;

	ret = kill(pid, SIGKILL);
	if (ret != 0)
		debugf("couldn't kill pid %d [%s]\n", pid, strerror(errno));
}

void show_backtrace(void)
{
	unsigned int j, nptrs;
#define SIZE 100
	void *buffer[SIZE];
	char **strings;

	nptrs = backtrace(buffer, SIZE);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		return;
	}

	for (j = 0; j < nptrs; j++)
		output(0, "%s\n", strings[j]);

	free(strings);
}
