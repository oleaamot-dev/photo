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

/*
	Implemenation of the USB Version of ExecuteCommand
*/
#include "../src/gphoto.h"
#include "print.h"
#include "usb.h"
#include "io.h"
#include "mdc800_spec.h"

#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>


/*
 * Checks wether the camera responds busy
 */
int mdc800_usb_isBusy (char* ch)
{
	int i=0;
	while (i<8)
	{
		if (ch [i] != (char)0x99)
			return 0;
		i++;
	}
	return 1;
}


/*
 * Checks wether the Camera is ready
 */
int mdc800_usb_isReady (char *ch)
{
	int i=0;
	while (i<8)
	{
		if (ch [i] != (char)0xbb)
			return 0;
		i++;
	}
	return 1;

}


/*
 * Waits for the camera 
 * type: 0:  Wait until the camera gets ready
 *       1:  Read data from irq
 * The function stores the readen 8 Bytes in data.
 * and return 0 on success else -1.
 */
int mdc800_usb_readFromIrq (int type, char* data, int timeout)
{
	
#ifdef GPIO_USB
	mdc800_io_device_settings.usb.inep=MDC800_USB_ENDPOINT_STATUS;
#endif
	gpio_set_settings (mdc800_io_device_handle, mdc800_io_device_settings);
	gpio_set_timeout (mdc800_io_device_handle, MDC800_DEFAULT_TIMEOUT);
	/*
	gpio_reset (mdc800_io_device_handle);
	*/
	
	timeout+=2*MDC800_USB_IRQ_INTERVAL;
	
	while (timeout > 0)
	{
		/* try a read */
		if (gpio_read (mdc800_io_device_handle, data,8) != GPIO_OK)
		{
			printCError ("(mdc800_usb_readFromIRQ) reading bytes from irq fails\n");
			return 0;
		}
		

		{
			int i=0;
			printf ("irq :");
			for (i=0; i<8; i++)
			{
				printf ("%i ", (unsigned char)data [i]);
			}
			printf ("\n");
		}

		
		/* check data */
		if (type)
		{
			if (!(mdc800_usb_isReady (data) || mdc800_usb_isBusy (data)))
			{
				/* Data received successfull */
				return 0;
			}
		}
		else
		{
			if (mdc800_usb_isReady (data))
			{
				/* Camera response ready */
				return 0;
			}
		}

		/* wait the speciefied time */
		{
			struct timeval t;
			t.tv_usec = MDC800_USB_IRQ_INTERVAL*1000;   
			t.tv_sec  = 0;  
			select (1 , NULL, NULL, NULL, &t);
			timeout-=MDC800_USB_IRQ_INTERVAL;
		}
	}
	
	/* Timeout */
	printCError ("(mdc800_usb_readFromIrq) timeout\n");
	return -1;
}





int mdc800_usb_sendCommand (char* command, char* buffer, int length)
{
	char tmp_buffer [16];

	printFnkCall ("(mdc800_usb_sendCommand) id:%i (%i,%i,%i,%i,%i,%i),answer:%i\n",command[1],command[2],command[3],command[4],command[5],command[6],command[7],length);		
	if (mdc800_io_device_handle == 0)
	{
		printCError ("(mdc800_usb_sendCommand) Camera is not open !\n");
		return 0;
	}

	/* Send the Command */
#ifdef GPIO_USB
	mdc800_io_device_settings.usb.outep=MDC800_USB_ENDPOINT_COMMAND;
#endif
	gpio_set_settings (mdc800_io_device_handle, mdc800_io_device_settings);
	gpio_set_timeout (mdc800_io_device_handle, MDC800_DEFAULT_TIMEOUT);
	/*
	gpio_reset (mdc800_io_device_handle);
	*/

	
	if (gpio_write (mdc800_io_device_handle, command ,8) != GPIO_OK)
	{
		printCError ("(mdc800_usb_sendCommand) sending Command fails!\n");
		return 0;
	}

	/* receive the answer */
	switch ((unsigned char) command [1])
	{
		case COMMAND_GET_THUMBNAIL:
		case COMMAND_GET_IMAGE:
			
#ifdef GPIO_USB
			mdc800_io_device_settings.usb.inep=MDC800_USB_ENDPOINT_DOWNLOAD;
#endif
			gpio_set_settings (mdc800_io_device_handle, mdc800_io_device_settings);
			gpio_set_timeout (mdc800_io_device_handle, 2000);
			/*
			gpio_reset (mdc800_io_device_handle);
			*/

			update_progress (0);
			
			if (gpio_read (mdc800_io_device_handle, buffer, 64) != GPIO_OK)
			{
				printCError ("(mdc800_usb_sendCommand) requesting 64Byte dummy data fails.\n");
				return 0;
			}
			
			
			/*
			if (gpio_read (mdc800_io_device_handle, buffer, length) != GPIO_OK)
			{
				printCError ("(mdc800_usb_sendCommand) reading image data fails.\n");
				return 0;
			}
			*/
			
			{ // Download loop
				int readen=0;
				while (readen < length)
				{
					if (gpio_read (mdc800_io_device_handle, buffer+readen, 64) != GPIO_OK)
					{
						printCError ("(mdc800_usb_sendCommand) reading image data fails.\n");
						return 0;
					}
					readen+=64;
					if (length)
						update_progress (100 * readen/length);
				}
			}
			break;
	
	
		default :
			if (length > 0)
			{
				int i=0;
				if (mdc800_usb_readFromIrq (1, tmp_buffer, mdc800_io_getCommandTimeout (command[1])))
				{
					/* Reading fails */
					printCError ("(mdc800_usb_sendCommand) receiving answer fails.\n");
					return 0;
				}
				for (i=0; i<length; i++)
					buffer[i]=tmp_buffer[i];
			}
	}
	
	/* Waiting for camera to get ready */
	
	if (mdc800_usb_readFromIrq (0, tmp_buffer, mdc800_io_getCommandTimeout (command[1])))
	{
		/* Reading fails */
		printCError ("(mdc800_usb_sendCommand) camera didn't get ready in the defined intervall ?!\n");
		return 0;
	}
			
	return 1;
}
