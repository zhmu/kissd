/*
 * Kiss DP-500 Server Daemon
 * (c) 2005, 2006 Rink Springer
 */
#ifndef __KISSD_H__
#define __KISSD_H__

/*! \brief Defaults extensions served */
#define KISSD_EXT_DEFAULT "jpg:bmp:png:jpeg:avi:mpeg:mpg:mp3:ogg"

extern int gQuit;
extern int gVerbosity;
extern char* gAudioPath;
extern char* gVideoPath;
extern char* gPicturePath;
extern char* gExtensions;

#endif /* !__KISSD_H__ */
