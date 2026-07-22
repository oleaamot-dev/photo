/*
 *	File: thumbs_to_file.c
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
 *	Download some thumbnails from the Kodak DC20 camera memory.
 *
 *	$Id: thumbs_to_file.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "thumbs_to_file.h"
#include "main.h"
#include "get_thumb.h"
#include "pixmaps.h"

int thumbs_to_file(int tfd, int mask, char *base_name, int format, int orientation_mask)
{
	struct pixmap	*pp;
	int				 i,
					 x,
					 y;
#ifdef DC25
	/*
	 * Probably could use the worst case (DC25) buffer size for
	 * both.
	 */
	unsigned char	 thumb[14400];
#else
	unsigned char	 thumb[4800];
#endif
	char			 file[1024];

#ifdef DC25
	/* DC25 returns thumbnails in 3-byte (rgb) format */
	if ((pp = alloc_pixmap(80, 60, 3)) == NULL) {
#else
	if ((pp = alloc_pixmap(80, 60, 1)) == NULL) {
#endif
		if (!quiet) fprintf(stderr, "%s: thumbs_to_file: error: alloc_pixmap\n", __progname);
		return -1;
	}
	
	for (i = 0; i < 16; i++) {
		if (mask & (1 << i)) {
			if (get_thumb(tfd, i + 1, thumb) == -1) {
				if (!quiet) fprintf(stderr, "%s: thumbs_to_file: error: get_thumb returned -1\n", __progname);
				return -1;
			}

			for (y = 0; y < 60; y++) {
				for (x = 0; x < 80; x++) {
#ifdef DC25
					/* DC25 returns thumbnails in 3-byte (rgb) format */
					set_pixel_rgb(pp, x, y, thumb[x*3 + (y * 240)], thumb[x*3 + (y * 240) + 1], thumb[x*3 + (y * 240) +2] );
#else
					set_pixel(pp, x, y, thumb[x + (y * 80)]);
#endif

				}
			}

			sprintf(file, base_name, i+1);
	
#ifdef DC25
			save_pixmap(pp, file, (orientation_mask >> (i*2)) & ROT_MASK, SAVE_24BITS);
#else
			save_pixmap(pp, file, (orientation_mask >> (i*2)) & ROT_MASK, format);
#endif
		}
	}

	free_pixmap(pp);
	
	return 0;
}
