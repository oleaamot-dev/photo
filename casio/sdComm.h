#ifndef _SDCOMM_H
#define _SDCOMM_H

#include "sdCommDefines.h"

typedef struct _sdcInfo *sdcInfo;

extern sdcInfo sdcInit(char *serialDeviceName);
extern void sdcDebug(sdcInfo info, int onOff);
extern int sdcOpen(sdcInfo info);
extern int sdcClose(sdcInfo info);
extern int sdcFlush(sdcInfo info);
extern int sdcIsClosed(sdcInfo info);
extern int sdcRead(sdcInfo info, unsigned char *buf, int len);
extern int sdcReadAll(sdcInfo info, unsigned char *buf, int *len);
extern int sdcWrite(sdcInfo info, unsigned char *buf, int len);
extern int sdcSendByte(sdcInfo info, unsigned char c);
extern int sdcGetBaudRate(sdcInfo info);
extern int sdcSetBaudRate(sdcInfo info, int baud_rate);

#endif /* _SDCOMM_H */
