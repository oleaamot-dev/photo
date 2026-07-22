#ifndef _CORE_H
#define _CORE_H

#include "io.h"
#include "mdc800_spec.h"
#include "image.h"

//#define SET_115200

#define MDC800_FLASHLIGHT_REDEYE 1
#define MDC800_FLASHLIGHT_ON 		2
#define MDC800_FLASHLIGHT_OFF		4
#define MDC800_FLASHLIGHT_AUTO   0

//--------------------------------------------------------------------------/

int mdc800_openCamera (char*);
int mdc800_closeCamera ();

int mdc800_changespeed (int);
int mdc800_getSpeed ();

//- Camera must be open for these functions --------------------------------/

int mdc800_setTarget (int);


struct Image* mdc800_getThumbnail (int);
struct Image* mdc800_getImage (int);

//------- SystemStatus ----------------------------------------------------/

int mdc800_getSystemStatus ();
int mdc800_isCFCardPresent ();
int mdc800_getMode ();
int mdc800_getFlashLightStatus ();
int mdc800_isLCDEnabled ();
int mdc800_isBatteryOk ();
int mdc800_isMenuOn ();
int mdc800_isAutoOffEnabled ();

int mdc800_getStorageSource ();

//------- Other Functions --------------------------------------------------/

// Most of these Function depends on the Storage Source

int mdc800_setDefaultStorageSource ();  // The driver don't needs this anymore
int mdc800_setStorageSource (int);
int mdc800_setMode (int);
int mdc800_enableLCD (int);
int mdc800_playbackImage (int );
int mdc800_getRemainFreeImageCount (int*,int* ,int*);
int mdc800_setFlashLight (int );

char* mdc800_getFlashLightString (int);

int mdc800_getImageQuality ();
int mdc800_setImageQuality (int);

int mdc800_getWBandExposure (int*, int*);
int mdc800_setExposure (int);

int mdc800_setWB (int);

int mdc800_setExposureMode (int);
int mdc800_getExposureMode ();

int mdc800_enableMenu (int);
#endif
