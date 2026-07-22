/*
 *	File: get_info.c
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
 *	Read the information packet from the Kodak DC20 camera.
 *
 *	$Id: get_info.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "get_info.h"
#include "main.h"
#include "send_pck.h"
#include "read_data.h"

static unsigned char info_pck[] = INFO_PCK;

Dc20Info *get_info(int fd)
{
	static Dc20Info result;
	unsigned char buf[256];

	if (send_pck(fd, info_pck) == -1) {
		if (!quiet) fprintf(stderr, "%s: get_info: error: send_pck returned -1\n", __progname);
		return NULL;
	}

	if (verbose) printf("%s: get_info: read info packet\n", __progname);

	if (read_data(fd, buf, 256) == -1) {
		if (!quiet) fprintf(stderr, "%s: get_info: error: read_data returned -1\n", __progname);
		return NULL;
	}

	if (end_of_data(fd) == -1) {
		if (!quiet) fprintf(stderr, "%s: get_info: error: end_of_data returned -1\n", __progname);
		return NULL;
	}

	result.model = buf[1];
	result.ver_major = buf[2];
	result.ver_minor = buf[3];
	result.pic_taken = buf[8]<<8|buf[9];
#ifdef DC25
	/* Not sure where the previous line came from.  All the
	 * information I have says that even on the DC20 the number of
	 * standard res pics is in byte 17 and the number of high res pics
	 * is in byte 19.  This is definitely true on my DC25.
	 */
	result.pic_taken = buf[17]+buf[19];
#endif
	result.pic_left = buf[10]<<8|buf[11];
#ifdef DC25
	/* Not sure where the previous line came from.  All the
	 * information I have says that even on the DC20 the number of
	 * standard res pics left is in byte 23 and the number of high res 
	 * pics left is in byte 21.  It seems to me that the conservative
	 * approach is to report the number of high res pics left.
	 */
	result.pic_left = buf[21];
#endif
	result.flags.low_res = buf[23];
#ifdef DC25
	/* Not sure where the previous line came from.  All the
	 * information I have says that even on the DC20 the low_res
	 * byte is 11.
	 */
	result.flags.low_res = buf[11];
#endif
	result.flags.low_batt = buf[29];
		
	return &result;
}
