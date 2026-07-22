#ifndef _COMMAND_H
#define _COMMAND_H

#include <termios.h>
#include "sdComm.h"

extern int QVok(sdcInfo info);
extern int QVreset(sdcInfo info, int flag);
extern int QVsectorsize(sdcInfo info, int newsize);
extern int QVdeletepicture(sdcInfo info, int picture_number);
extern int QVbattery(sdcInfo info);
extern long QVrevision(sdcInfo info);
extern struct Image *casio_qv_download_thumbnail(sdcInfo info,
						  int picture_number);
extern struct Image * casio_qv_download_picture(sdcInfo info,
						 int picture_number,
						 int lowResPictureSize);
extern int casio_qv_record(sdcInfo info);
extern int casioSetPortSpeed(sdcInfo info, int speed);

#endif /* _COMMAND_H */
