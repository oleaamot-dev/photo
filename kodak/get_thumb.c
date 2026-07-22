/*
 *	File: get_thumb.c
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
 *	Downloads a thumbnail from the Kodak DC20 camera memory.
 *
 *	$Id: get_thumb.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "get_thumb.h"
#include "main.h"
#include "send_pck.h"
#include "read_data.h"
#include "hash_mark.h"

static unsigned char thumb_pck[] = THUMBS_PCK;

int get_thumb(int fd, int which, unsigned char *thumb)
{
	unsigned char buf[1024];
	int i;

	thumb_pck[3] = (unsigned char)which;

	if (send_pck(fd, thumb_pck) == -1) {
		if (!quiet) fprintf(stderr, "%s: get_thumb: error: send_pck returned -1\n", __progname);
		return -1;
	}

	printf("Get thumb #%d: ", which);
	hash_init();

#ifdef DC25
	for (i = 0; i < 15; i++) {
		hash_mark(i, 14, 14);

#else
	for (i = 0; i < 5; i++) {
		hash_mark(i, 4, 40);
#endif

		if (read_data(fd, buf, 1024) == -1) {
			if (!quiet) fprintf(stderr, "%s: get_thumb: error: read_data returned -1\n", __progname);
			return -1;
		}

#ifdef DC25
		/*
		 * On the DC25, the thumbnail consists of 14400 bytes of
		 * rgb data.  Apparently the DC20 uses 4800 bytes of 
		 * grayscale info.
		 */
		memcpy((char *)&thumb[i*1024], buf, (i*1024 + 1024 > 14400)? 64 : 1024); 
#else
		memcpy((char *)&thumb[i*1024], buf, (i*1024 + 1024 > 4800)? 704 : 1024);
#endif
	}

	printf("\n");

#ifndef DC25
	/*
	 * I'm not sure if we really need this for the DC25, but we'll
	 * keep it in for now
	 */
	for (i = 0; i < 4800; i++) {

	  if (thumb[i] + 50 <= 255)
	    thumb[i] += 50;
	}
#endif
	return end_of_data(fd);
}
