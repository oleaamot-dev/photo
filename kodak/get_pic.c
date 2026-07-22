/*
 *	File: get_pic.c
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
 *	Download a picture from the Kodak DC20 camera memory.
 *
 *	$Id: get_pic.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "get_pic.h"
#include "main.h"
#include "send_pck.h"
#include "read_data.h"
#include "hash_mark.h"

static unsigned char pic_pck[] = PICS_PCK;

int get_pic(int fd, int which, unsigned char *pic, int low_res)
{
	unsigned char buf[1024];
	int n = (low_res) ? 61 : 122;
	int i;

	pic_pck[3] = (unsigned char)which;

	if (send_pck(fd, pic_pck) == -1) {
		if (!quiet) fprintf(stderr, "%s: get_pic: error: send_pck returned -1\n", __progname);
		return -1;
	}

	printf("Get image #%d: ", which);
	hash_init();

	for (i = 0; i < n; i++) {
		hash_mark(i, n - 1, 40);

		if (read_data(fd, buf, 1024) == -1) {
			if (!quiet) fprintf(stderr, "%s: get_pic: error: read_data returned -1\n", __progname);
			return -1;
		}

#ifdef DC25	
		/* 
		 * If this is the first row, byte 4 tells us if this is 
		 * a low or hi-res image
		 */
		if ( i == 0 ) {
			if ( buf[4] == 0 ) {
				n=122;
		} else {
				n=61;	
			}
		}
#endif

		memcpy((char *)&pic[i*1024], buf, 1024);
	}

	printf("\n");
	return end_of_data(fd);
}
