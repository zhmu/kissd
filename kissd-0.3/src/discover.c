/*
 * Kiss DP-500 Server Daemon
 * (c) 2008 Rink Springer
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "discover.h"

int gDiscoverSocket = -1;

/*! \brief Initializes the UDP discovery code
 *  \return Zero on failure or non-zero on success
 */
int
discover_init()
{
	struct sockaddr_in sin;

	gDiscoverSocket = socket (AF_INET, SOCK_DGRAM, 0);
	if (gDiscoverSocket < 0) {
		perror ("socket");
		return 0;
	}

	memset (&sin, 0, sizeof (struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (DISCOVER_PORT);
	if (bind (gDiscoverSocket, (struct sockaddr*)&sin, sizeof (sin)) < 0) {
		perror ("bind");
		return 0;
	}

	return 1;
}

/*! \brief Cleans the discoverer up */
void
discover_done()
{
	if (gDiscoverSocket != -1) {
		shutdown (gDiscoverSocket, SHUT_RDWR);
		close (gDiscoverSocket);
		gDiscoverSocket = -1;
	}
}

/*! \brief Called if something is available on the discovery socket */
void
discover_handle()
{
	char buf[128];
	struct sockaddr sa;
	socklen_t slen = sizeof(sa);
	ssize_t s;

	/* Grab data from the socket and NUL-terminate it for convenience */
	s = recvfrom(gDiscoverSocket, buf, sizeof(buf), 0, &sa, &slen);
	buf[s] = 0;

	if (strcmp(buf, "ARE_YOU_KISS_PCLINK_SERVER?"))
		return;

	/* Yes, we are! Send identification */
	if (gethostname(buf, sizeof(buf)) < 0) {
		warn("gethostbyname"); strcpy(buf, "???");
	}

	if (sendto(gDiscoverSocket, buf, strlen(buf), 0, &sa, slen) < 0)
		warn("sendto");
}

/* vim:set ts=2 sw=2: */
