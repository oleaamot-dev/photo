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

 */

#ifndef SERIO_H__
#define SERIO_H__



#if !defined(FALSE)
#	define FALSE 0
#endif

#if !defined(TRUE)
#	define TRUE 1
#endif


/*
*  serio.c
*/
int InitSonyDSCF55(char *serial_port);
void CloseSonyDSCF55(void);

int Write(unsigned char *buffer, int length);
int Read(unsigned char *buffer, int *length);

void DumpData(unsigned char *buffer, int length);

int SetSpeed(int speed);
int dscSetSpeed(int);



typedef struct _tagPacket
{
	int valid;
	int length;
	unsigned char buffer[16384];
	unsigned char checksum;
} Packet;

/*
*  config.c
*/
int ConfigDSCF55Speed();

#endif SERIO_H__
