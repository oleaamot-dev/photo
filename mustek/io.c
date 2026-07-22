/*
 * Copyright (C) 1999/2000 by Henning Zabel <henning@uni-paderborn.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
/*
 * gphoto driver for the Mustek MDC800 Digital Camera. The driver 
 * supports rs232 and USB. It automatically detects which Kernelnode
 * is used.
 */
 
/*
 * Implements cummunication-core :
 *
 * Sending commands and receiving data
 */
#define _IO_C
#include "io.h"
#undef _IO_C
#include "../src/gphoto.h"

#include <string.h>
#include <sys/time.h>
#include "print.h"
#include "rs232.h"
#include "usb.h"
#include "mdc800_spec.h"

//--------------------- Communicating to the Camera --------------------------/
           
/*
 * Opens Connection to Camera, 
 */
int mdc800_io_openDevice (char* device)
{
	if (mdc800_io_device_handle != 0)
	{
		printCError ("(mdc800_io_openDevice) Camera was already open.\n");
		return 1;
		//closeCamera ();
	}
	
	if (mdc800_io_using_usb)
	{
		mdc800_io_device_handle=gpio_new (GPIO_DEVICE_USB);
		
/*
		mdc800_io_device_settings.usb.bus=1;
		mdc800_io_device_settings.usb.device=10;
*/
	}
	else
	{
		mdc800_io_device_handle=gpio_new (GPIO_DEVICE_SERIAL);
		
		mdc800_io_device_settings.serial.speed=57600;
		mdc800_io_device_settings.serial.parity=0;
		mdc800_io_device_settings.serial.bits=8;
		mdc800_io_device_settings.serial.stopbits=1;
		strcpy (mdc800_io_device_settings.serial.port, device);
	}
	
	gpio_set_settings (mdc800_io_device_handle, mdc800_io_device_settings);
	
/*
	if (gpio_reset (mdc800_io_device_handle) != GPIO_OK)
	{
		printCError ("(mdc800_io_openDevice) error reset libgpio ?!\n");
		gpio_free (mdc800_io_device_handle);
		mdc800_io_device_handle=0;
		return 0;
	}
*/	
	
	if (gpio_open (mdc800_io_device_handle) != GPIO_OK)
	{
		printCError ("(mdc800_io_openDevice) can't open camera.\n");
		gpio_free (mdc800_io_device_handle);
		mdc800_io_device_handle=0;
		return 0;
	}



	return 1;
	
}


/*
 * Closes the connection to the camera
 */
int mdc800_io_closeDevice ()
{
	if (mdc800_io_device_handle == 0)
	{
		printCError ("(mdc800_io_closeCamera) Camera is already closed !\n");
		return (1);
	}
	gpio_close (mdc800_io_device_handle);
	gpio_free (mdc800_io_device_handle);
	mdc800_io_device_handle=0;

	return (1);
}


/*
 * Sets the baud rate 
 * 0: 19200, 1:57600, 2: 115200
 */
int mdc800_io_changespeed (int value)
{
	int rates [3]={19200, 57600, 115200};
	int retval=0;
	if (mdc800_io_using_usb)
	{
		return 1;
	}
	
	mdc800_io_device_settings.serial.speed=rates[value];
	retval=gpio_set_settings (mdc800_io_device_handle, mdc800_io_device_settings);
	
	return (retval == GPIO_OK);
}


/*
 * Send a Command to the Camera. It is unimportent wether this
 * is a USB or a RS232 Command. The Function implements an automatic
 * retry of a failed command.
 *
 * The Function will only be used in this Layer, execpt on probing
 * the Baudrate of Rs232.
 * if quiet is true, the function won't print notes
 */
int mdc800_io_sendCommand_with_retry (char* command, char* buffer, int length, int maxtries,int quiet)
{
	int try=0;
	int retval=0;
	
	while (try < maxtries)
	{
		if (try)
		{
			struct timeval timeout;
			timeout.tv_usec = MDC800_DEFAULT_COMMAND_RETRY_DELAY*1000;   
			timeout.tv_sec  = 0;  
			select (1 , NULL, NULL, NULL, &timeout);
			printCError ("retry Command %i (%i)\n",(unsigned char) command[1], try);
		}

		if (mdc800_io_using_usb)
			retval=mdc800_usb_sendCommand (command, buffer, length );
		else
			retval=mdc800_rs232_sendCommand ( command, buffer, length);
			
		if (retval)
			return 1;
			
		try++;
	}
	
	if (!quiet)
	{
		printCoreNote ("\nCamera is not responding (Maybe off?)\n");
		printCoreNote ("giving it up after %i times.\n\n", try);
	}
	return 0;
}


/*
 * sends a command and receives the answer to this
 * buffer: Strores the answer
 * length: length of answer or null
 */
int mdc800_io_sendCommand (char commandid,char par1,char par2,char par3, char* buffer, int length)
{
	char command [8];
	command[0]=COMMAND_BEGIN;
	command[1]=commandid;
	command[2]=par1;
	command[3]=par2;
	command[4]=par3;
	command[5]=COMMAND_END;
	command[6]=0;
	command[7]=0;
	return mdc800_io_sendCommand_with_retry (command,buffer,length, MDC800_DEFAULT_COMMAND_RETRY,0);
}


/*
 * Send an USB Command if USB is detected
 */
int mdc800_io_sendUSBCommand (char commandid , char b1, char b2, char b3, char b4, char b5, char b6, char* buffer, int length)
{
	char command [8];
	
	if (!mdc800_io_using_usb)
	{
		printAPIError ("USB is not detected\n");
		return 0;
	}
	
	printCError ("(mdc800_io_sendUSBCommand) id:%i (%i,%i,%i,%i,%i,%i) answer:%i\n",(unsigned char) commandid,(unsigned char) b1,(unsigned char) b2,(unsigned char) b3,(unsigned char) b4,(unsigned char) b5,(unsigned char) b6, length);
	command[0]=COMMAND_BEGIN;
	command[1]=commandid;
	command[2]=b1;
	command[3]=b2;
	command[4]=b3;
	command[5]=b4;
	command[6]=b5;
	command[7]=b6;
	return mdc800_io_sendCommand_with_retry ( command, buffer, length,1,0);	
}



/*
 * Helper function for rs232 and usb
 * It returns a timeout for the specified command
 */
int mdc800_io_getCommandTimeout (char command)
{
	switch ((unsigned char) command)
	{
		case COMMAND_SET_STORAGE_SOURCE:
		case COMMAND_SET_TARGET:
		case COMMAND_SET_CAMERA_MODE:
		case COMMAND_DELETE_IMAGE:
			return MDC800_LONG_TIMEOUT;
			
		case COMMAND_TAKE_PICTURE:
		case COMMAND_PLAYBACK_IMAGE:
		case COMMAND_SET_PLAYBACK_MODE:
			return MDC800_TAKE_PICTURE_TIMEOUT;
	}
	return MDC800_DEFAULT_TIMEOUT;
}
