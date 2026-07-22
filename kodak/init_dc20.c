/*
 *	File: init_dc20.c
 *
 *	Copyright (C) 1998 Ugo Paternostro <paterno@dsi.unifi.it>
 *
 *	This file is part of the dc20ctrl package. The complete package can be
 *	downloaded from:
 *	    http://aguirre.dsi.unifi.it/~paterno/binaries/dc20ctrl.tar.gz
 *
 *	This package is derived from the dc20 package, built by Karl Hakimian
 *	<hakimian@aha.com> that you can find it at ftp.eecs.wsu.edu in the
 *	/pub/hakimian directory. The complete URL is:
 *	    ftp://ftp.eecs.wsu.edu/pub/hakimian/dc20.tar.gz
 *
 *	This package also includes a sligthly modified version of the Comet to ppm
 *	conversion routine written by YOSHIDA Hideki <hideki@yk.rim.or.jp>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published 
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Setup the Kodak DC20 camera serial speed.
 *
 *	$Id: init_dc20.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "init_dc20.h"
#include "main.h"
#include "send_pck.h"

static unsigned char init_pck[] = INIT_PCK;

static struct pkt_speed speeds[] = SPEEDS;

static struct termios tty_orig;

int init_dc20(char *device, speed_t speed)
{
  struct termios tty_new;
  int tfd, speed_index;
	
  fprintf(stderr, "port was %s and speed was %d \n", device, speed);
	
  for (speed_index = 0; speed_index < NUM_OF_SPEEDS; speed_index++) {
    if (speeds[speed_index].baud == speed) {
      init_pck[2] = speeds[speed_index].pkt_code[0];
      init_pck[3] = speeds[speed_index].pkt_code[1];
      break;
    }
  }
	
  fprintf (stderr, "int2 is %x and int 3 is %x \n", init_pck[2], init_pck[3]);

  if (init_pck[2] == 0) {
    if (!quiet) fprintf(stderr, "%s: init_dc20: error: unsupported baud rate.\n", __progname);
    return -1;
  }

  /*
    Open device file.
  */
  if ((tfd = open(device, O_RDWR)) == -1) {
    if (!quiet) {
      perror("open");
      fprintf(stderr, "%s: init_dc20: error: could not open %s for read/write\n", __progname, device);
    }
    return -1;
  }
  /*
    Save old device information to restore when we are done.
  */
  if (tcgetattr(tfd, &tty_orig) == -1) {
    if (!quiet) {
      perror("tcgetattr");
      fprintf(stderr, "%s: init_dc20: error: could not get attributes\n", __progname);
    }
    return -1;
  }

  memcpy((char *)&tty_new, (char *)&tty_orig, sizeof(struct termios));
  /*
    We need the device to be raw. 8 bits even parity on 9600 baud to start.
  */
  cfmakeraw(&tty_new);
  tty_new.c_oflag &= ~CSTOPB;
  tty_new.c_cflag |= PARENB;
  tty_new.c_cflag &= ~PARODD;
  tty_new.c_cc[VMIN] = 0;
  tty_new.c_cc[VTIME] = 10;
  cfsetospeed(&tty_new, B9600);
  cfsetispeed(&tty_new, B9600);

  if (tcsetattr(tfd, TCSANOW, &tty_new) == -1) {
    if (!quiet) {
      perror("tcsetattr");
      fprintf(stderr, "%s: init_dc20: error: could not set attributes\n", __progname);
    }
    return -1;
  }

  if (send_pck(tfd, init_pck) == -1) {
    /*
     *	Ok, we tried at 9600, but the machine may be at a different speed
     *	Let's try our speed.
     */

    for (speed_index = NUM_OF_SPEEDS-1; speed_index > 0; speed_index--) {
      if (verbose) printf("%s: init_dc20: changing speed to %d\n", __progname, (int)speeds[speed_index].baud);

      cfsetospeed(&tty_new, speeds[speed_index].baud);
      cfsetispeed(&tty_new, speeds[speed_index].baud);

      if (tcsetattr(tfd, TCSANOW, &tty_new) == -1) {
	if (!quiet) {
	  perror("tcsetattr");
	  fprintf(stderr, "%s: init_dc20: error: could not set attributes\n", __progname);
	}
	return -1;
      }
      if (send_pck(tfd, init_pck) != -1)
	break;
    }

    if (speed_index == 0) {
      tcsetattr(tfd, TCSANOW, &tty_orig);
      if (!quiet) fprintf(stderr, "%s: init_dc20: error: no suitable baud rate\n", __progname);
      return -1;
    }
  }
  /*
    Set speed to requested speed. Also, make a long timeout (we need this for
    erase and shoot operations)
  */
  tty_new.c_cc[VTIME] = 150;
  cfsetospeed(&tty_new, speed);
  cfsetispeed(&tty_new, speed);

  if (tcsetattr(tfd, TCSANOW, &tty_new) == -1) {
    if (!quiet) {
      perror("tcsetattr");
      fprintf(stderr, "%s: init_dc20: error: could not set attributes\n", __progname);
    }
    return -1;
  }
  fprintf(stderr, "About to return from init_dc20!");
  return tfd;
}

void close_dc20(int fd)
{
	/*
	 *	Put the camera back to 9600 baud
	 */

	init_pck[2] = speeds[0].pkt_code[0];
	init_pck[3] = speeds[0].pkt_code[1];
	if (send_pck(fd, init_pck) == -1) {
		if (!quiet) fprintf(stderr, "%s: close_dc20: error: could not set attributes\n", __progname);
	}

/*
 Restore original device settings.
*/
	if (tcsetattr(fd, TCSANOW, &tty_orig) == -1 && !quiet) {
		perror("tcsetattr");
		fprintf(stderr, "%s: close_dc20: error: could not set attributes\n", __progname);
	}

	if (close(fd) == -1 && !quiet) {
		perror("close");
		fprintf(stderr, "%s: close_dc20: error: could not close device\n", __progname);
	}
}
