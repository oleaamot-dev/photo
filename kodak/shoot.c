/*
 *	File: shoot.c
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
 *	Take a photo.
 *
 *	$Id: shoot.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "shoot.h"
#include "main.h"
#include "read_data.h"

static unsigned char shoot_pck[] = SHOOT_PCK;

int shoot(int fd)
{
	struct termios	 tty_temp,
					 tty_old;
	int				 result = 0;

	fprintf(stderr, "Made it to shoot!\n");

	if (tcgetattr(fd, &tty_old)== -1) {
		if (!quiet) {
			perror("tcgetattr");
			fprintf(stderr, "%s: shoot: error: could not get attributes\n", __progname);
		}
		return -1;
	}


	fprintf(stderr, "About to memcpy\n");
	memcpy((char *)&tty_temp, (char *)&tty_old, sizeof(struct termios));

	cfsetispeed(&tty_temp, B9600);
	cfsetospeed(&tty_temp, B9600);

	fprintf(stderr, "Made it back from speed calls!\n");

	if (write(fd, (char *)shoot_pck, 8) != 8) {
		if (!quiet) {
			perror("write");
			fprintf(stderr, "%s: shoot: error: write error\n", __progname);
		}
		return -1;
	}

	/*
	 *	WARNING: now we set the serial port to 9600 baud!
	 */

#ifndef DC25
	/* 
	 * Apparently there is a bug in the DC20 where the response to 
	 * the shoot request is always at 9600.  The DC25 does not have
	 * this bug, so we skip this block.
	 */
	if (tcsetattr(fd, TCSANOW, &tty_temp) == -1) {
		if (!quiet) {
			perror("tcsetattr");
			fprintf(stderr, "%s: shoot: error: could not set attributes\n", __progname);
		}
		return -1;
	}
#endif
	fprintf(stderr, "Made it back from tcsetattr!");

	if (read(fd, (char *)&result, 1) != 1) {
		if (!quiet) {
			perror("read");
			fprintf(stderr, "%s: shoot: error: read returned -1\n", __progname);
		}
		result = -1;
	} else {

	        fprintf (stderr, "result is %X !\n", result);
		result = (result == 0xD1) ? 0 : -1;
	}

	/*
	 *	We reset the serial to its original speed
	 */

#ifndef DC25
	/*
	 * We can skip this on the DC25 also.
	 */
	if (tcsetattr(fd, TCSANOW, &tty_old) == -1) {
		if (!quiet) {
			perror("tcsetattr");
			fprintf(stderr, "%s: shoot: error: could not reset attributes\n", __progname);
		}
		result = -1;
	}
#endif

	if (result == 0) {
#ifdef DC25
		/* 
		 * If we don't put this in, the next read will time out
		 * and return failure.
		 */
		sleep(3);
#endif
		if (end_of_data(fd) == -1) {
			if (!quiet) fprintf(stderr, "%s: shoot: error: end_of_data returned -1\n", __progname);
			result = -1;
		}

	}

	return result;
}
