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

/*********************
 * 0.3.1: 30.12.99, Juergen Weigert <jw@netvision.de>
 * 	ported to solaris, gcc -Wall clean.
 *	existing files are skipped per default, more command line options 
 *	for standalone apllication, mpeg download added, resetting sequence 
 *	number on exit.
 *
 * 0.4.1: 16.01.00, Bernd Seemann <bernd@seebaer.ruhr.de>
 *	fixed thumbnail support for MSAC-SR1 and Memory Sticks used by
 *	DCR-PC100 (missing JPEG signature). Don't know whether this is
 *	a bug or feature of the MSAC-SR1 or the DCR-PC100. So I have added
 *	a new camera model for gphoto until this is checked by someone.
 *
 *	fixed invalid sequence error for MSAC-SR1. This error produces one
 *	"Invalid Sequence" and 9 "Checksum invalid" errors in Converse().
 *	So I simple make new packets that will automagically will get the
 *	next sequence number and try again.
 *
 * 0.4.2: 08.02.00 Mark Davies <mdavies@dial.pipex.com>
 *	Ported stand alone to OS/2
 *	Corrected a few potential memory problems.
 *
 * 0.4.3: 23.03.00 Mark Davies <mdavies@dial.pipex.com>
 *	Added code so that it compiles on WIN32 platforms.
 *
 *
 */

#if !defined(OS2) && !defined(_WIN32)
#  include <sys/ioctl.h>
#  include <termios.h>
#else
# if defined(_WIN32)
#  include <windows.h>
#  include <time.h>
# endif
#endif // OS2 && _Win32
#include <stdio.h>
#if !defined(_WIN32)
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>	/* for tolower(), jw */
#include <errno.h>

#include "config.h"	
#include "serio.h"

#if !defined(STAND_ALONE)
# if !defined(OS2) && !defined(_WIN32)
#   include "../../src/gphoto.h"
#   include "../../src/util.h"
# endif /* !OS2 */
#else /* STAND_ALONE */
# include <signal.h>
# if defined(OS2) || defined(_WIN32)
#  if !defined(_WIN32)
#   include <i86.h>
#  endif
#  if defined(_WIN32)
#    define	usleep(a)	Sleep((a)/1000)
#    define   strcasecmp(a,b) stricmp((a),(b))
#    define	B9600		9600
#  else
#    define	usleep(a)	delay(((a)/1000))
#    define	strcasecmp(a,b)	stricmp((a),(b))
#    define	B9600		9600
#  endif
# endif /* OS2 */
char *serial_port;
char serialbuff[64];
#endif


int item_count(unsigned char *from, int from_len);


#ifdef __linux__
char *default_serial_speed	= "B115200";
char *default_serial_port	= "/dev/ttyS0";
#else
# ifdef __sun__
char *default_serial_speed	= "B38400";
char *default_serial_port	= "/dev/ttya";
# else
#  if defined(OS2) || defined(_WIN32)
char *default_serial_port	= "COM1";
char *default_serial_speed	= "9600";
#  else
char *default_serial_port	= "/dev/tty??";
char *default_serial_speed	= "B9600";
#  endif
# endif
#endif


#if defined(OS2) || defined(_WIN32)
	static int PORT_SPEED		= 9600;
#else
	static int PORT_SPEED		= B9600;
#endif /* OS2 || _WIN32 */


char *serial_speed;


static int MSAC_SR1 = 0;
static int verbose = 1;


int TransferRateID(int baud);
int ReadCommByte(unsigned char *byte);
int ReadFileByte(unsigned char *byte);
Packet *ReadPacket(int(*readfunc)(unsigned char *));


static const char version_string[] = "0.4.4";


static unsigned char START_PACKET	= 192;
static unsigned char END_PACKET		= 193;

char ESC_START_STRING[]	= { 0x7d, 0xe0 };
char ESC_END_STRING[]	= { 0x7d, 0xe1 };
char ESC_ESC_STRING[]	= { 0x7d, 0x5d };


static FILE	*dsc_global_file_fd;

/*------------------------------------------------------------------------
*
* This array contains the expected packet sequence code to to be applied/
* checked for.
*
*
*/
static const unsigned char dsc_sequence[] = {14,0,32,34,66,68,100,102,134,136,168,170,202,204,236,238,255};


/*------------------------------------------------------------------------
*
* This array is used by ReadPacket
*
*
*/
unsigned char PacketCodes[2] = {192,193};


/**************************************************************
*
*/
int decode_image(char *filename, int image_style );
int Converse(Packet *out, unsigned char *str, int len);



#define DSC_INVALID_CHECKSUM	0x40
#define DSC_INVALID_SEQUENCE	0x41
#define DSC_RESET_SEQUENCE	0x42
#define DSC_RESEND_PACKET	0x43

#define DSC_ESCAPE_CHAR		0x7d
#define DSC_START_CHAR		0xc0
#define DSC_END_CHAR		0xc1


/* static int	NO_CTRL		= 0; */
/* static int	CHG_CTRL	= 1; */
/* static int	IN_PACKET	= 0; */

/* static unsigned char		dscf55_controlchar; */
static unsigned short int	dscf55_image_count;
static unsigned short int	dscf55_sequence_id;




Packet	CameraInvalid =	{0, 2, {131,125}, 93};
Packet	ResendPacket =	{0, 4, {129,2,'1',0}, 'L'};
 

#if defined(USE_ALL_TYPES)
static unsigned char	EmailImage[] = {0,2,2,0,16,'/','M','S','S','O','N','Y','/','I','M','C','I','F','1','0','0'};
#endif


static unsigned char	IdentString[] = {0,1,1,'S','O','N','Y',' ',' ',' ',' ',' '};
static unsigned char	EmptyPacket[]		= {0}; /* null packet */
static unsigned char	SetTransferRate[]	= {0,1,3,0};
static unsigned char	SendImage[]		= {0,2,'1',0,1,0,0};
static unsigned char	SendNextImagePacket[]	= {0,2,'1',0};
static unsigned char	SendThumbnail[]		= {0,2,'0',0};
static unsigned char	SelectImage[]		= {0,2,48,0,0,0,0};

//static unsigned char	SelectCamera[]		= {0,1,2};
//static unsigned char	DownloadComplete[]	= {0,2,'0',255};
//static unsigned char	X10Camera[]		= {0,1,5};

static unsigned char	StillImage[] = {0,2,2,0,14,'/','D','C','I','M','/','1','0','0','M','S','D','C','F'};
static unsigned char	MpegImage[]  = {0,2,2,0,16,'/','M','S','S','O','N','Y','/','M','O','M','L','0','0','0','1'};


/*static unsigned char	X5Camera[]		= {0,2,1};*/
static unsigned char	SendImageCount[]		= {0,2,1};
/* static unsigned char	X13Camera[]		= {0,2,18}; */



/*******************************************************************
*
*
*
*/
int SetSpeed(int baud)
{
	Packet dp;

	SetTransferRate[3] = TransferRateID(baud);

/*   printf("SetSpeed(%d), id=%d\n", baud, SetTransferRate[3]); */

	Converse(&dp, SetTransferRate, 4);
	dscSetSpeed(baud);
	Converse(&dp, EmptyPacket, 1);
	usleep(50000);	/* 50000 was good too, jw */

	return 0;
}



/*******************************************************************
*
*
*
*/
int sony_dscf55_initialize()
{
	verbose = 5;

#if !defined(STAND_ALONE)
	verbose = 5;
	serial_speed = default_serial_speed;
#endif

	if (verbose > 1) printf("Init\n");

	PORT_SPEED = ConfigDSCF55Speed(serial_speed, verbose);

	if(InitSonyDSCF55(serial_port))
	{
		int count = 0;
		Packet dp;

		for(count=0; count<3; count++)
		{
			dscf55_sequence_id = 0;

			if(verbose)
				printf("Init - Start\n");

			if(Converse(&dp, IdentString,12))
				break;
	
//			usleep(2000);
			printf("Init - Fail %u\n", count+1);
		}

		if(count<3)
			return TRUE;
	}

	printf("Init - leaving\n");

	return FALSE;
}


/*******************************************************************
*
*
*
*/
int sony_msac_sr1_initialize()
{
	MSAC_SR1 = 1;
	sony_dscf55_initialize();

	return 0;
}


/*******************************************************************
*
*
*
*/
struct Image *sony_dscf55_get_preview()
{
	return FALSE;
}


/*******************************************************************
*
* This may be possible but I have no information on how to do this.
*
*/
int sony_dscf55_delete_picture(int number)
{
	return FALSE;
}


/*******************************************************************
*
* This may be possible but I have no information on how to do this.
*
*/
int sony_dscf55_take_picture()
{
	return FALSE;
}


int sony_dscf55_item_count()
{
      return item_count(StillImage, sizeof(StillImage));
}

/*******************************************************************
*
* Return count of images taken.
*
*/
int item_count(unsigned char *from, int from_len)
{
	Packet dp;

	Converse(&dp, SetTransferRate, 4);

	if(Converse(&dp, from, from_len))
	{

		if(Converse(&dp, SendImageCount, 3))
		{
			dscf55_image_count = (unsigned short int) dp.buffer[5];
			return (int) dp.buffer[5];
		}
		else
			fprintf(stderr, "SendImageCount Failed\n");

	}
	else
		fprintf(stderr, "Init Image Failed\n");

	return 0;

}



/*******************************************************************
*
*
*
*/
int sony_msac_sr1_item_count()
{
 	MSAC_SR1 = 1;
 	return sony_dscf55_item_count();
}


/*******************************************************************
*
*
*
*/
int sony_dscf55_configure()
{
	return 0;
}


#if !defined(STAND_ALONE)
/*******************************************************************
*
*
*
*/
char *sony_dscf55_summary()
{
	return "Not Supported";
}


/*******************************************************************
*
*
*
*/
char *sony_dscf55_description()
{
	return "Sony DSC F55/505 gPhoto Library 
Mark Davies <mdavies@dial.pipex.com> 
Sony DSC F55 and DSC F505 cameras. 
Still image support. 
* Mpegs are downloadable in standalone 
  version only. 
";
}


char *sony_msac_sr1_description()
{
        return "Sony DSC F55/505 gPhoto Library
Mark Davies <mdavies@dial.pipex.com>
Sony MSAC-SR1 and Memory Stick used
by DCR-PC100.
Still image support. Patches by
Bernd Seemann <bernd@seebaer.ruhr.de>
* Mpegs are downloadable in
standalone version only.
";
}
#endif  // STAND_ALONE


/*******************************************************************
*
*
*
*/
int ReadFileByte(unsigned char *byte)
{
	int status = fread(byte, 1, 1, dsc_global_file_fd);
	return status;		/* XXX: what should we return here? jw */
}


/*******************************************************************
*
*
*
*/
Packet *ReadPacket(int(*readfunc)(unsigned char *))
{
	unsigned int n;
	unsigned char byte=0;
	static Packet p;

	p.length = 0;

	for(n=0; n<2; n++)
	{
		for(byte=0;byte!=(unsigned char)PacketCodes[n];)
		{
			if(readfunc(&byte) <=0)
				return (Packet *)0;

			if(n>0)
				if(DSC_ESCAPE_CHAR == byte)
				{
					unsigned char extra;

					readfunc(&extra);

					switch(extra)
					{
						case 1:
						case 7:
						case 0xe1:
						case 0xe0:
							extra &= 0xcf;
							p.buffer[p.length++]=extra;
							continue;
						case 0x5d:
							p.buffer[p.length++]=byte;
							continue;
						default:
							p.buffer[p.length++]=byte;
							p.buffer[p.length++]=extra;
							continue;
					}
				}
				else
					p.buffer[p.length++]=byte;
		}
	}

	p.length-=2;
	p.checksum = p.buffer[p.length];

	return &p;
}


/*******************************************************************
*
*
*
*/
int ReadCommsPacket(Packet *dst)
{
	Packet *p;

	p = ReadPacket(ReadCommByte);

	if(p)
	{
		memcpy(dst, p, sizeof(Packet));
		return 1;
	}

	return 0;
}


/*******************************************************************
*
*
*
*/
unsigned char CalcCheckSum(Packet *p)
{
	unsigned short int o = 0;
	unsigned long int sum = 0;

	sum = 0;

	while(o < p->length)
		sum += p->buffer[o++];

	return 256-(sum&255);
}


/*******************************************************************
*
*
*
*/
int MakePacket(Packet *p, unsigned char *buffer, unsigned short int length)
{
	p->length = 0;

	while(length--)	
		p->buffer[p->length++] = *(buffer++);

	if(255==dsc_sequence[++dscf55_sequence_id])
		dscf55_sequence_id = 0;

	p->buffer[0] = dsc_sequence[dscf55_sequence_id++];

	if(255==dsc_sequence[dscf55_sequence_id])
		dscf55_sequence_id = 0;

	p->checksum = CalcCheckSum(p);

	return TRUE;
}


/*******************************************************************
*
*
*
*/
int CheckPacket(Packet *p)
{
	unsigned char c = CalcCheckSum(p);

	if(c!=p->checksum)
		return DSC_INVALID_CHECKSUM;

	if(129==p->buffer[0])
		return DSC_RESEND_PACKET;

	if(dsc_sequence[dscf55_sequence_id] != p->buffer[0])
		return DSC_INVALID_SEQUENCE;

	return TRUE;
}


/*******************************************************************
*
*
*
*/
int ComparePacket(Packet *s, Packet *d)
{
	if(s->length == d->length)
		if(s->checksum == d->checksum)
		{
			if(!memcmp(s->buffer, d->buffer, d->length))
				return TRUE;
			else
				printf("memcmp failed\n");
		}
		else
			printf("checksum failed\n");
	else
		printf("length failed\n");

	return FALSE;
}


/*******************************************************************
*
*
*
*/
void SendPacket(Packet *p)
{
	unsigned short int count;

	extWrite((char *)&START_PACKET, 1);

	p->buffer[p->length] = p->checksum;

	for(count=0; count<p->length+1; count++)
	{
		switch((unsigned char)p->buffer[count])
		{
			case DSC_ESCAPE_CHAR:
				extWrite(ESC_ESC_STRING, 2);
				break;

			case DSC_START_CHAR:
				extWrite(ESC_START_STRING, 2);
				break;

			case DSC_END_CHAR:
				extWrite(ESC_END_STRING, 2);
				break;

			default:
				extWrite((char *)&p->buffer[count], 1);
				break;
		}
	}

	extWrite((char *)&END_PACKET, 1);
}


/*******************************************************************
*
*
*
*/
int Converse(Packet *out, unsigned char *str, int len)
{
	Packet ps;
	char old_sequence=33;
	int sequence_count=0;
 	int invalid_sequence=0;
	int count;

	MakePacket(&ps, str, len);

	for(count=0; count<10; count++)
	{
		SendPacket(&ps);
		if(ReadCommsPacket(out))
			switch(CheckPacket(out))
			{
				case DSC_INVALID_CHECKSUM:
					if (invalid_sequence)
					{
						MakePacket(&ps, str, len);
						break;
					}

					printf("Checksum invalid\n");
					ps.buffer[0] = 129;
					ps.checksum = CalcCheckSum(&ps);
					break;

				case DSC_INVALID_SEQUENCE:
					if (MSAC_SR1)
					{
						invalid_sequence = 1;
						MakePacket(&ps, str, len);
						break;
					}

					if(old_sequence == out->buffer[0])
						sequence_count++;
					else
						if(0==sequence_count)
							old_sequence = out->buffer[0];

					if(sequence_count==4)
					{
						printf("Attempting to reset sequence id - image may be corrupt.\n");
						dscf55_sequence_id=0;

						while(dsc_sequence[dscf55_sequence_id] != old_sequence)
							dscf55_sequence_id++;

						return TRUE;
					}

					printf("Invalid Sequence\n");
					ps.buffer[0] = 129;
					ps.checksum = CalcCheckSum(&ps);
					break;

				case DSC_RESET_SEQUENCE:
					dscf55_sequence_id=0;
					return TRUE;
		
				case DSC_RESEND_PACKET:
					printf("Resending Packet\n");
					break;

				case TRUE:
					return TRUE;

				default:
					printf("Unknown Error\n");
					break;
			}
			else
			{
/*				printf("Incomplete packet\n"); */
				ps.buffer[0] = 129;
				ps.checksum = CalcCheckSum(&ps);
			}
	}

	printf("Converse: Failed to read packet.\n");
#ifndef STAND_ALONE
	exit(0);
#endif

	return FALSE;
}


#if !defined(STAND_ALONE)


/*******************************************************************
*
*
*
*/
struct Image *sony_dscf55_get_picture(int imageid, int thumbnail)
{
	static FILE *fp;
	char filename[64];
	long size;
	int sc=11; /* count of bytes to skip at start of packet */
	struct Image *image;
	FILE *temp;
	Packet dp;

	if (PORT_SPEED > B9600) SetSpeed(PORT_SPEED);

	if(thumbnail)
	{
		sc = 0x247;

		if(!Converse(&dp, StillImage, 19))
			printf("StillImage Failed\n");

		SelectImage[4] = imageid;
		Converse(&dp, SelectImage, 7);

		sprintf(filename, "/tmp/gphoto_image_%u.jpg", imageid-1);
		temp = fopen(filename, "wb");

		if (MSAC_SR1)
			fwrite("\xff\xd8\xff", 3, 1, temp);
		for(;;)
		{
			Converse(&dp, SendThumbnail, 4);

			fwrite((char *)dp.buffer+sc, sizeof(char), dp.length-sc, temp);
			sc=7;

			if(3==dp.buffer[4])
				break;
		}

		fclose(temp);
	}
	else
	{
		int last_packet = 0;

		sprintf(filename, "/tmp/gphoto_image_%u.jpg", imageid-1);

		if(!Converse(&dp, StillImage, 19))
			printf("StillImage Failed\n");

		temp = fopen(filename, "wb");

		SendImage[4] = imageid;
		Converse(&dp, SendImage, 7);


		for(;;)
		{
			fwrite((char *)dp.buffer+sc, sizeof(char), dp.length-sc, temp);
			sc=7;

			if(3==dp.buffer[4])
				break;

			Converse(&dp, SendNextImagePacket, 4);
		}

		if(temp)
		    fclose(temp);
	}

	fp = fopen(filename, "r");

	image = 0;

	if(fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		rewind(fp);
		image = (struct Image*)malloc(sizeof(struct Image));

		if(image)
		{
			image->image = (char*)malloc(sizeof(char)*size);
			image->image_size = size;
			image->image_info_size = 0;
			strcpy(image->image_type, "jpg");
			fread(image->image, sizeof(char), size, fp);
		}
		else
			printf("Failed to allocate Image struct\n");

		fclose(fp);
		unlink(filename);
	}
	else
		printf("Failed to open file\n");

        SetSpeed(B9600);

	return(image);
}


/*******************************************************************
*
*
*
*/
struct Image *sony_msac_sr1_get_picture(int imageid, int thumbnail)
{
	MSAC_SR1 = 1;
	return sony_dscf55_get_picture(imageid, thumbnail);
}


struct _Camera sony_dscf55 = {  sony_dscf55_initialize,
			       sony_dscf55_get_picture,
			       sony_dscf55_get_preview,
			       sony_dscf55_delete_picture,
			       sony_dscf55_take_picture,
			       sony_dscf55_item_count,
			       sony_dscf55_configure,
			       sony_dscf55_summary,
			       sony_dscf55_description,
			       NULL};


struct _Camera sony_msac_sr1 = {  sony_msac_sr1_initialize,
                               sony_msac_sr1_get_picture,
                               sony_dscf55_get_preview,
                               sony_dscf55_delete_picture,
                               sony_dscf55_take_picture,
                               sony_msac_sr1_item_count,
                               sony_dscf55_configure,
                               sony_dscf55_summary,
                               sony_msac_sr1_description,
			       NULL};


#else /* STAND_ALONE */

/*******************************************************************
*
*
*/

int force_write = 0;
int do_exit = 0;

void sig_cleanup(int a)
{
  do_exit = 1;
}

/*
 * reset the camera sequence count and baud rate.
 */
int clean_exit(int e)
{
	Packet dp; 

	if(verbose)
		printf("exiting...\n");

	SetSpeed(B9600);

	while(dscf55_sequence_id)
		Converse(&dp, EmptyPacket, 1);

	CloseSonyDSCF55();
	exit(e);

	return 0; /* Stop compiler moaning */
}


int sony_dscf55_store(int imageid, char *path, unsigned char *from, int from_len, char *name_match)
{
	FILE *temp=0;
	int f;
	int sc=11; /* count of bytes to skip at start of packet */
	char filename[256];
	Packet dp;
	char *fname = filename + 1;


	SelectImage[4] = imageid;
	Converse(&dp, SelectImage, 7);

	sprintf(filename, "%s/%11.11s", path ? path : "", dp.buffer+5);
	if (path) fname += strlen(path);

/*	printf("%s\n", filename); */

	f = strlen(filename);
	filename[f] = filename[f-1];
	filename[f-1] = filename[f-2];
	filename[f-2] = filename[f-3];
	filename[f-3] = '.';
	filename[f+1] = 0;

	f = 0;

	while(filename[f])
	{
		filename[f] = tolower(filename[f]);
		f++;
	}

	if (name_match)
	  {
	    if (strcasecmp(fname, name_match) && 
	        (atoi(fname+3) != atoi(name_match)))
	    return -1;
	  }

	if (!path)	/* = ls_flag */
	  {
	    unsigned int l = 0;
	    l = (l << 8) | dp.buffer[16];
	    l = (l << 8) | dp.buffer[17];
	    l = (l << 8) | dp.buffer[18];
	    l = (l << 8) | dp.buffer[19];
	    printf(" %s %8d\n", fname, l);
	    return -1;
	  }

	if (!force_write && (temp = fopen(filename, "r")))
	  {
	    fclose(temp);
	    if (verbose) printf("%s exists, skipping...\n", filename);

	    return -1;
	  }

	if(!Converse(&dp, from, from_len))
		printf("store init failed\n");

	temp = fopen(filename, "wb");
	printf("%s\n", filename);

	usleep(50000);

	SendImage[4] = imageid;
	Converse(&dp, SendImage, 7);

	for(;;)
	{
		if (verbose > 1) { putchar('.'); fflush(stdout); }

		if(dp.length>6)
			fwrite((char *)dp.buffer+sc, sizeof(char), dp.length-sc, temp);
		sc=7;

		if (3 == dp.buffer[3])
			break;

		Converse(&dp, SendNextImagePacket, 4);

		if (do_exit)
			break;
	}

	if (verbose > 1) putchar('\n');

	if(temp)
	    fclose(temp);

	return 1;
}

char *output_dir = ".";

int usage(char *av0, char *fmt, char *arg)
{
	fprintf(stderr, "rsony %s\n", version_string);

	if(fmt)
	{
		fprintf(stderr, "ERROR: "); 
		fprintf(stderr, fmt, arg);
		fprintf(stderr, "\n\n");
	}

	fprintf(stderr, "Usage:\n%s [options] <name_or_number>\n", av0);
	fprintf(stderr, "\nvalid options are:\n");
	fprintf(stderr, "\t-device port         Serial device (default: %s)\n", serial_port);
	fprintf(stderr, "\t-speed baud          Baud rate (default: %s)\n", serial_speed);
	fprintf(stderr, "\t-output directory    Path where to store Images & Movies. (default: %s)\n", output_dir);
	fprintf(stderr, "\t-list                List Image&Movie directories. (default: download)\n");
	fprintf(stderr, "\t-verbose             Print dots while downloading.\n");
	fprintf(stderr, "\t-quiet               Print nothing but names.\n");
	fprintf(stderr, "\t-force               Overwrite existing files. (default: skip)\n");
	fprintf(stderr, "\n<name_or_number> selects what to retrieve from the Camera:\n");
	fprintf(stderr, "\t-all                 Download all Images and Movies\n");
	fprintf(stderr, "\t-jpeg                Download all Images\n");
	fprintf(stderr, "\t-mpeg                Download all Movies\n");
	fprintf(stderr, "\t<number>             Download DSC<number>.JPG or MOV<number>.MPG\n");
	fprintf(stderr, "\t<name>               Download Image or Movie by name\n");
	return 1;
}


/*******************************************************************
*
*
*
*/
int main(int argc, char *argv[])
{
	int totalpics;
	int count;
	int ls_flag = 0;
	int jpeg_flag = 0;
	int mpeg_flag = 0;
	char *name_match = NULL;
	int all_flag = 0;
	char *av0 = argv[0];

	argc--; argv++;

	serial_port = default_serial_port;
	serial_speed = default_serial_speed;

	if (getenv("DSCF55E_PORT"))  serial_port  = getenv("DSCF55E_PORT");
	if (getenv("DSCF55E_SPEED")) serial_speed = getenv("DSCF55E_SPEED");
	if (getenv("DSCF55E_MODEL") &&
		!strcmp(getenv("DSCF55E_MODEL"), "MSAC-SR1")) MSAC_SR1 = 1;

	while (argc && argv[0][0] == '-')
	{
		if(!strncmp(argv[0], "-list", 2))
			ls_flag = 1;
		else if(!strncmp(argv[0], "-verbose", 2))
			verbose++;
		else if(!strncmp(argv[0], "-quiet", 2))
			verbose = 0;
		else if(!strncmp(argv[0], "-force", 2))
			force_write = 1;
		else if(!strncmp(argv[0], "-device", 2) && argc > 1)
		{
			serial_port = argv[1];
			argv++; argc--;
		}
		else if(!strncmp(argv[0], "-speed", 2) && argc > 1)
		{
			serial_speed = argv[1];
			argv++; argc--;
		}
		else if(!strncmp(argv[0], "-output", 2) && argc > 1)
		{
			output_dir = argv[1];
			argv++; argc--;
		}
		else if(!strncmp(argv[0], "-mpeg", 2))
			all_flag = mpeg_flag = 1;
		else if(!strncmp(argv[0], "-jpeg", 2))
			all_flag = jpeg_flag = 1;
		else if(!strncmp(argv[0], "-all", 2))
			all_flag = mpeg_flag = jpeg_flag = 1;
		else
			return usage(av0, "unknown option: %s", argv[0]);
		argv++; argc--;
	}

	printf("Parsed args\n");
  
	if(!all_flag && argc > 0)
	{
		name_match = argv[0];
		if(!mpeg_flag && !jpeg_flag)
			mpeg_flag = jpeg_flag = 1;
		argv++; argc--;
	}
  
	if(ls_flag && !mpeg_flag && !jpeg_flag && !name_match)
		all_flag = mpeg_flag = jpeg_flag = 1;
  
	if(argc > 0)
		return usage(av0, "bogus parameter: %s ...", argv[0]);

	if(!name_match && !all_flag)
		return usage(av0, NULL, NULL);

	signal(SIGINT, sig_cleanup);

	if(!sony_dscf55_initialize())
	{
		fprintf(stderr, "Unable to initialize camera\n");
		clean_exit(1);
	}

	usleep(30000);

	if(PORT_SPEED > B9600)
		SetSpeed(PORT_SPEED);

	if(jpeg_flag)
	{
		if(!(totalpics = item_count(StillImage, sizeof(StillImage))))
		{
		  fprintf(stderr, "Camera error or no pictures to download\n");
		  if(!mpeg_flag)
			clean_exit(2);
		}

		if (verbose) 
			printf("%s %d pics\n", ls_flag ? "total:" : "downloading", totalpics);

		for(count=1; count<=totalpics; count++)
		{
			long t = time(0);
			int s = sony_dscf55_store(count, ls_flag ? NULL : output_dir, 
			StillImage, sizeof(StillImage), name_match);

			if(s > 0 && verbose)
				printf(" - %ld seconds.\n", time(0) - t);

			if(!s)
			{
				fprintf(stderr, "Error downloading image\n");
				clean_exit(4);
			}

			  if(do_exit)
				clean_exit(3);

		}
	}

	if(mpeg_flag)
	{
		if(!(totalpics = item_count(MpegImage, sizeof(MpegImage))))
		{
			fprintf(stderr, "no movies to download\n");
			clean_exit(4);
		}

		if(verbose)
			printf("%s %d movies\n", ls_flag ? "total:" : "downloading", totalpics);

		for (count=1; count<=totalpics; count++)
		{
			long t = time(0);
			int s = sony_dscf55_store(count, ls_flag ? NULL : output_dir, MpegImage, sizeof(MpegImage), name_match);

			if(s > 0 && verbose)
				printf(" - %ld seconds.\n", time(0) - t);

			if(!s)
			{
				fprintf(stderr, "Error downloading movie\n");
				clean_exit(4);
			}

			if(do_exit)
				clean_exit(3);

		}
	}

	return clean_exit(0);


}
#endif

