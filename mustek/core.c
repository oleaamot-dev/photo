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
 * supports rs232 and USB.
 */

#include "core.h"
#include "../src/gphoto.h"
#include <termios.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include "print.h"

//------------- Global System Data ----------------------------------------/

static char mdc800_system_flags [4];			// System flags
static int  mdc800_system_flags_valid=0;     // Flags ok ?

//--- These values are "cached" to decrease communication ------------------/
static int mdc800_memory_source=-1;			// Set by setStorageSource

static int mdc800_baud_rate=1;		// Current Baudrate
												// 0: 19200, 1:57600, 2: 115200
//--------------------------------------------------------------------------/


//----------------------  Funktion for Communication --------------------------/


/*
 * Send the Initial Command. If the device is rs232 the
 * Function will probe for the Baudrate
 */
int mdc800_sendInitialCommand (char* answer)
{
	char* baud_string [3]={"19200","57600","115200"};
	
	int try_next=3;

	mdc800_baud_rate=1;

	while (try_next)
	{
		char command [8]={COMMAND_BEGIN,COMMAND_INIT_CONNECT,0,0,0,COMMAND_END,0,0};
		
		if (mdc800_io_using_usb)
			try_next=0;
			
		if (mdc800_io_changespeed (mdc800_baud_rate))
		{
			if (mdc800_io_sendCommand_with_retry (command,answer,8,1,1))
			{
				if (!mdc800_io_using_usb)
				{
					printCoreNote ("RS232 Baudrate probed at %s \n",baud_string [mdc800_baud_rate]);
				}
				try_next=0;
				return 1;
			}
		}
		
		if (try_next)
		{
			try_next--;
			
			printCoreError ("Probing RS232 Baudrate with %s fails.\n", baud_string [mdc800_baud_rate]);
			mdc800_baud_rate=(mdc800_baud_rate+2)%3;
			if (!try_next)
			{
				printCoreError ("Probing fails completly.\n");
				return 0;
			}
		}
	}
	return 0;
}



/*
 * Opens the Camera:
 * (1) open serial device 		(2) -
 * (3) send Initial command	
 */
int mdc800_openCamera (char* device)
{
	char answer [8];
	int  i;
	
	if (!mdc800_io_openDevice (device))
		return 0;

	if (mdc800_io_using_usb)
	{
		printCoreNote ("Device Registered as USB.\n");
	}
	else
	{
		printCoreNote ("Device Registered as RS232. \n");
	}
		


	/* Send initial command */
	if (!mdc800_sendInitialCommand (answer))
	{
		printCoreError ("(mdc800_openCamera) can't send initial command.\n");
		mdc800_io_closeDevice ();
		return 0;
	}

	printCoreNote ("Firmware info (last 5 Bytes) : ");

	for (i=0; i<8; i++)
		printCoreNote ("%i ",(unsigned char) answer[i]);
	printCoreNote ("\n");

	mdc800_system_flags_valid=0;	
	//mdc800_memory_source=-1;

	if (!mdc800_setDefaultStorageSource ())
	{
		printCoreError ("(mdc800_openCamera) can't set Storage Source.\n");
		mdc800_io_closeDevice ();
		mdc800_baud_rate=1;
	}
		
	return 1;
}



/*
 * Send closing Command and close the device
 */
int mdc800_closeCamera ()
{
	//mdc800_io_sendCommand (COMMAND_DISCONNECT,0,0,0,0,0);
	//mdc800_baud_rate=1;
	mdc800_system_flags_valid=0;
	
	return mdc800_io_closeDevice ();
}


/*
 * Sets the camera speed to the defined value: 
 * 0: 19200, 1:57600, 2: 115200
 */
int mdc800_changespeed (int new)
{
	char* baud_string [3]={"19200","57600","115200"};
	
	printFnkCall ("(mdc800_changespeed) called.\n");
	
	if (mdc800_baud_rate == new)
		return 1;
	
	/* We are using USB, so we don't need to change BaudRates */
	if (mdc800_io_using_usb)
		return 1;

	/* Setting comunication speed */
	if (!mdc800_io_sendCommand (COMMAND_CHANGE_RS232_BAUD_RATE,new,mdc800_baud_rate,0,0,0))
	{
		printCoreError ("(mdc800_changespeed) can't send first command.\n");
		return 0;
	}

	if (!mdc800_io_changespeed (new))
	{
		printCoreError ("(mdc800_changespeed) Changing Baudrate fails.\n");
		return 0;
	}

	/* Second Command */
	if (!mdc800_io_sendCommand (COMMAND_CHANGE_RS232_BAUD_RATE,new,new,0,0,0))
	{
		printCoreError ("(mdc800_changespeed) can't send second command.\n");
		return 0;
	}

	mdc800_baud_rate=new;
	printCoreNote ("Set Baudrate to %s\n", baud_string [new]);

	return 1;
}


/*
 * Get the current Bautrate
 */
int mdc800_getSpeed ()
{
	return mdc800_baud_rate;
}


/*
 * Sets Target
 * 1: image, 2:thumbnail, 3:video, 4:not setting
 */
int mdc800_setTarget (int value)
{
	printFnkCall ("(mdc800_setTarget) called. \n");

	if (!mdc800_io_sendCommand (COMMAND_SET_TARGET,value,0,0,0,0))
	{
		printCoreError ("(mdc800_setTarget): sending command fails!\n");
		return 0;
	}

	return 1;
}


/*
 * Loads a thumbnail from the cam .
 */
struct Image* mdc800_getThumbnail (int index)
{
	char buffer [4096];
	int i=0;
	struct Image* image;
	
	printFnkCall ("(mdc800_getThumbNail) called for %i . \n",index);
	
	if (!mdc800_io_sendCommand (COMMAND_GET_THUMBNAIL,index/100,(index%100)/10,index%10,buffer,4096))
	{
		printCoreError ("(mdc800_getThumbNail) can't get Thumbnail.\n");
		return 0;	
	}
	
	image = (struct Image *) malloc (sizeof(struct Image));
	image->image_size=4096;
	image->image=(unsigned char *) malloc (4096);
	for (i=0; i<4096; i++)
		image->image[i]=buffer [i];
	strcpy(image->image_type,"jpg");
   image->image_info_size = 0;
	mdc800_correctImageData (image->image,1,0,mdc800_memory_source == 1);
	
	return image;
}



/*
 * Load an Image from the cam ..
 */
struct Image* mdc800_getImage (int index)
{
	char buffer [348160];
	int imagequality=-1;
	int imagesize=0,i;
	struct Image* image=0;
	
	printFnkCall ("(mdc800_getImage) called for %i . \n",index);

	
	if (!mdc800_setTarget (1))
	{
		printCoreError ("(mdc800_getImage) can't set Target. \n");
		return 0;  
	}
	
	
	if (!mdc800_io_sendCommand (COMMAND_GET_IMAGE_SIZE,index/100,(index%100)/10,index%10,buffer,3))
	{
		printCoreError ("(mdc800_getImage) request for Imagesize of %i fails.\n",index);
		return 0;
	}
	
	imagesize=(int) ((unsigned char) buffer[0])*65536+((unsigned char)buffer[1])*256+(unsigned char)buffer[2];
	printCoreNote ("Imagesize of %i is %i ",index,imagesize);
	switch (imagesize/1024)
	{
		case 48: 	
			imagequality=0; 
			printCoreNote ("(Economic Quality 506x384)\n");
			break;
		case 128:	
			imagequality=1; 
			printCoreNote ("(Standard Quality 1012x768)\n");
			break;
		case 320:  
			imagequality=2; 
			printCoreNote ("(High Quality 1012x768)\n");
			break;
		case 4:
			printCoreNote ("(ThumbNail ? 112x96)\n");
			imagequality=-1;
			break;
		default:
			printCoreNote ("(not detected)\n");
			return 0;
	}
	
	
	
	if (!mdc800_io_sendCommand (COMMAND_GET_IMAGE,index/100,(index%100)/10,index%10,buffer,imagesize))
	{
		printCoreError ("(mdc800_getImage) request fails for Image %i.\n",index);
		return 0;	
	}
	
	image = (struct Image *) malloc (sizeof(struct Image));
	image->image_size=imagesize;
	image->image=(unsigned char *) malloc (imagesize);
	for (i=0; i<imagesize; i++)
		image->image[i]=buffer [i];
   strcpy(image->image_type,"jpg");
   image->image_info_size = 0;
	
	mdc800_correctImageData (image->image,imagequality == -1,imagequality,mdc800_memory_source == 1);

	return image;

}



//------	SystemStatus of the Camera ------------------------------------------/


// Load the Status from the camera if necessary
int mdc800_getSystemStatus ()
{
	if (mdc800_system_flags_valid)
		return 1;
	mdc800_system_flags_valid=0;
	if (!mdc800_io_sendCommand (COMMAND_GET_SYSTEM_STATUS,0,0,0,mdc800_system_flags,4))
	{
		printCoreError ("(mdc800_getSystemStatus) request fails.\n");
		return 0;	
	}
	mdc800_system_flags_valid=1;
	return 1;
}


int mdc800_isCFCardPresent ()
{
	mdc800_getSystemStatus ();
	if (mdc800_system_flags_valid)
		return (((unsigned char)mdc800_system_flags[0]&1) == 0);
	else
	{
		printCoreError ("(mdc800_isCFCardPresent) detection fails.\n");
		return 0;
	}
}


int mdc800_isBatteryOk ()
{
	mdc800_getSystemStatus ();
	return (((unsigned char)mdc800_system_flags[0]&4) == 0)?1:0;
}


/*
 * Gets CamMode.
 * 0: Camera, 1: Playback, 2:VCam
 */
int mdc800_getMode ()
{
	mdc800_getSystemStatus ();
	if (((unsigned char)mdc800_system_flags[1]&16) == 0)
		return (((unsigned char)mdc800_system_flags[1]&32) == 0)?1:0;
	else
		return 2;
}


/*
 * Return status of Flashlight. The
 * answer is one of MDC800_FLASHLIGHT_ Mask.
 */
int mdc800_getFlashLightStatus ()
{
	mdc800_getSystemStatus ();
	return ((unsigned char)mdc800_system_flags[3]&7);
}


int mdc800_isLCDEnabled ()
{
	mdc800_getSystemStatus ();
	return (((unsigned char)mdc800_system_flags[1]&4) == 4);
}


int mdc800_isMenuOn ()
{
	mdc800_getSystemStatus ();
	return (((unsigned char)mdc800_system_flags[1]&1) == 1);
}

int mdc800_isAutoOffEnabled ()
{
	mdc800_getSystemStatus ();
	return (((unsigned char)mdc800_system_flags[1]&8) == 8);
}

//----- Other fine functions-------------------------------------------------/


/*
 * Sets Source of Images :
 * 0: FlashCard, 1: Internal Memory
 */
int mdc800_setStorageSource (int flag)
{
	if (flag == mdc800_memory_source)
		return 1;
	
	/* Check wether FlashCard is present */
	if ((flag == 0) && !mdc800_isCFCardPresent ())
	{
		printCoreNote ("There's is no FlashCard in the Camera !\n");
		return 1;
	}
	printFnkCall ("(mdc800_setStorageSource) called with flag=%i\n",flag);
	if (!mdc800_io_sendCommand (COMMAND_SET_STORAGE_SOURCE,flag,0,0,0,0))
	{
		if (flag)
		{
			printCoreError ("Can't set InternalMemory as Input!\n");
		}
		else
		{
			printCoreError ("Can't set FlashCard as Input!\n");
		}
		return 0;
	}

	printCoreNote ("Storage Source set to ");
	if (flag)
	{
		printCoreNote ("Internal Memory \n");
	}
	else
	{
		printCoreNote ("Comact Flash Card \n");
	}

	mdc800_system_flags_valid=0;
	mdc800_memory_source=flag;
	return 1;
}


/*
 * Sets the default storage source.
 * Default means, that if there's a FlashCard, the Flashcard
 * is used, else the InternalMemory
 *
 * If mdc800_memory_source is set ( after driver has been closed ),
 * this value will be used.
 */
int mdc800_setDefaultStorageSource ()
{
	int source;
	
	if (mdc800_memory_source != -1)
	{
		source=mdc800_memory_source;
		mdc800_memory_source=-1;
	}
	else
	{
		source=mdc800_isCFCardPresent ()?0:1;
	}
			
	if (!mdc800_setStorageSource (source))
	{
		printCoreError ("(mdc800_setDefaultStorageSource) Setting Storage Source fails\n");
		return 0;
	}

	return 1;
}


/*
 *	Returns what StorageSource is selected by the driver
 * 0: FlashCard, 1: Internal
 */
int mdc800_getStorageSource ()
{
	if (mdc800_memory_source == -1)
		mdc800_setDefaultStorageSource ();
	return mdc800_memory_source;
}



/*
 * Sets Camera to Camera- or PlaybackMode
 * m: 0 Camera, 1 Playback, 2:VCam (USB)
 */
int mdc800_setMode (int m)
{
	int last=mdc800_getMode ();
/*
	if (mdc800_getMode () == m)
		return 1;
*/		
	switch (m)
	{
		case 0:
			if (!mdc800_io_sendCommand (COMMAND_SET_CAMERA_MODE,0,0,0,0,0))
			{
				printCoreError ("(mdc800_setMode) setting Camera Mode fails\n");
				return 0;
			}
			if (last != m)
				printCoreNote ("Mode set to Camera Mode.\n");
			break;
			
		case 1:
			if (!mdc800_io_sendCommand (COMMAND_SET_PLAYBACK_MODE,0,0,0,0,0))
			{
				printCoreError ("(mdc800_setMode) setting Playback Mode fails\n");
				return 0;
			}
			if (last != m)
				printCoreNote ("Mode set to Payback Mode.\n");
			break;
			
	}
	mdc800_system_flags_valid=0;
	return 1;
}


/*
 * Sets up Flashlight. The waitForCommit waits a long
 * time, to give the camera enough time to load the
 * flashlight.
 */
int mdc800_setFlashLight (int value)
{
	int command=0;
	int redeye_flag=0;
	
	if (mdc800_getFlashLightStatus () == value)
		return 1;
	
	redeye_flag=(value&MDC800_FLASHLIGHT_REDEYE) != 0;
	
	if ((value&MDC800_FLASHLIGHT_ON) != 0)
		command=COMMAND_SET_FLASHMODE_ON;
	else if ((value&MDC800_FLASHLIGHT_OFF) != 0)
	{
		command=COMMAND_SET_FLASHMODE_OFF;
		redeye_flag=0;
	}
	else
		command=COMMAND_SET_FLASHMODE_AUTO;
		
	
	mdc800_system_flags_valid=0;
	if (!mdc800_io_sendCommand (command,redeye_flag,0,0,0,0))
	{
		printCoreError ("(mdc800_setFlashLight) sending command fails.\n");
		return 0;
	}
	
	
	printCoreNote (mdc800_getFlashLightString (value));
	printCoreNote ("\n");
		
	return 1;
}



/*
 * Gets a String with the Text of the Flashlight-Status
 * depending on value
 */
char* mdc800_getFlashLightString (int value)
{
	switch (value)
	{
		case ( MDC800_FLASHLIGHT_REDEYE | MDC800_FLASHLIGHT_AUTO ) :
			return "FlashLight : Auto (RedEye Reduction)";
		case MDC800_FLASHLIGHT_AUTO :
			return "FlashLight : Auto";
		case ( MDC800_FLASHLIGHT_REDEYE | MDC800_FLASHLIGHT_ON ) :
			return "FlashLight : On (RedEye Reduction)";
		case MDC800_FLASHLIGHT_ON :
			return "FlashLight : On";
		case MDC800_FLASHLIGHT_OFF :
			return "FlashLight : Off";
	}
	return "FlashLight : undefined";
}


/*
 * Enable/Disable the LCD
 */
int mdc800_enableLCD (int enable)
{
	int command;
	if (enable == mdc800_isLCDEnabled ())
		return 1;
	
	if (enable) 
		command=COMMAND_SET_LCD_ON;
	else
		command=COMMAND_SET_LCD_OFF;

	mdc800_system_flags_valid=0;	
	if (!mdc800_io_sendCommand (command,0,0,0,0,0))
	{
		printCoreError ("(mdc800_enableLCD) can't enable/disable LCD\n");
		return 0;
	}
			
	if (enable)
	{
		printCoreNote ("LCD is enabled\n");
	}
	else
	{
		printCoreNote ("LCD is disabled\n");
	}
		
	return 1;
}


/*
 * Shows the specified Image, the Camera has to
 * be in Playback Mode !
 */
int mdc800_playbackImage (int index )
{
	if (mdc800_getMode () != 1)
	{
		printCoreError ("(mdc800_showImage) camera must be in Playback Mode !");
		return 0;
	}

	if (!mdc800_io_sendCommand (COMMAND_PLAYBACK_IMAGE,index/100,(index%100)/10,index%10,0,0))
	{
		printCoreError ("(mdc800_showImage) can't playback Image %i \n",index);
		return 0;
	}
	
	return 1;
}


/*
 * With ths function you can get information about, how many
 * pictures can be stored in the free memory of the camera.
 *
 * h: High Quality, s: Standard Quality, e: Economy Quality
 * If one of these Pointers are 0 the will be ignored.
 */
int mdc800_getRemainFreeImageCount (int* h,int* s,int *e)
{
	unsigned char data [6];
	
	if (!mdc800_io_sendCommand (COMMAND_GET_REMAIN_FREE_IMAGE_COUNT,0,0,0,(char*)data,6))
	{
		printCoreError ("(mdc800_getRemainFreeImageCount) Error sending Command.\n");
		return 0;
	}
	
	if (h != 0)
		(*h)=(int)((data[0]/16)*1000)+((data[0]%16)*100)+((data[1]/16)*10)+(data[1]%16);
	if (s != 0)
		(*s)=(int)((data[2]/16)*1000)+((data[2]%16)*100)+((data[3]/16)*10)+(data[3]%16);
	if (e != 0)
		(*e)=(int)((data[4]/16)*1000)+((data[4]%16)*100)+((data[5]/16)*10)+(data[5]%16);
	return 1;
}


/*
 * Get Image Quallity 
 * 0: Economic, 1:Standard, 2:High, -1 for Error
 */
int mdc800_getImageQuality ()
{
	char retval;
	if (mdc800_io_sendCommand (COMMAND_GET_IMAGE_QUALITY,0,0,0,&retval,1))
	{
		return retval;
	}
	printCoreError ("(mdc800_getImageQuality) fails.\n");
	return -1;
}


/*
 *	Set Image Quality, return 1 if ok.
 */
int mdc800_setImageQuality (int v)
{
	if (mdc800_io_sendCommand (COMMAND_SET_IMAGE_QUALITY,v,0,0,0,0))
	{
		return 1;
	}
	printCoreError ("(mdc800_setImageQuality) fails.\n");
	return 0;
}



/*
 * Set the WhiteBalance value
 * 1:auto ,2:indoor, 4:indoor with flashlight, 8:outdoor
 */
int mdc800_setWB (int v)
{
	if (mdc800_io_sendCommand (COMMAND_SET_WB,v,0,0,0,0))
	{
		return 1;
	}
	printCoreError ("(mdc800_setWB) fails.\n");
	return 0;
}


/*
 * Return the Exposure settings and W.B.
 */
int mdc800_getWBandExposure (int* exp, int* wb)
{
	char retval[2];
	
	/* What's that here is a real diffenrence between USB and RS232 */
	int toggle=mdc800_io_using_usb;
	
	if (mdc800_io_sendCommand (COMMAND_GET_WB_AND_EXPOSURE,0,0,0,retval,2))
	{
		(*exp)=(unsigned char) retval[toggle]-2;
		(*wb)=(unsigned char) retval[1-toggle];
		return 1;
	}
	printCoreError ("(mdc800_getWBandExposure) fails.\n");
	return 0;
}


/*
 * Sets the Exposure Value
 */
int mdc800_setExposure (int v)
{
	if (mdc800_io_sendCommand (COMMAND_SET_EXPOSURE,v+2,0,0,0,0))
	{
		return 1;
	}
	printCoreError ("(mdc800_setExposure) fails.\n");
	return 0;
}

/*
 * Sets the Exposure Mode 
 * 0: MTRX 1:CNTR
 */
int mdc800_setExposureMode (int m)
{
	if (mdc800_io_sendCommand (COMMAND_SET_EXPOSURE_MODE,m,0,0,0,0))
	{
		return 1;
	}
	printCoreError ("(mdc800_setExposureMode) fails.\n");
	return 0;
}


/*
 * return the Exposure Mode or -1
 */
int mdc800_getExposureMode ()
{
	char retval;
	if (mdc800_io_sendCommand (COMMAND_GET_EXPOSURE_MODE,0,0,0,&retval,1))
	{
		return (unsigned char) retval;
	}
	printCoreError ("(mdc800_getImageQuality) fails.\n");
	return -1;
}


/*
 * Enable, Disable the Menu 
 */
int mdc800_enableMenu (int enable)
{
	char command=enable?COMMAND_SET_MENU_ON:COMMAND_SET_MENU_OFF;
	
	if (enable == mdc800_isMenuOn ())
		return 1;

	mdc800_system_flags_valid=0;	
	
	if (mdc800_io_sendCommand (command,0,0,0,0,0))
	{
		return 1;
	}
	printCoreError ("(mdc800_enableMenu) fails.\n");
	return 0;
}
