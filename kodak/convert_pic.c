/*
 *	File: convert_pic.c
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
 *	Convert a COMET file to a 8 bit greyscale or 24 bit color format.
 *
 *	$Id: convert_pic.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "convert_pic.h"
#include "main.h"
#include "pixmaps.h"
#include "comet_to_ppm.h"

int convert_pic(char *base_name, int format, int orientation)
{
	FILE			*ifp;
	unsigned char	 pic[MAX_IMAGE_SIZE];
	int				 res,
					 image_size,
					 image_width,
					 net_width,
					 camera_header,
					 components;
	char			 file[1024],
					*extp;
	struct pixmap	*pp,
					*pp2;

	/*
	 *	Read the image in memory
	 */

	if ((ifp = fopen(base_name, "rb")) == NULL) {
		if (!quiet) fprintf(stderr, "%s: convert_pic: error: cannot open %s for reading\n", __progname, base_name);
		return -1;
	}

	if (fread(pic, COMET_HEADER_SIZE, 1, ifp) != 1) {
		if (!quiet) {
			perror("fread");
			fprintf(stderr, "%s: convert_pic: error: cannot read COMET header\n", __progname);
		}
		fclose(ifp);
		return -1;
	}

	if (strncmp(pic, COMET_MAGIC, sizeof(COMET_MAGIC)) != 0) {
		if (!quiet) fprintf(stderr, "%s: convert_pic: error: file %s is not in COMET format\n", __progname, base_name);
		fclose(ifp);
		return -1;
	}

	if (fread(pic, LOW_CAMERA_HEADER, 1, ifp) != 1) {
		if (!quiet) {
			perror("fread");
			fprintf(stderr, "%s: convert_pic: error: cannot read camera header\n", __progname);
		}
		fclose(ifp);
		return -1;
	}

	res = pic[4];
	if (res == HIGH_RES) {
		/*
		 *	We just read a LOW_CAMERA_HEADER block, so resync with the
		 *	HIGH_CAMERA_HEADER length by reading once more one of this.
		 */
		if (fread(pic + LOW_CAMERA_HEADER, LOW_CAMERA_HEADER, 1, ifp) != 1) {
			if (!quiet) {
				perror("fread");
				fprintf(stderr, "%s: convert_pic: error: cannot resync with high resolution header\n", __progname);
			}
			fclose(ifp);
			return -1;
		}
	}

	if (fread(pic + CAMERA_HEADER(res), WIDTH(res), HEIGHT, ifp) != HEIGHT) {
		if (!quiet) {
			perror("fread");
			fprintf(stderr, "%s: convert_pic: error: cannot read picture\n", __progname);
		}
		fclose(ifp);
		return -1;
	}

	fclose(ifp);

	/*
	 *	Setup image size with resolution
	 */

	image_size = IMAGE_SIZE(res);
	image_width = WIDTH(res);
	net_width = image_width - LEFT_MARGIN - RIGHT_MARGIN(res);
	camera_header = CAMERA_HEADER(res);
	components = (format & SAVE_24BITS) ? 3 : 1;

	/*
	 *	Convert the image to 24 bits
	 */

	if ((pp = alloc_pixmap(net_width - 1, HEIGHT - BOTTOM_MARGIN - 1, components)) == NULL) {
		if (!quiet) fprintf(stderr, "%s: convert_pic: error: alloc_pixmap\n", __progname);
		return -1;
	}

	comet_to_pixmap(pic, pp);

	if (format & SAVE_ADJASPECT) {
		/*
		 *	Strech image
		 */
	
		if (res)
			pp2 = alloc_pixmap(320, HEIGHT - BOTTOM_MARGIN - 1, components);
		else
			pp2 = alloc_pixmap(net_width - 1, 373, components);
	
		if (pp2  == NULL) {
			if (!quiet) fprintf(stderr, "%s: convert_pic: error: alloc_pixmap\n", __progname);
			free_pixmap(pp);
			return -1;
		}

		if (res)
			zoom_x(pp, pp2);
		else
			zoom_y(pp, pp2);

		free_pixmap(pp);
		pp = pp2;
		pp2 = NULL;
	}

	/*
	 *	Remove the extension (.cmt) from the file name
	 */

	strcpy(file, base_name);
	if ((extp = strrchr(file, '.')) != NULL)
		*extp = '\0';

	save_pixmap(pp, file, orientation, format);

	free_pixmap(pp);

	return 0;
}
