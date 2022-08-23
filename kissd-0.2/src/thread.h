/* 
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#include <pthread.h>
#include <stdio.h>

#ifndef __THREAD_H__
#define __THREAD_H__

#define THREAD_STATE_UNUSED 0
#define THREAD_STATE_USED   1

struct THREAD_STATE {
	int state;
	int socket;
	FILE* file;
	pthread_t handle;
};

#define THREAD_MAX_CMD_LEN	256

void* thread_run(void* arg);

#endif /* !__THREAD_H__ */

/* vim:set ts=2 sw=2: */
