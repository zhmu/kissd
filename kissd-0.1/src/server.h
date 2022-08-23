/*
 * Kiss DP-500 Server Daemon
 * (c) 2005 Rink Springer
 */
#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_PORT 8000
#define SERVER_MAX_CONNS 16

int  server_init();
void server_run();
void server_done();

#endif /* !__SERVER_H__ */
