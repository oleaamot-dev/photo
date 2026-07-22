/*
   ** Copyright (C) 2000 James McKenzie
   **
   ** Filename:    samsung.c
   ** Author:      James McKenzie
   ** Date:        12 January 1999
   ** Version:     1.0
   ** Platform:    Linux
   ** WWW ref:     http://bullard.esc.cam.ac.uk/~james/digimax.html
   ** Licence:     GNU General Public Licence
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
   **  This program downloads images from a Samsung Digimax 800k
   **
   ** Todo list:
   **   - fix crc code
   **
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
/* #define SDSC_TRACE */

typedef unsigned char byte;

const unsigned char SDSC_START = 0x43;
const unsigned char SDSC_NEXT = 0x53;
const unsigned char SDSC_BINARY = 0x06;
const unsigned char SDSC_RETRANSMIT = 0x15;

/* number of image data bytes transfered in each packet */
const int SDSC_BLOCKSIZE = 0x405;
const int SDSC_INFOSIZE = 0x85;
const int SDSC_TIMEOUT = 500000 /* usec */ ;
const int SDSC_RETRIES = 3;


int fd;

static int
sendcommand (byte cmd)
{
  int i;

#ifdef DEBUG
  fprintf (stderr, "Sending byte 0x%02x\n", cmd);
#endif
  return write (fd, &cmd, 1);
}

static int
waitforinput ()
{
  fd_set rfds;
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = SDSC_TIMEOUT;

  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);

  if (!select (fd + 1, &rfds, NULL, NULL, &tv))
    return 0;

  if (!FD_ISSET (fd, &rfds))
    return 0;

  return 1;
}

static int
checkcrc (byte * buf, int len)
{
/* FIXME: */
  return 1;
}

static int
receive (byte * buf, int length, int expected)
{
  int tries = 0;
  int n, total, i;
  byte *ptr;


  while (1)
    {


      ptr = buf;
      i = length;
      total = 0;

      while (waitforinput (fd))
        {

          n = read (fd, ptr, i);

          if (n > 0)
            {
              ptr += n;
              i -= n;
              total += n;

              if (checkcrc (buf, total) && (total == expected))
                {
#ifdef DEBUG
                  fprintf (stderr, "Read %d bytes as expected\n", total);
#endif
                  return total;

                }

            }
        }

#ifdef DEBUG
      fprintf (stderr, "Read %d byes of %d bytes expected\n", total, expected);
#endif

      /*Ok didn't match expected - two posibities */
      /*1st we missed packet/crcerror */
      /*2nd this is the last packet */

      tries++;


      if ((total == 1) && (tries == 2) && (expected == SDSC_BLOCKSIZE))
        {
#ifdef DEBUG
          fprintf (stderr, "Detected EOF condition\n");
#endif
          return total;
        }

      if (tries == SDSC_RETRIES)
        {
#ifdef DEBUG
          fprintf (stderr, "Giving up\n");
#endif
          return 0;
        }

#ifdef DEBUG
      fprintf (stderr, "Retrying\n");
#endif
      sendcommand (SDSC_RETRANSMIT);


    }

}


static int 
init_comms ()
{
  struct termios tios;
  unsigned char buf[0x100];
  int length;

  tcgetattr (fd, &tios);
  tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CLOCAL | CRTSCTS);
  tios.c_cflag |= CLOCAL;
  tios.c_cflag |= CS8;

  tios.c_cflag |= CREAD | HUPCL;
  tios.c_iflag = IGNBRK | IGNPAR;

  tios.c_oflag = 0;
  tios.c_lflag = 0;
  tios.c_cc[VMIN] = 1;
  tios.c_cc[VTIME] = 0;


  cfsetospeed (&tios, B115200);
  cfsetispeed (&tios, B115200);

  if (tcsetattr (fd, TCSAFLUSH, &tios) < 0)
    dialog_error ("Error setting communication parameters");

  return 0;
}

/*
   ** Place the camera in remote mode. In this mode, "PC CONNECTED"
   ** will be displayed on the camera's screen.
 */
static int 
SDSC_Initialise (void)
{

  fd = open (serial_port, O_RDWR);

  if (fd < 0)
    {
      dialog_error ("Couldn't open camera device");
      return 1;
    }

  if (init_comms (fd))
    {
      dialog_error ("Couldn't init comm port");
      return 1;
    }
  return 0;

}

static int 
isnullinfo (byte * buf)
{
  int n = 128;
  buf += 3;

  while (n--)
    {
      if (*(buf++))
        return 0;
    }

  return 1;
}

/*
   **
 */
static int 
SDSC_disconnect ()
{
  unsigned char buffer[2048];
  int len;


#ifdef DEBUG
  fprintf(stderr,"Do disconnect\n");
#endif

  do
    {
      sendcommand (SDSC_NEXT);
      sendcommand (SDSC_START); /*Read out a header */
      len = receive (buffer, sizeof (buffer), SDSC_INFOSIZE);

      if (len != SDSC_INFOSIZE)
        return 0;

    }
  while (!isnullinfo (buffer));
  sendcommand (SDSC_NEXT);

#ifdef DEBUG
  fprintf(stderr,"Done disconnect\n");
#endif

  return 0;
}

/*
   ** returns 0 for error or the number of pictures in the camera
 */
static int 
SDSC_QueryPics ()
{
  unsigned char buffer[2048];
  int len;
  int n;

#ifdef DEBUG
  fprintf(stderr,"Count\n");
#endif

  SDSC_disconnect (fd);

  /*Now at start so count how many... */

  n = 0;

  do
    {
      sendcommand (SDSC_START); /*Read out a header */
      len = receive (buffer, sizeof (buffer), SDSC_INFOSIZE);

      if (len != SDSC_INFOSIZE)
        return 0;

      if (!isnullinfo (buffer))
        {
          n++;
        }

          sendcommand (SDSC_NEXT);

    }
  while (!isnullinfo (buffer));

#ifdef DEBUG
  fprintf(stderr,"Done Count %d\n",n);
#endif

  return n;
}


/*
   ** fill out the rest of the image structure, given "number" already
 */
static struct Image *
SDSC_ReadImage (int picnum, int thumbnail)
{
  unsigned char buffer[2048];
  char str[1024];
  int len;
  int i;
  int blocks;

  struct Image *image = (struct Image *) malloc (sizeof (struct Image));
  byte *ptr;
  int left, size;

#ifdef DEBUG
  fprintf(stderr,"ReadImage \n");
#endif
  if (thumbnail)
    {
      free (image);
      return 0;
    }

  SDSC_disconnect (fd);         /*Rewind */

  i = picnum;
#ifdef DEBUG
  fprintf(stderr,"Fetching %d (%d,%d)\n",picnum,thumbnail);
#endif

  while (--i) 			/*Seek*/
    {
      sendcommand (SDSC_START);    
      len = receive (buffer, sizeof (buffer), SDSC_INFOSIZE);
      sendcommand (SDSC_NEXT);
    }

  sendcommand (SDSC_START);     /*Read out a header */
  len = receive (buffer, sizeof (buffer), SDSC_INFOSIZE);

  if (len != SDSC_INFOSIZE)
    {
      free (image);
      return 0;
    }

  buffer[SDSC_INFOSIZE] = 0;

  image->image_size = atoi (&buffer[15]);


  sprintf (str, "Length of image %d is %d bytes\n",
           picnum,
           image->image_size);
  update_status (str);

  blocks = (image->image_size + 0x3ff) / 0x400;
  blocks += 2;

  left = size = 0x400 * blocks;
  i = 0;

  ptr = image->image = (unsigned char *) malloc (size);

  if (!image->image)
    {
      dialog_error ("Failed to malloc image data.");
      free (image);
      return 0;
    }

  update_progress (0);

  sendcommand (SDSC_BINARY);    /*Put into image mode */
  sendcommand (SDSC_START);     /*Start reading */

  while (left > 0x400)
    {
      len = receive (buffer, sizeof (buffer), SDSC_BLOCKSIZE);

      if (!len)
        {
          free (image->image);
          free (image);
#ifdef DEBUG
  fprintf(stderr,"ReadImage failed \n");
#endif
          return 0;
        }

      if ((len == 1) && (buffer[1] == 0x4))
        break;

      bcopy (buffer + 3, ptr, 0x400);

      ptr += 0x400;
      left -= 0x400;
      i += 0x400;

      if (size)
	      update_progress (100 * i / size);

      sendcommand (SDSC_BINARY);  /*Next block */
    }


  SDSC_disconnect (fd);         /*Rewind */

  strcpy (image->image_type, "jpg");
  image->image_info_size = 0;
#ifdef DEBUG
  fprintf(stderr,"Done ReadImage \n");
#endif

  return image;
}

/*
   ** delete an image from the camera
   ** The image number is one based (1 is the first image).
 */
static int 
SDSC_DeleteImage (int image_no)
{
  return 0;
}

static char *
SDSC_Summary (void)
{
  return "Not Supported";
}

static char *
SDSC_Description (void)
{
  return "gPhoto Samsung digimax 800k driver\n"
    "(C) James Mckenzie 2000\n"
    "mailto:james@fishsoup.dhs.org\n";
}

static struct Image *
SDSC_Preview (void)
{
  return 0;
}

static int 
SDSC_TakePicture ()
{
  return 0;
}

static int 
SDSC_Configure ()
{
  return 0;
}

struct _Camera samsung800k =
{
  SDSC_Initialise,
  SDSC_ReadImage,
  SDSC_Preview,
  SDSC_DeleteImage,
  SDSC_TakePicture,
  SDSC_QueryPics,
  SDSC_Configure,
  SDSC_Summary,
  SDSC_Description,
  NULL
};
