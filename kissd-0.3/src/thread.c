/*
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include "cmd.h"
#include "kissd.h"
#include "thread.h"
#include "util.h"

/*! \brief Main code of a thread
 *  \param arg THREAD_STATE structure of the thread
 *  \return NULL
 */
void* thread_run(void* arg)
{
	struct THREAD_STATE* ts = (struct THREAD_STATE*)arg;
	int len;
	char cmd[THREAD_MAX_CMD_LEN];
	fd_set fds;
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, SIGINT);
	pthread_sigmask (SIG_SETMASK, &nset, NULL);

	while (1) {
		FD_ZERO (&fds);
		FD_SET (ts->socket, &fds);
		if (select (ts->socket + 1, &fds, &fds, NULL, NULL) < 0) {
			perror ("select");
			goto cleanup;
		}

		memset (cmd, 0, sizeof (cmd));
		len = read (ts->socket, cmd, sizeof (cmd) - 1);
		if (len < 0) {
			perror ("read");
			goto cleanup; 
		}
		cmd[len] = 0;

		if (!len) {
			/* no data; we assume the connection has died away */
			VPRINTF (1, "thread %p: connection lost\n", ts);
			goto cleanup;
		}

		len--;
		while (cmd[len] == 0x0a || cmd[len] == 0x0d) cmd[len--] = 0;

		/* handle the actual command. zero exit means: kill the connection */
		if (!cmd_handle (ts, cmd))
			goto cleanup;
	}

cleanup:
	close (ts->socket);
	ts->state = THREAD_STATE_UNUSED;

	return NULL;
}

/* vim:set ts=2 sw=2: */
