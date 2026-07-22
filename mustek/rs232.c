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
 
#include "../src/gphoto.h"
#include "print.h"
#include "rs232.h"
#include "mdc800_spec.h"
#include "io.h"

#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>


/*
 * sends a command and receives the answer to this
 * buffer: Strores the answer
 * length: length of answer or null
 */
int mdc800_rs232_sendCommand (char* command, char* buffer, int length)
{
	char answer;
	int fault=0;
	struct timeval timeout;
	int i;
	
	if (mdc800_io_device_handle == 0)
	{
		printCError ("(mdc800_rs232_sendCommand) Camera is not open !\n");
		return 0;
	}


	printFnkCall ("(mdc800_rs232_sendCommand) id:%i (%i,%i,%i),answer:%i\n",command[1],command[2],command[3],command[4],length);	


	timeout.tv_usec = MDC800_DEFAULT_COMMAND_DELAY*1000;   
	timeout.tv_sec  = 0;  
	select (1 , NULL, NULL, NULL, &timeout);

	gpio_set_timeout (mdc800_io_device_handle, MDC800_DEFAULT_TIMEOUT );

	// Send command
	for (i=0; i<6; i++)
	{
		if (gpio_write (mdc800_io_device_handle, &command[i] ,1) != GPIO_OK)
		{
			printCError ("(mdc800_rs232_sendCommand) sending Byte %i fails!\n",i);
			fault=1;
		}

		if (gpio_read (mdc800_io_device_handle,&answer,1) != GPIO_OK)
		{
			printCError ("(mdc800_rs232_sendCommand) receiving resend of Byte %i fails.\n",i);
			fault=1;
		}

		if (command [i] != answer)
		{
			printCError ("(mdc800_rs232_sendCommand) Byte %i differs : send %i, received %i \n",i,command[i],answer);
			fault=1;
		}
	}
	if (fault)
		return 0;

	// Receive answer
	if (length)
	{
		// Some Commands needs a download.
		switch (command[1])
		{
			case COMMAND_GET_IMAGE:
			case COMMAND_GET_THUMBNAIL:
				if (!mdc800_rs232_download (buffer,length))
				{
					printCError ("(mdc800_rs232_sendCommand) download of %i Bytes fails.\n",length);
					fault=1;
				}
				break;
			default:
				if (!mdc800_rs232_receive (buffer,length))
				{
					printCError ("(mdc800_rs232_sendCommand) receiving %i Bytes fails.\n",length);
					fault=1;
				}
		}
	}

	if (fault)
		return 0;

	// commit 
	if (!(command[1] == COMMAND_CHANGE_RS232_BAUD_RATE))
		if (!mdc800_rs232_waitForCommit (command[1]))
		{
			printCError ("(mdc800_rs232_sendCommand) receiving commit fails.\n");
			fault=1;
		}

	if (fault)
		return 0;

	// all right
	return 1;
}


/*
 * waits for the Commit value
 * The function expexts the commandid auf the corresponding command
 * to determiante, wether a long timeout is needed or not. (Take Photo)
 */
int mdc800_rs232_waitForCommit (char commandid)
{
	char ch[1];
	
	if (mdc800_io_device_handle == 0)
	{
		printCError ("(mdc800_rs232_waitForCommit) Camera is not open !\n");
		return 0;
	}

	gpio_set_timeout (mdc800_io_device_handle, mdc800_io_getCommandTimeout (commandid) );
	
	if (gpio_read (mdc800_io_device_handle,ch,1) != GPIO_OK)
	{
		printCError ("(mdc800_rs232_waitForCommit) Error receiving commit !\n");
		return 0;
	}
	
	if (ch[0] != ANSWER_COMMIT )
	{
		printCError ("(mdc800_rs232_waitForCommit) Byte \"%i\" was not the commit !\n",ch[0]);
		return 0;
	}
	return (1);
}


/*
 * receive Bytes from camera
 */
int mdc800_rs232_receive (char* buffer, int b)
{
	if (mdc800_io_device_handle == 0)
	{
		printCError ("(mdc800_rs232_receive) Camera is not open !\n");
		return 0;
	}
	
	gpio_set_timeout (mdc800_io_device_handle, MDC800_DEFAULT_TIMEOUT );
	
	if (gpio_read (mdc800_io_device_handle, buffer,b) != GPIO_OK)
	{
		printCError ("(mdc800_rs232_receive) can't read %i Bytes !\n",b);
		return 0;
	}
	return 1;
}


/*
 * downloads data from camera and send
 * a checksum every 512 bytes.
 */
int mdc800_rs232_download (char* buffer, int size)
{
	int checksum,readen=0,i;
	char DSC_checksum;
	int numtries=0;
	
	gpio_set_timeout (mdc800_io_device_handle, MDC800_DEFAULT_TIMEOUT );
	
	while (readen < size)
	{
		if (size)
			update_progress (100 * readen / size);
		if (!mdc800_rs232_receive (&buffer[readen],512))
			return readen;
		checksum=0;
		for (i=0; i<512; i++)
			checksum=(checksum+(unsigned char) buffer [readen+i])%256;
		if (gpio_write (mdc800_io_device_handle,(char*) &checksum,1) != GPIO_OK)
			return readen;
		
		if (!mdc800_rs232_receive (&DSC_checksum,1))
			return readen;
			

		if ((char) checksum != DSC_checksum)
		{
			numtries++;
			printCError ("(mdc800_rs232_download) checksum: software %i, DSC %i , reload block! (%i) \n",checksum,(unsigned char)DSC_checksum,numtries);
			if (numtries > 10)
			{
				printCError ("(mdc800_rs232_download) to many retries, giving up..");
				return 0;
			}
		}
		else
		{
			readen+=512;
			numtries=0;
		}
	}
	

	{
		int i,j;
		unsigned char* b=(unsigned char*) buffer;
		for (i=0; i<4; i++)
		{
			printCError ("%i: ",i);
			for (j=0; j<8; j++)
				printCError (" %i", b[i*8+j]);
			printCError ("\n");
		}
	}

	if (size)
		update_progress (100 * readen/size);
	return readen;
}
