/*
** Copyright (C) 1999 Michael McCormack
**
** Filename:    coolpix600.c
** Author:      Michael McCormack
** Date:        20 March 1999
** Version:     0.2
** Platform:    Linux
** WWW ref:     http://www.ozemail.com.au/~mccormac/dsc.c
** Licence:     GNU Public Licence
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; version 2 dated June, 1991.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program;  if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
**
** For more infomation see http://www.gnu.org/copyleft/gpl.html
**
** Description:
**  This program downloads images from a Nikon CoolPix 9600.
**  It compiles and runs under Linux and is intended as a replacement
**  for the Nview600.exe program packaged with the CoolPix.
**  DSC is is Nikon's acronym for Digital Still Camera.
**
** Installation:
**  Connect the camera to COM1 (cua0 is the first com port). To
**  configure the program to use COM1, make a symlink as follows:
**     ln -s /dev/cua0 /dev/dsc
**
**  To compile the program:
**     gcc -o dsc dsc.c
**
**  To download all the thumbnails:
**     dsc -t all
**
**  To view the thumbnails, use a JPEG viewer of your choice. eg:
**     xv thumb_*
**
** Todo list:
**   - add date and time setting
**   - support other baud rates. Currently 38400 is hardcoded.
**   - put pictures back into the camera
*    - port to other *nixs. That shouldn't be too hard.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include "../src/gphoto.h"
#include "../src/util.h"

/* print all data transmitted and received via the serial port */
/* #define DSC_TRACE */

const unsigned char DSC_THUMB_READ =0x16;
const unsigned char DSC_IMAGE_READ =0x1a;
const unsigned char DSC_READ_BLOCK =0x1e;
const unsigned char DSC_INITIALISE =0x10;
const unsigned char DSC_QUERY_PICS =0x07;
const unsigned char DSC_DISCONNECT =0x1f;
const unsigned char DSC_DELETE_PIC =0x11;

/* number of image data bytes transfered in each packet */
const int DSC_BLOCKSIZE = 0x400;
const char* DSC_DEFAULT_DEVICE = "/dev/dsc";
const int DSC_MAX_PICS = 0xFF;
const int DSC_TIMEOUT = 5000000 /* usec */;

/* const int TRUE  = 1;
 * const int FALSE = 0;
 */

/* typedef struct TAG_Image {
 *   int number;
 *   int length;
 *   ImageType image_type;
 *   unsigned char *data;
 * } Image;
 */

/* these messages are used for negotiation of communications speed */
static unsigned char tx_msg1[]=
{
    0x4d, 0x4b, 0x45, 0x20, 0x44, 0x53, 0x43, 0x20, 
    0x50, 0x43, 0x20, 0x20, 0x00, 0x00, 0x00, 0x01, 
    0x04, 0x01,
};

static unsigned char rx_msg1[]=
{
    0x4d, 0x4b, 0x45, 0x20, 0x50, 0x43, 0x20, 0x20, 
    0x44, 0x53, 0x43, 0x20, 0x00, 0x00, 0x00, 0x01, 
    0x01, 0x00,
};

static unsigned char tx_msg2[]=
{
    0x4d, 0x4b, 0x45, 0x20, 0x44, 0x53, 0x43, 0x20, 
    0x50, 0x43, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,  
    0x02,
};

static unsigned char rx_msg2[]=
{
    0x4d, 0x4b, 0x45, 0x20, 0x50, 0x43, 0x20, 0x20, 
    0x44, 0x53, 0x43, 0x20, 0x00, 0x00, 0x00, 0x04, 
    0x03, 0x4e, 0x49, 0x4b, 0x31,
};

/* globals suck, but there's no choice */
static int fd;

static int transmit(int fd, unsigned char *out, int length) 
{
    int i;

#ifdef DSC_TRACE
    printf("Transmitting: ");
    for(i=0; i<length ; i++)
        printf("%02X ",out[i]);
    printf("\n");
#endif

    return write(fd,out,length);
}

jmp_buf timeout_jmp;

static void timeout(int x) 
{
    longjmp(timeout_jmp,1);
}

static int receive(int fd, unsigned char *buf, int length) 
{
    int n,total=0,i;
    struct itimerval timer;
     
    if(0==setjmp(timeout_jmp)) {
        /* set up a timer to stop ourselves from blocking forever */
        signal(SIGALRM, timeout);
        getitimer(ITIMER_REAL,&timer);
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = DSC_TIMEOUT;
        setitimer(ITIMER_REAL, &timer, NULL);

        /* read from the serial port until we have all the data we need */
        total=0;
        while(total<length) {
            n = read(fd, &buf[total], length);
            if(n<0)
                break;
            total+=n;
        }
    }

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL,&timer, NULL);

#ifdef DSC_TRACE
    /* dump what we received */
    printf("Received: ");
    for(i=0; i<((total>16)?16:total); i++)
        printf("%02X ",buf[i]);
    printf("\n");
#endif

    return total;
}

/*
** Receives characters from the serial port and checks
** that the checksum is correct.
*/
static int ReceiveDSCMessage(int fd, unsigned char *buf, int length) 
{
    int len,i,sum;

    len = receive(fd,buf,length);

    if(length<4)
        return length;

    /* check the checksum */
    sum=0;
    for(i=1; i<(len-2); i++)
    {
        sum += buf[i];
        sum %= 0x100;
    }
    sum -= buf[i];
    if(sum)
    {
        dialog_error("Checksum was bad.");
    }
    return len;
}

/* TODO: generalise this so that it supports other baud rates */
static int init_comms(int fd)
{
    struct termios tio;
    unsigned char buf[0x100];
    int length;

    cfmakeraw(&tio);
    cfsetospeed(&tio,B9600);
    if(0>tcsetattr(fd, TCSADRAIN, &tio))
        dialog_error("Error setting communication parameters");

    transmit(fd,tx_msg1,sizeof tx_msg1);

    length = receive(fd,buf,sizeof rx_msg1);

    if((length != sizeof rx_msg1) || memcmp(buf,rx_msg1,sizeof rx_msg1))
        return -1;

    /* change the baud rate to 38400 */
    cfmakeraw(&tio);
    cfsetospeed(&tio,B38400);
    if(0>tcsetattr(fd, TCSADRAIN, &tio))
        return -1;
    else
        update_status("DSC: Set baud rate to 38400\n");

    transmit(fd,tx_msg2,sizeof tx_msg2);

    length = receive(fd,buf,sizeof rx_msg2);

    if((length == sizeof rx_msg2) && !memcmp(buf,rx_msg2,sizeof rx_msg2)) {
        return 0;
    }

    return -1;
}

static int SendDSCMessage(int fd, unsigned int seqno, 
                   unsigned char command, unsigned char number) 
{
    unsigned char send[0x10];
    unsigned char sum;
    int i;

    memset(send,0,sizeof send);

    send[0]=0x08;
    send[1]=seqno;
    send[2]=~seqno;
    send[3]=command;
    send[4]=number;

    sum = 0;
    for(i=1; i<0x0e; i++)
        sum+=send[i];

    send[0x0e] = sum;

    return transmit(fd,send,sizeof send);
}

/*
** Place the camera in remote mode. In this mode, "PC CONNECTED"
** will be displayed on the camera's screen.
*/
static int DSC_Initialise(void) 
{
    unsigned char buffer[0x10];
    int len;
    
    fd = open(serial_port,O_RDWR);

    if(fd<0)
    {
        dialog_error("Couldn't open camera device");
        return 0;
    }

    if(init_comms(fd)) 
    {
        dialog_error("Couldn't init comm port");
        return 0;
    }

    SendDSCMessage(fd,0,DSC_INITIALISE,0);
    len = ReceiveDSCMessage(fd,buffer,0x10);

    if((0x10 == len) && (buffer[4]==0))
        return 0;

    dialog_error("Camera initialisation failed.");

    return 1;
}

/*
** returns 0 for error or the number of pictures in the camera
*/
static int DSC_QueryPics() 
{
    unsigned char buffer[0x10];
    int len;
    
    SendDSCMessage(fd,0,DSC_QUERY_PICS,0);
    len = ReceiveDSCMessage(fd,buffer,0x10);

    if(0x10 == len) 
        return buffer[4];

    return 0;
}

/*
** This enables the camera's local interface again. The
** "PC CONNECTED" message should disappear from the camera's screen.
*/
static int DSC_Disconnect(int fd) 
{
    unsigned char buffer[0x10];
    
    SendDSCMessage(fd,0,DSC_DISCONNECT,0);
    ReceiveDSCMessage(fd,buffer,0x10);

    return 0;
}

/*
** the name of this function is deceptive, as it
** selects a thumbnail for reading as well as finding its length
*/
static int DSC_QueryThumbnailLength(int fd, int no) 
{
    unsigned char buffer[0x10];
    int len;
    
    SendDSCMessage(fd,0,DSC_THUMB_READ,no);
    len = ReceiveDSCMessage(fd,buffer,0x10);

    if(0x10 == len) 
        return buffer[4] + buffer[5]*0x100 + buffer[6]*0x10000;

    return -1;
}

/*
** the name of this function is deceptive, as it
** selects a thumbnail for reading as well as finding its length
*/
static int DSC_QueryImageLength(int fd, int no) 
{
    unsigned char buffer[0x10];
    int len;
    int i;

    SendDSCMessage(fd,0,DSC_IMAGE_READ,no);
    len = ReceiveDSCMessage(fd,buffer,0x10);

    if(0x10 != len) 
        return -1;

    /* return the length of the image */
    return buffer[4] + buffer[5]*0x100 + buffer[6]*0x10000;
}

static int DSC_ReadBlock(int fd, int blockno, unsigned char *block, int no) {
    static unsigned char buf[0x500];
    int len;

    SendDSCMessage(fd,blockno,DSC_READ_BLOCK,blockno);
    len = ReceiveDSCMessage(fd,buf,DSC_BLOCKSIZE+6);

    if(len != DSC_BLOCKSIZE+6)
        return -1;

    memcpy(block,&buf[4],DSC_BLOCKSIZE);

    return 0;
}

/*
** fill out the rest of the image structure, given "number" already
*/
static struct Image *DSC_ReadImage(int picnum, int thumbnail)
{
    int blocks,i;
    char str[80];
    struct Image *image = (struct Image *) malloc (sizeof(struct Image));

    if(thumbnail)
        image->image_size = DSC_QueryThumbnailLength(fd,picnum);
    else 
        image->image_size = DSC_QueryImageLength(fd,picnum);

    if(image->image_size<0)
    {
        dialog_error("Query length failed.");
        free (image);
        return 0;
    }

    sprintf(str, "Length of %s %d is %d bytes\n", 
            thumbnail?"thumbnail":"image",
            picnum, 
            image->image_size);
    update_status(str);

    blocks = (image->image_size + DSC_BLOCKSIZE - 1) / DSC_BLOCKSIZE;

    image->image = (unsigned char *) malloc(blocks * DSC_BLOCKSIZE);

    if(!image->image)
    {
        dialog_error("Failed to malloc image data.");
        free(image);
        return 0;
    }

    update_progress(0);
    for(i=0; i<blocks; i++) 
    {
        if(0>DSC_ReadBlock(fd,
                           i,
                           &image->image[i*DSC_BLOCKSIZE],
                           picnum))
        {
            dialog_error("ReadBlock failed.");
            free(image->image);
            image->image=NULL;
            free(image);
            return NULL;
        }
	if (blocks)
	        update_progress(100 * i/ blocks);
    }

    strcpy(image->image_type,"jpg");
    image->image_info_size = 0;

    return image;
}

/*
** delete an image from the camera
** The image number is one based (1 is the first image).
*/
static int DSC_DeleteImage(int image_no)
{
    static unsigned char buf[0x500];
    int len;

    SendDSCMessage(fd,0,DSC_DELETE_PIC,image_no);
    len = ReceiveDSCMessage(fd,buf,0x10);

    if(len != 0x10)
        return 0;

    /* TODO: add a check here to see if the return data indicates success */

    return 1;
}

static char *DSC_Summary(void)
{
    return "Not Supported";
}

static char *DSC_Description(void)
{
    return	"gPhoto Nikon Coolpix 600 driver\n"
		"(C) Michael McCormack 1999\n"
		"mailto:mccormack@ozemail.com.au\n";
}

static struct Image *DSC_Preview(void)
{
    return 0;
}

static int DSC_TakePicture()
{
    return 0;
}

static int DSC_Configure()
{
    return 0;
}

struct _Camera coolpix600 =
{
	DSC_Initialise,
	DSC_ReadImage,
	DSC_Preview,
	DSC_DeleteImage,
	DSC_TakePicture,
	DSC_QueryPics,
	DSC_Configure,
	DSC_Summary,
	DSC_Description,
};
