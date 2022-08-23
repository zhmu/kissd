/*
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#include "thread.h"

#ifndef __UTIL_H__
#define __UTIL_H__

void VPRINTF (int level, char* fmt, ...);
void sendf (struct THREAD_STATE* t, char* fmt, ...);

#endif /* !__UTIL_H__ */
