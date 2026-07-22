/*
 *	File: erase.c
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
 *	Erase all the pictures from the Kodak DC20 camera memory.
 *
 *	$Id: erase.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "erase.h"
#include "main.h"
#include "send_pck.h"
#include "read_data.h"

static unsigned char erase_pck[] = ERASE_PCK;

int erase(int fd, int which)
{
#ifdef DC25
	int count=0;

        erase_pck[3] = (unsigned char)which;
#endif

	if (send_pck(fd, erase_pck) == -1) {
		if (!quiet) fprintf(stderr, "%s: erase: error: send_pck returned -1\n", __progname);
		return -1;
	}

#ifdef DC25
	/*
	 * This block may really apply to the DC20 also, but since I
	 * don't have one, it's hard to say for sure.  On the DC25, erase
	 * takes long enought that the read may timeout without returning
	 * any data before the erase is complete.   We let this happen 
	 * up to 4 times, then give up.
	 */
	{
		int old_quiet = quiet;

		quiet=1;

		while ( count < 4 ) {

			if (end_of_data(fd) == -1) {
				count++;
			}
			else {
				break;
			}	
		}

		quiet = old_quiet;	
	}
	if ( count == 4 ) {
		if (!quiet) fprintf(stderr, "%s: erase: error: end_of_data returned -1\n", __progname);
		return -1;
	}
#else
	if (end_of_data(fd) == -1) {
		if (!quiet) fprintf(stderr, "%s: erase: error: end_of_data returned -1\n", __progname);
		return -1;
	}
#endif			
	return 0;
}
