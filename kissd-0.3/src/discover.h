/*
 * Kiss DP-500 Server Daemon
 * (c) 2008 Rink Springer
 */
#ifndef __DISCOVER_H__
#define __DISCOVER_H__

#define DISCOVER_PORT 8000

extern int gDiscoverSocket;

int  discover_init();
void discover_handle();
void discover_done();

#endif /* !__DISCOVER_H__ */
