/* 
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "kissd.h"
#include "thread.h"
#include "util.h"

/*! \brief Does a printf() based on verbosity
 *  \param level Verbosity level required to print
 *  \param fmt Format string for printf()
 */
void
VPRINTF (int level, char* fmt, ...)
{
	va_list va;

	if (gVerbosity < level)
		return;

	va_start (va, fmt);
	vfprintf (stderr, fmt, va);
	va_end (va);
}

/*! \brief Sends formatted output to a thread's socket
 *  \param t Thread's socket to use
 *  \param fmt Format specifier to use
 */
void
sendf (struct THREAD_STATE* t, char* fmt, ...)
{
	va_list va;

	va_start (va, fmt);
	vfprintf (t->file, fmt, va);
	va_end (va);
}

/* vim:set ts=2 sw=2: */
