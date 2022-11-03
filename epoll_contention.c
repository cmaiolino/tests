#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>

#define NFILES 100
#define NTHREADS 10

/*global epoll fd*/
int efd;

void * contend_files() {

	int lfds[NFILES][2];
	struct epoll_event events[NFILES] = {0};
	int i = 0;
	int ret = 0;

	for (i = 0; i < NFILES; i++) {
		ret = pipe(lfds[i]);
		if (ret < 0) {
			perror("pipe()");
			exit(1);
		}

		ret = epoll_ctl(efd, EPOLL_CTL_ADD, lfds[i][0], &events[i]);
		if (ret) {
			perror("epoll_ctl");
			exit(1);
		}
	}

	for (i = 0; i < NFILES; i++)
		close(lfds[i][0]);
	return NULL;
}

int main(void) {

	efd = epoll_create1(0);
	if (efd < 0) {
		perror ("epoll_create");
		exit(1);
	}

	contend_files();
	close(efd);
	return 0;
}
