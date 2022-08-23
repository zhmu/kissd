/*
 * Kiss DP-500 Server Daemon
 * (c) 2005, 2006 Rink Springer
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "discover.h"
#include "kissd.h"
#include "server.h"
#include "thread.h"
#include "util.h"

int gSocket = -1;

static struct THREAD_STATE thread_state[SERVER_MAX_CONNS];

/*! \brief Retrieves an available thread slot
 *  \return The index on success or -1 if none available
 */
static int
server_alloc_thread_slot()
{
	int i;

	for (i = 0; i < SERVER_MAX_CONNS; i++)
		if (thread_state[i].state == THREAD_STATE_UNUSED) {
			thread_state[i].state = THREAD_STATE_USED;
			return i;
		}

	return -1;
}

/*! \brief Initializes the TCP server code
 *  \return Zero on failure or non-zero on success
 */
int
server_init()
{
	struct sockaddr_in sin;
	int i;

	for (i = 0; i < SERVER_MAX_CONNS; i++)
		thread_state[i].state = THREAD_STATE_UNUSED;

	gSocket = socket (AF_INET, SOCK_STREAM, 0);
	if (gSocket < 0) {
		perror ("socket");
		return 0;
	}

	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (SERVER_PORT);
	if (bind (gSocket, (struct sockaddr*)&sin, sizeof (sin)) < 0) {
		perror ("bind");
		return 0;
	}

	if (listen (gSocket, 5) < 0) {
		perror ("listen");
		return 0;
	}

	return 1;
}

/*! \brief Handles connections as needed */
void
server_run()
{
	int s, slot;
	socklen_t len;
	struct sockaddr_in saddr;
	fd_set rfds, wfds;

	while (!gQuit) {
		/*
		 * We use select(2), because it it will be interrupted by signals
		 * whereas accept(2) does not. This ensures we can exit immediately on a
		 * SIGINT, for example.
		 */
		int max = gSocket;
		FD_ZERO (&rfds); FD_ZERO (&wfds);
		FD_SET (gSocket, &rfds); FD_SET (gSocket, &wfds);
		if (gDiscoverSocket >= 0) {
			FD_SET (gDiscoverSocket, &rfds);
			if (gDiscoverSocket > max)
				max = gDiscoverSocket;
		}
		if (select (max + 1, &rfds, &wfds, NULL, NULL) < 0)
			return;

		if (gDiscoverSocket >= 0 && FD_ISSET(gDiscoverSocket, &rfds))
			discover_handle();

		if (!FD_ISSET(gSocket, &rfds) && !FD_ISSET(gSocket, &wfds))
			continue;

		len = sizeof (struct sockaddr_in);
		s = accept (gSocket, (struct sockaddr*)&saddr, &len);
		if (s < 0) {
			perror ("accept");
			return;
		}

		slot = server_alloc_thread_slot();
		if (slot != -1) {
			VPRINTF (1, "new connection from %s\n", inet_ntoa (saddr.sin_addr));
			thread_state[slot].socket = s;
			thread_state[slot].file   = fdopen (s, "wb");
			if (thread_state[slot].file == NULL) {
				perror ("fdopen");
				close (s);
			} else {
				setbuffer (thread_state[slot].file, NULL, 0);

				/* start a thread to handle this request */
				if (pthread_create (&thread_state[slot].handle, NULL, thread_run, (void*)&thread_state[slot])) {
					perror ("pthread_create");
					close (s);
				}
			}
		} else {
			VPRINTF (0, "connection from %s dropped, out of connection slots\n", inet_ntoa (saddr.sin_addr));
			close (s);
		}
	}
}

/*! \brief Cleans the server up */
void
server_done()
{
	int i;

	if (gSocket != -1) {
		shutdown (gSocket, SHUT_RDWR);
		close (gSocket);
		gSocket = -1;
	}

	/* cancel all threads, this will terminate them */
	for (i = 0; i < SERVER_MAX_CONNS; i++)
		if (thread_state[i].state == THREAD_STATE_USED)
			pthread_cancel (thread_state[i].handle);
}

/* vim:set ts=2 sw=2: */
