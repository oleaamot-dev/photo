/*
 *	File: send_pck.c
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
 *	Send a command packet to the Kodak DC20 camera.
 *
 *	$Id: send_pck.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "send_pck.h"
#include "main.h"

int send_pck(int fd, unsigned char *pck)
{
	int n;
	unsigned char r;

#ifdef DC25
	/*
	 * Not quite sure why we need this, but the program works a whole
	 * lot better (at least on the DC25)  with this short delay.
	 */
	usleep(10);
#endif
	if (write(fd, (char *)pck, 8) != 8) {
		if (!quiet) {
			perror("write");
			fprintf(stderr, "%s: send_pck: error: write returned -1\n", __progname);
		}
		return -1;
	}

	if ((n = read(fd, (char *)&r, 1)) != 1) {
		if (!quiet) {
			perror("read");
			fprintf(stderr, "%s: send_pck: error: read returned -1\n", __progname);
		}
		return -1;
	}

	return (r == 0xd1) ? 0 : -1;
}
