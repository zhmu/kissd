/*
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kissd.h"
#include "server.h"
#include "util.h"

int gVerbosity = 0;
int gQuit = 0;
char* gAudioPath = NULL;
char* gVideoPath = NULL;
char* gPicturePath = NULL;

/*! \brief Prints usage information */
void
usage()
{
	fprintf (stderr, "usage: kissd [-h?vd] [-a path] [-i path] [-p path]\n\n");
	fprintf (stderr, "   -h, -?          this help\n");
	fprintf (stderr, "   -v              increase verbosity\n");
	fprintf (stderr, "   -a path         path to audio files\n");
	fprintf (stderr, "   -i path         path to video files\n");
	fprintf (stderr, "   -p path         path to picture files\n");
	fprintf (stderr, "   -d              daemonize after startup\n");
	fprintf (stderr, "\n");
}

/*! \brief Catches SIGINT signals */
void sigint (int num)
{
	gQuit++;
	VPRINTF(1, "caught SIGINT, exiting\n");
}

/*! \brief The main program
 *  \param argc Argument count
 *  \param argv Arguments
 *  \return EXIT_SUCCESS on success otherwise EXIT_FAILURE
 */
int
main(int argc,char** argv)
{
	int ch;
	int daemonize = 0;

	/* parse options */
	while ((ch = getopt (argc, argv, "h?va:i:p:d")) != -1) {
		switch (ch) {
			case 'h':
			case '?': /* help */
		            usage();
		            return EXIT_FAILURE;
			case 'v': /* verbosity */
		            gVerbosity++;
		            break;
			case 'a': /* audio files */
		            gAudioPath = strdup (optarg);
		            break;
			case 'i': /* video files */
		            gVideoPath = strdup (optarg);
		            break;
			case 'p': /* picture files */
		            gPicturePath = strdup (optarg);
		            break;
			case 'd': /* daemonize */
		            daemonize++;
		            break;
			 default: /* ? */
		            usage();
		            break;
		}
	}
	argc -= optind;
	argv += optind;

	if (gAudioPath == NULL) gAudioPath = strdup ("/tmp");
	if (gVideoPath == NULL) gVideoPath = strdup ("/tmp");
	if (gPicturePath == NULL) gPicturePath = strdup ("/tmp");

	fprintf (stderr, "Kiss DP-500 Daemon version 1.0\n");
	fprintf (stderr, "(c) 2005 Rink Springer\n");
	fprintf (stderr, "\n");
	if (gVerbosity) {
		fprintf (stderr, "Audio path: %s\n", gAudioPath);
		fprintf (stderr, "Video path: %s\n", gVideoPath);
		fprintf (stderr, "Image path: %s\n", gPicturePath);
		fprintf (stderr, "\n");
	}

	signal (SIGINT, sigint);

	if (daemonize) {
		if (daemon (1, 1) < 0)
			perror("daemon");
	}

	if (!server_init())
		return EXIT_FAILURE;
	server_run();
	server_done();

	return EXIT_SUCCESS;
}

/* vim:set ts=2 sw=2: */
