/*
 * Kiss DP-500 Server Daemon
 * (c) 2005, 2006 Rink Springer
 */
#include <sys/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "kissd.h"
#include "server.h"
#include "util.h"

int gVerbosity = 0;
int gQuit = 0;
char* gAudioPath = NULL;
char* gVideoPath = NULL;
char* gPicturePath = NULL;
char* gExtensions = NULL;

/*! \brief Prints usage information */
void
usage()
{
	fprintf (stderr, "usage: kissd [-h?vdV] [-a path] [-i path] [-p path] [-e extensions]\n\n");
	fprintf (stderr, "   -h, -?          this help\n");
	fprintf (stderr, "   -V              version information\n");
	fprintf (stderr, "   -v              increase verbosity\n");
	fprintf (stderr, "   -a path         path to audio files\n");
	fprintf (stderr, "   -i path         path to video files\n");
	fprintf (stderr, "   -p path         path to picture files\n");
	fprintf (stderr, "   -d              daemonize after startup\n");
	fprintf (stderr, "   -e [exts]       file extensions to serve, seperated by :'s.\n");
	fprintf (stderr, "                   anything will be accepted if you specify a blank list (-e '')\n");
	fprintf (stderr, "                   (default: %s)\n", KISSD_EXT_DEFAULT);
	fprintf (stderr, "\n");
}

/*! \brief Catches SIGINT signals */
void sigint (int num)
{
	gQuit++;
	VPRINTF(1, "caught SIGINT, exiting\n");
}

/*! \brief Displays the version */
void version()
{
	fprintf (stderr, "Kiss DP-500 Daemon "VERSION"\n");
	fprintf (stderr, "(c) 2005, 2006 Rink Springer\n");
	fprintf (stderr, "\n");
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
	char tmp_path[PATH_MAX];

	/* parse options */
	while ((ch = getopt (argc, argv, "h?va:i:p:de:V")) != -1) {
		switch (ch) {
			case 'h':
			case '?': /* help */
		            usage();
		            return EXIT_FAILURE;
			case 'v': /* verbosity */
		            gVerbosity++;
		            break;
			case 'a': /* audio files */
		            if (realpath(optarg, tmp_path) == NULL) {
		            	perror("can't resolve audio path");
		            	return EXIT_FAILURE;
		            }
		            gAudioPath = strdup (tmp_path);
		            break;
			case 'i': /* video files */
		            if (realpath(optarg, tmp_path) == NULL) {
		            	perror("can't resolve video path");
		            	return EXIT_FAILURE;
		            }
		            gVideoPath = strdup (tmp_path);
		            break;
			case 'p': /* picture files */
		            if (realpath(optarg, tmp_path) == NULL) {
		            	perror("can't resolve picture path");
		            	return EXIT_FAILURE;
		            }
		            gPicturePath = strdup (tmp_path);
		            break;
			case 'd': /* daemonize */
		            daemonize++;
		            break;
			case 'e': /* daemonize */
		            gExtensions = strdup (optarg);
		            break;
			case 'V': /* version */
		            version();
		            return EXIT_SUCCESS;
			 default: /* ? */
		            usage();
		            break;
		}
	}
	argc -= optind;
	argv += optind;

	if (gExtensions == NULL)
		gExtensions = KISSD_EXT_DEFAULT;

	if (gVerbosity) {
		fprintf (stderr, "Audio path: %s\n", (gAudioPath != NULL) ? gAudioPath : "(none)");
		fprintf (stderr, "Video path: %s\n", (gVideoPath != NULL) ? gVideoPath : "(none)");
		fprintf (stderr, "Image path: %s\n", (gPicturePath != NULL) ? gPicturePath : "(none)");
		fprintf (stderr, "Extensions: %s\n", (*gExtensions != '\0') ? gExtensions : "(anything)");
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
