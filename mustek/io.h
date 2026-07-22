#ifndef _IO_H
#define _IO_H

/* Maximum Number of tries, before a command is given up */
#define MDC800_DEFAULT_COMMAND_RETRY			3

/* There is a little delay for the next sending (ms) */
#define MDC800_DEFAULT_COMMAND_RETRY_DELAY 	300

/* Default Timeout */
#define MDC800_DEFAULT_TIMEOUT		 250

/* Prevent Overruns ( ms) ?? */
#define MDC800_DEFAULT_COMMAND_DELAY 50

/* Long Timeout for Functions that needs time (Take Photo, delete..) */
#define MDC800_LONG_TIMEOUT			 5000

/* 20sec Timeout for Flashlight */
#define MDC800_TAKE_PICTURE_TIMEOUT  20000


int mdc800_io_sendCommand_with_retry (char*, char* , int, int,int);

/* The API to the upper Layer */
int mdc800_io_openDevice (char*);
int mdc800_io_closeDevice ();
int mdc800_io_sendCommand (char ,char,char,char,char *,int );
int mdc800_io_sendUSBCommand (char, char, char, char, char, char, char, char*, int);
int mdc800_io_changespeed (int );


/* Helper Function for rs232 and usb */
int mdc800_io_getCommandTimeout (char );

#include <gpio.h>

#ifdef _IO_C
gpio_device_settings  mdc800_io_device_settings;      // Device settings
gpio_device*          mdc800_io_device_handle=0;		// Device handle
int                   mdc800_io_using_usb=0;			   // This value is set by initialize
#else
extern gpio_device_settings  mdc800_io_device_settings;
extern gpio_device*          mdc800_io_device_handle;
extern int                   mdc800_io_using_usb;
#endif

#endif
