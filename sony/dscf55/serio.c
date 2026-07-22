/*


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    http://www.gnu.org/copyleft/gpl.html

 */

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "serio.h"

#if defined(OS2)
#  include <i86.h>
#  define	usleep(a)	delay((a/1000))
#endif


static int dscf55_fd;
static struct	termios local;
static struct	termios master;



#ifndef __linux__
#ifdef __sun__
int cfmakeraw(struct termios *termios_p)
{

  /* code taken from Redhat 2.0.37 Manual, jw */
  termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
  termios_p->c_oflag &= ~OPOST;
  termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
  termios_p->c_cflag &= ~(CSIZE|PARENB);
  termios_p->c_cflag |= CS8;

  return 0;
}
#endif
#endif

/***************************************************************
*
*
*/
int TransferRateID(int baud)
{
	int r = 0;

	switch (baud)
	{
		case B115200:
		 	r = 4;
			break;
		case B57600:
			r = 3;
			break;	/* FIXME ??? */
		case B38400:
			r = 2;
			break;	/* works on sun */
		case B19200:
			r = 1;
			break;	/* works on sun */
		default:
			case B9600:
			r = 0; break;	/* works on sun */
	}

	return r;
}


/***************************************************************
*
*
*/
int InitSonyDSCF55(char *devicename)
{
	char	buffer[256];

	dscf55_fd = open(devicename, O_RDWR|O_NOCTTY);

	if(-1 != dscf55_fd)
	{
		if(tcgetattr(dscf55_fd, &master)== -1)
		{
			perror("tcgetattr failed\n");
			return FALSE;
		}

		memcpy(&local, &master, sizeof(struct termios));
		cfmakeraw(&local);

		local.c_cc[VMIN] = 0;
		local.c_cc[VTIME] = 5;

		if(!dscSetSpeed(B9600))
		{
			perror("dscSetSpeed");
		}

		while(read(dscf55_fd, buffer, 256)>0);

		return TRUE;
	}

	printf("return with FALSE\n");

	return FALSE;
}


/***************************************************************
*
*
*/
void CloseSonyDSCF55()
{
	if(-1 != dscf55_fd)
	{
		close(dscf55_fd);
	}
}


/***************************************************************
*
*
*/
int dscSetSpeed(int speed)
{
	usleep(50000);

	switch(speed)
	{
		case B115200:
    		case B57600:
		case B38400:
		case B19200:
			cfsetispeed(&local, speed);
			cfsetospeed(&local, speed);
			break;
		default:
			cfsetispeed(&local, B9600);
			cfsetospeed(&local, B9600);
			break;
	}

	return !tcsetattr(dscf55_fd, TCSANOW, &local);
}


/***************************************************************
*
*
*/
int extRead(unsigned char *buffer, int *length)
{
	int len = read(dscf55_fd, buffer, *length);

	if(len < 0)
	{
		perror("Read failed\n");
		*length = 0 ;
	}

	return len;
}


/***************************************************************
*
*
*/
int ReadCommByte(unsigned char *byte)
{
  static unsigned char buf[256];
  static int bytes_read = 0;
  static int bytes_returned = 0;

  if (bytes_returned < bytes_read)
    {
      *byte = buf[bytes_returned++];
      return 1;
    }

  if ((bytes_read = read(dscf55_fd, buf, sizeof(buf))) < 0)
    perror("ReadCommByte failed\n");

  bytes_returned = 0;
  if (bytes_read) *byte = buf[bytes_returned++];
  return (bytes_read > 0) ? 1 : bytes_read;
}


/***************************************************************
*
*
*/
int extWrite(unsigned char *buffer, int length)
{
	int bytecount;

	for(bytecount=0; bytecount<length; bytecount++)
	{
/*
		if(length==2)
			printf("{%u}",(unsigned char)*(buffer+bytecount));
*/

		if(write(dscf55_fd, buffer+bytecount, 1) != 1)
		{
			printf("Write failed\n");
			break;
		}
	}

	return bytecount;
}


/***************************************************************
*
*
*/
void DumpPacket(Packet *p)
{
	DumpData(p->buffer, p->length);
}


/***************************************************************
*
*
*/
void DumpData(unsigned char *buffer, int length)
{
	int n=0;

	printf("Dumping :");

	for(n=0; n<length; n++)
	{
		printf("%u ", (int) ((unsigned char )buffer[n]));
	}

	fflush(stdout);
}

