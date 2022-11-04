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

/* global epoll fd */
int efd;

/* Thread descriptor */
struct thread_info {
	pthread_t thread_id;
	int thread_num;
};

pthread_mutex_t tmutex;

void * contend_files(void *tnum) {

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

	printf("Thread %d locking\n", *(int *)tnum);
	pthread_mutex_lock(&tmutex);
	pthread_mutex_unlock(&tmutex);
	printf("Thread %d unlocked\n", *(int *)tnum);
	for (i = 0; i < NFILES; i++)
		close(lfds[i][0]);
	return NULL;
}

int main(void) {


	struct thread_info threads[NTHREADS];
	int i = 0;
	int ret = 0;
	int garbage = 0;

	efd = epoll_create1(0);
	if (efd < 0) {
		perror ("epoll_create");
		exit(1);
	}

	pthread_mutex_init(&tmutex, NULL);

	pthread_mutex_lock(&tmutex);

	for (i = 0; i < NTHREADS; i++) {
		threads[i].thread_num = i;
		ret = pthread_create(&threads[i].thread_id, NULL,
				     &contend_files,
				     (void *)&threads[i].thread_num);
		if (ret) {
			perror("pthread_create");
			exit(1);
		}
	}

	/*
	 * We don't care whatever user type here, it's just a say to wait user
	 * to trigger the mutex unlock
	 */
	printf("Waiting user input\n");
	scanf("%d ", &garbage);
	pthread_mutex_unlock(&tmutex);

	for (i = 0; i < NTHREADS; i++)
		pthread_join(threads[i].thread_id, NULL);

	pthread_mutex_destroy(&tmutex);
	close(efd);
	return 0;
}
