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
#include "mdc800.h"
#include "../src/util.h"

#include "mdc800_spec.h"
#include "io.h"
#include "core.h"
#include "print.h"
#include "string.h"
#include "config.h"


static int  mdc800_camera_open=0;
static char mdc800_summary_output [500]; 

/*
 * Init this library.
 * usb: 0: rs232   1:usb
 */
int mdc800_initialize ()
{
	int h,s,e;

	if (mdc800_camera_open)
		return 1;

	printAPINote ("-Init---------------------------------------------------------------------------\n");		
	printAPINote ("Serial Port is \"%s\" \n",serial_port);

	if (!mdc800_openCamera (serial_port))
	{
		printAPIError ("(mdc800_initialize) open camera fails.\n");
		return 0;
	}
	
	printAPINote ("\n");
	mdc800_camera_open=1;
	printAPINote ( mdc800_summary () );
	printAPINote ("\n");
	
	if (mdc800_getRemainFreeImageCount (&h,&s,&e))
		printAPINote ("\nFree Memory for H%i S%i E%i\n",h,s,e);

	printAPINote ("-ok-----------------------------------------------------------------------------\n");		
	return 1;
}


/*
 * The function resets the driver.
 * 
 * It closes the Camera and reopens it.
 */
int mdc800_close ()
{
	printAPINote ("\nClose the Driver.\n\n");
	
	if (mdc800_camera_open)
	{
		mdc800_closeCamera ();
		mdc800_camera_open=0;
	}
	return 1;		
}


/*****************************************************************************
	Begin of GPHOTO API 
 *****************************************************************************/

/*
 * Gives Information about the current camera status.
 */
char* mdc800_summary()
{
	char line[50];
	
	if (!mdc800_camera_open)
	{
		printAPIError ("(mdc800_summary) camera is not open !\n");
		return 0;
	}
	
	strcpy (mdc800_summary_output,"Summary for Mustek MDC800:\n");
	
	if (!mdc800_getSystemStatus ())
	{
		strcat (mdc800_summary_output,"no status reported.");
		mdc800_close ();
		return mdc800_summary_output;
	}
	
	if (mdc800_isCFCardPresent ())
		sprintf (line,"Compact Flash Card detected\n");
	else
		sprintf (line,"no Compact Flash Card detected\n");
	strcat (mdc800_summary_output,line);

	if (mdc800_getMode () == 0)
		sprintf (line, "Current Mode: Camera Mode\n");
	else
		sprintf (line, "Current Mode: Playback Mode\n");
	strcat (mdc800_summary_output,line);


	sprintf (line,mdc800_getFlashLightString (mdc800_getFlashLightStatus ()));
	strcat (line,"\n");
	strcat (mdc800_summary_output,line);
	
	
	if (mdc800_isBatteryOk ())
		sprintf (line, "Batteries are ok.");
	else
		sprintf (line, "Batteries are low.");
	strcat (mdc800_summary_output,line);
	
		
	return mdc800_summary_output;
}


/*
 * Gives Information about the CameraModel and
 * this "driver" .
 */
char* mdc800_description()
{
	return "Mustek MDC-800 gPhoto Library\n"
		    "Henning Zabel <henning@uni-paderborn.de>\n\n"
			 "Supports Serial and USB Protokoll. \n";
}


struct Image* mdc800_get_picture (int picture_number,int thumbnail)
{

	struct Image* bild=0;
	printFnkCall ("(mdc800_get_picture) called \n");

	if (!mdc800_initialize ())
	{
		return 0;
	}

	
	if (thumbnail)
		bild=mdc800_getThumbnail (picture_number);
	else
		bild=mdc800_getImage (picture_number);
	
	if (bild == 0)
	{
		mdc800_close ();
	}

	return bild;
}


int mdc800_delete_image (int pn)
{
	
	printFnkCall ("(mdc800_delete_image) called for Image %i.\n",pn);
	
	if (!mdc800_initialize ())
	{
		return 0;
	}

	printAPINote ("Delete Image %i \n", pn);

	if (!mdc800_setTarget (1))	// Image
	{
		printAPIError ("(mdc800_delete_image) can't set Target\n");
		mdc800_close ();
		return 0;
	}
		
	if (!mdc800_io_sendCommand (COMMAND_DELETE_IMAGE,pn/100,(pn%100)/10,pn%10,0,0))
	{
		printAPIError ("(mdc800_delete_image ) deleting Image %i fails !.\n",pn);
		mdc800_close ();
		return 0;
	}	

	
	return 1;
}


int mdc800_number_of_pictures ()
{
	unsigned char answer [2];
	
	printFnkCall ("(mdc800_number_of_pictures) called.\n");
	
	if (!mdc800_initialize ())
	{
		return 0;
	}

	if (!mdc800_setTarget (1))	// Image
	{
		printAPIError ("(mdc800_number_of_pictures) can't set Target\n");
		mdc800_close ();
		return 0;
	}
	
/*
	if (!mdc800_setMode (1))
	{
		printError ("(mdc800_number_of_pictures) can't set Mode\n");
		mdc800_close ();
		return 0;
	}
*/

	if (!mdc800_io_sendCommand (COMMAND_GET_NUM_IMAGES,0,0,0,(char*)answer,2))
	{
		printAPIError ("(mdc800_getNumberOfImages) request Number of Pictures fails.\n");
		mdc800_close ();
		return 0;
	}	


	return (int)answer[0]*256+answer [1];
}


/*
 * int mdc800_configure () -> config.c
 */


/*
 * Force camera to make a picture in the current quality
 */
int mdc800_take_picture ()
{
	unsigned char answer [2];
	
	printFnkCall ("(mdc800_take_picture) called.\n");

	if (!mdc800_initialize ())
	{
		return 0;
	}

	if (!mdc800_setMode (0)) // Camera Mode
	{
		printAPIError ("(mdc800_take_picture) can't set Camera Mode!\n");
		mdc800_close ();
		return 0;
	}

	if (!mdc800_setTarget (1))	// Image
	{
		printAPIError ("(mdc800_take_pictures) can't set Target\n");
		mdc800_close ();
		return 0;
	}


	if (!mdc800_io_sendCommand (COMMAND_TAKE_PICTURE,0,0,0,0,0))
	{
		printAPIError ("(mdc800_take_picture) take picture fails.\n");
		mdc800_close ();
		return 0;
	}	

	if (!mdc800_setTarget (1))	// Image
	{
		printAPIError ("(mdc800_take_pictures) can't set Target\n");
		mdc800_close ();
		return 0;
	}
		
	// Assume taken picture is the last one !
	if (!mdc800_io_sendCommand (COMMAND_GET_NUM_IMAGES,0,0,0,(char*)answer,2))
	{
		printAPIError ("(mdc800_take_picture) request Number of taken Pictures fails.\n");
		mdc800_close ();
		return 0;
	}	
	
	
	return (int)answer[0]*256+answer [1];
	
}


/*
 * Get a preview from the Camera.
 * 
 * Function return null, if no picture was taken, otherwise
 * the picture ..
 */
struct Image* mdc800_get_preview ()
{
/*
	struct Image* image=null;
	char buffer [320*1024];
	int i;
	int pic_size=32*1024;
	
	if (!mdc800_io_sendUSBCommand (0x3f, 1,0, 0,1,0,0, 0, 0))
	{
		printError ("can't set camera to VCAM Mode \n");
		return 0;
	}
	
	
	if (!mdc800_io_sendUSBCommand (0x3e,pic_size/256,pic_size%256,0,0,0,0,buffer,pic_size))
	{
		printError ("can't take picture \n");
		return 0;	
	}

	image = (struct Image *) malloc (sizeof(struct Image));
	image->image_size=pic_size;
	image->image=(unsigned char *) malloc (pic_size);
	for (i=0; i<pic_size; i++)
		image->image[i]=buffer [i];
	strcpy(image->image_type,"jpg");
   image->image_info_size = 0;
	correctImageData (image->image,0,0,1);
	
	return image;
*/

	struct Image* pic=null;
	int num=mdc800_take_picture ();
	if (num == 0)
	{
		printAPIError ("(mdc800_get_preview) taking picture fails.\n");
		mdc800_close ();
		return null;
	}
	pic=mdc800_get_picture (num,1);  // Thumbnail
	if (pic == null)
	{
		printAPIError ("(mdc800_get_preview) there's no picture ??\n");
		mdc800_close ();
		return 0;
	}
	
	if (!mdc800_delete_image (num))
	{
		mdc800_close ();
		printAPIError ("(mdc800_get_preview) can't delete taken picture (%i)\n",num);
		return 0;
	}
	
	return (pic);
}


/*
 * Spezial USB and rs232 Initialisation
 * Camera structs :
 */
int mdc800_initialize_rs232 ()
{
	mdc800_io_using_usb=0;
	return mdc800_initialize ();
}

struct _Camera mustek_mdc800_rs232 = 
{
	mdc800_initialize_rs232 ,
	mdc800_get_picture,
	mdc800_get_preview,
	mdc800_delete_image,
	mdc800_take_picture,
	mdc800_number_of_pictures,
	mdc800_configure,
	mdc800_summary,
	mdc800_description,
	NULL
};


int mdc800_initialize_usb ()
{
	mdc800_io_using_usb=1;
	return mdc800_initialize ();
}

struct _Camera mustek_mdc800_usb = 
{
	mdc800_initialize_usb ,
	mdc800_get_picture,
	mdc800_get_preview,
	mdc800_delete_image,
	mdc800_take_picture,
	mdc800_number_of_pictures,
	mdc800_configure,
	mdc800_summary,
	mdc800_description,
	NULL
};
