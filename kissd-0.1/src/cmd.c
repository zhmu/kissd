/*
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cmd.h"
#include "config.h"
#include "kissd.h"
#include "thread.h"
#include "util.h"

#ifdef HAVE_FTS_H
#include <fts.h>

/*! \brief Compare function for fts(3)
 *  \param a First file to compare
 *  \param b File to compare with
 *  \return Same as strcoll()
 */
static int
compar(const FTSENT* const* a, const FTSENT* const* b)
{
	return strcoll ((*a)->fts_name, (*b)->fts_name);
}
#endif /* HAVE_FTS_H */

/*! \brief Checks whether the KiSS can cope with an extension
 *  \param name Filename to check
 *  \return Non-zero if the extension is ok, zero if not
 *
 *  As the KiSS tends to go berserk on file types it doesn't really
 *  understand, this function will protect it from harm.
 */
static int
verify_ext(char* name)
{
	char* ext = strchr (name, '.');
	if (ext == NULL) return 0;
	ext++;

	/* this is to protect the KISS from file types it can't handle */
	if (strcasecmp (ext, "jpg") && strcasecmp (ext, "bmp") &&
			strcasecmp (ext, "png") && strcasecmp (ext, "jpeg") &&
			strcasecmp (ext, "avi") && strcasecmp (ext, "mpeg") &&
			strcasecmp (ext, "mpg") && strcasecmp (ext, "mp3"))
		return 0;

	return 1;
}

/*! \brief Handles KiSS LIST commands
 *  \param t Thread to handle the command for
 *  \param cmd Full command to handle
 *  \return Zero means close socket, non-zero keeps it open
 *
 *  The LIST command is in the form
 *
 *  LIST <category> |<path>|
 *
 *  In which <category> is AUDIO, VIDEO or PICTURE. It must send a
 *  directory listing, in the following form:
 *
 *  File      : displayname| full path and file|0
 *  Directory : displayname| directory|1
 *
 *  The <directory> part of a directory listing must _not_ contain
 *  parent directories; Surprisingely, the KiSS is clever enough to
 *  handle them itself.
 *
 *  The connection must be closed after each listing.
 */
static int
cmd_list (struct THREAD_STATE* t, char* cmd)
{
	char* category = (cmd + 5);
	char* path;
	char* ptr;
	char* basepath = NULL;
	char  tmppath[_POSIX_PATH_MAX];
#ifndef HAVE_FTS_H
	DIR*  dir;
  struct dirent* dent;
	struct stat st;
	char  tmppath2[_POSIX_PATH_MAX];
#else /* HAVE_FTS_H */
	FTS* fts;
	FTSENT* fent;
	char* paths[2];
#endif

	ptr = strchr (category, ' '); if (ptr == NULL) return 1;
	*ptr++ = 0; if (*ptr != '|') return 1;
	path = (ptr + 1); ptr = strchr (path, '|');
	if (ptr == NULL) return 1;
	*ptr = 0;

	VPRINTF(2, "thread %p: list, category='%s', path='%s'\n", t, category, path);
	
	if (!strcmp (category, "AUDIO")) basepath = gAudioPath;
	if (!strcmp (category, "VIDEO")) basepath = gVideoPath;
	if (!strcmp (category, "PICTURE")) basepath = gPicturePath;
	if (basepath == NULL) return 1;
	snprintf (tmppath, sizeof (tmppath), "%s/%s", basepath, path);

	VPRINTF(3, "final path: '%s'\n", tmppath);

#ifndef HAVE_FTS_H
	dir = opendir (tmppath);
	if (dir == NULL) {
		perror ("readdir");
		return 1;
	}
#else /* HAVE_FTS_H */
	paths[0] = tmppath;
	paths[1] = NULL;
	fts = fts_open (paths, FTS_LOGICAL, compar);
	if (fts == NULL) {
		perror ("fts_open");
		return 1;
	}
#endif

#ifndef HAVE_FTS_H
	while ((dent = readdir (dir)) != NULL) {
		if (!strcmp (dent->d_name, ".") || !strcmp (dent->d_name, ".."))
			continue;

		if (dent->d_type == DT_UNKNOWN) {
			/*
			 * It seems d_type is not filled out for NFS mounts. Therefore, we must
			 * do a quick stat() and copy the information from there
			 */
			snprintf (tmppath2, sizeof (tmppath2), "%s/%s", tmppath, dent->d_name);
			if (stat (tmppath2, &st) < 0)
				continue;

			if (S_ISREG (st.st_mode)) dent->d_type = DT_REG;
			if (S_ISDIR (st.st_mode)) dent->d_type = DT_DIR;
		}

		if ((dent->d_type != DT_REG && dent->d_type != DT_DIR))
			continue;

		if (dent->d_type != DT_DIR) {
			if (!verify_ext (dent->d_name))
				continue;
			sendf (t, "%s| %s/%s|0|\n", dent->d_name, tmppath, dent->d_name);
		} else {
			sendf (t, "%s| %s|1|\n", dent->d_name, dent->d_name);
		}
	}

	closedir (dir);
#else /* HAVE_FTS_H */
	while ((fent = fts_read (fts)) != NULL) {
		/* ignore anything not a file, dir or link */
		if ((fent->fts_info != FTS_D) && (fent->fts_info != FTS_F) &&
			  (fent->fts_info != FTS_NSOK) && (fent->fts_info != FTS_DEFAULT))
			continue;

		if (fent->fts_info == FTS_D) {
			if (fent->fts_level > 0) {
				sendf (t, "%s| %s|1|\n", fent->fts_name, fent->fts_name);

				/* ensure we skip any child directories */
				fts_set(fts, fent, FTS_SKIP);
			}
			continue;
		}

		if (!verify_ext (fent->fts_name))
			continue;

		sendf (t, "%s| %s/%s|0|\n", fent->fts_name, tmppath, fent->fts_name);
	}

	fts_close(fts);
#endif /* HAVE_FTS_H */

	return 0;
}

/*! \brief Handles KiSS GET commands
 *  \param t Thread to handle the command for
 *  \param cmd Full command to handle
 *  \return Zero means close socket, non-zero keeps it open
 *
 *  The GET command is in the form
 *
 *  GET <file>| <offs> <len>
 *
 *  In which <file> is the location on the server (as returned
 *  in the <full path and file> field of LIST), <offs> is the
 *  offset to read and <len> the length in bytes.
 *
 *  The connection must be closed on errors. Short reads or
 *  writes must not be considered errors!
 */
static int
cmd_get (struct THREAD_STATE* t, char* cmd)
{
	char* file = (cmd + 4);
	char* ptr;
	unsigned long offs, len, filelen, lenread;
	FILE* f;

	ptr = strchr (file, '|'); if (ptr == NULL) return 1;
	*ptr++ = 0;
	offs = strtol (ptr, &ptr, 10);
	if (*ptr != ' ') return 1; ptr++;
	len = strtol (ptr, &ptr, 10);
	if (*ptr != ' ') return 1;

	/* GET /tmp/file.jpg| 0 0  */
	VPRINTF (2, "thread %p: get, file='%s', offs=%lu, len=%lu\n", t, file, offs, len);

	/* safety checks */
	if (strstr (file, "..") != NULL)
		return 1;

	f = fopen (file, "rb");
	if (f == NULL) {
		perror ("fopen");
		return 0;
	}
	fseek (f, 0, SEEK_END); filelen = ftell (f); rewind (f);

	if ((offs == 0) && (len == 0)) len = filelen;

	ptr = malloc (len);
	if (ptr == NULL) {
		perror ("malloc");
		return 0;
	}
	fseek (f, offs, SEEK_SET);
	if ((lenread = fread (ptr, 1, len, f)) < 0) {
		perror ("fread");
		free (ptr);
		return 0;
	}

	fclose (f);

	if (send (t->socket, ptr, lenread, 0) != lenread) {
		perror ("send");
	}
	free (ptr);
	return 1;
}

/*! \brief Handles KiSS ACTION commands
 *  \param t Thread to handle the command for
 *  \param cmd Full command to handle
 *  \return Zero means close socket, non-zero keeps it open
 *
 *  The ACTION command is possible in two forms:
 *
 *  (1): ACTION <num> |<playerid>| <file>|
 *  (2): ACTION <num> <file>|
 *
 *  <num> is some value indicating the server what is happening
 *  with the file; this hasn't been researched further. <playerid>
 *  is the KiSS' player id, and <file> is the full path+name of the
 *  file as returned using LIST.
 *
 *  Response must be either 200 (if the file exists and can be used)
 *  or 404 if the file doesn't exist. The connection must be kept open,
 *  and no newlines may be sent.
 */
static int
cmd_action (struct THREAD_STATE* t, char* cmd)
{
	char* ptr;
	char* playerid;
	char* file;
	unsigned long num;
	struct stat sb;

	num = strtol (cmd + 7, &ptr, 10);
	if (*ptr++ != ' ') return 1;
	if (*ptr != '|') {
		/* It's form 2 */
		file = ptr; playerid = NULL;
	} else {
		playerid = ++ptr;
		ptr = strchr (playerid, '|'); if (ptr == NULL) return 1; *ptr++ = 0;
		if (*ptr++ != ' ') {
			return 1;
		}
		file = ptr; 
	}
	ptr = strchr (file, '|'); if (ptr == NULL) return 1;
	*ptr = 0;

	VPRINTF (2, "thread %p: action, num=%d, playerid=%s, file=%s\n", t, num, playerid, file);

	if (stat (file, &sb) < 0) {
		sendf (t, "404");
	} else {
		sendf (t, "200");
	}

	return 1;
}

/*! \brief Handles KiSS SIZE commands
 *  \param t Thread to handle the command for
 *  \param cmd Full command to handle
 *  \return Zero means close socket, non-zero keeps it open
 *
 *  The SIZE command is in the form
 *
 *  SIZE <file>|
 *
 *  In which <file> is the full path+filename as returned by
 *  LIST. The function must send exactly 15 bytes of filesize,
 *  padded with leading 0's as needed. On failure, a size of
 *  0 can be sent. Absolutely no newlines may be sent (this
 *  breaks video playback) and the socket must remain open.
 */
static int
cmd_size(struct THREAD_STATE* t, char* cmd)
{
	char* file;
	char* ptr;
	FILE* f;
	unsigned long size;

	file = (cmd + 5);
	ptr = strchr (file, '|'); if (ptr == NULL) return 1;
	*ptr = 0;

	f = fopen (file, "rb");
	if (f == NULL) {
		perror ("fopen");
		size = 0;
	} else {
		fseek (f, 0, SEEK_END); size = ftell (f); rewind (f);
		fclose (f);
	}

	sendf (t, "%015d", size);

	return 1;
}

/*! \brief Handles KiSS commands
 *  \param t Thread to handle the command for
 *  \param cmd Full command to handle
 *  \return Zero means close socket, non-zero keeps it open
 */
int
cmd_handle(struct THREAD_STATE* t, char* cmd)
{
	VPRINTF(3,"Thread %p: command [%s]\n", t, cmd);

	if (!strncmp (cmd, "LIST ", 5))   return cmd_list   (t, cmd);
	if (!strncmp (cmd, "GET ",  4))   return cmd_get    (t, cmd);
	if (!strncmp (cmd, "ACTION ", 7)) return cmd_action (t, cmd);
	if (!strncmp (cmd, "SIZE ", 5))   return cmd_size   (t, cmd);

	return 0;
}

/* vim:set ts=2 sw=2: */
