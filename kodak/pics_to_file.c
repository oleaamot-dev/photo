/*
 *	File: pics_to_file.c
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
 *	Save some pictures from the Kodak DC20 camera memory to files.
 *
 *	$Id: pics_to_file.c,v 1.4 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "pics_to_file.h"
#include "main.h"
#include "get_pic.h"

int pics_to_file(int tfd, int mask, int low_res, char *base_name, char **list)
{
	int				 ofd,
					 image_size;
	unsigned char	 pic[MAX_IMAGE_SIZE];
	int				 i;

	for (i = 0; i < 16; i++) {
		if (mask & (1 << i)) {
			if (get_pic(tfd, i + 1, pic, low_res) == -1) {
				if (!quiet) fprintf(stderr, "%s: pics_to_file: error: get_pic returned -1\n", __progname);
				return -1;
			}

#ifdef DC25
			/*
			 * On DC25, we need to get the image size from the
			 * picture.  Perhaps this works on the DC20 also?
			 */
			image_size = IMAGE_SIZE(pic[4]);
#else
			image_size = IMAGE_SIZE(low_res);
#endif

			/*
			 *	Save COMET (raw) file
			 */
#ifndef sun
			asprintf(&list[i], base_name, i + 1, COMET_EXT);
#else
#warning Beware, the kodak library is not fully supported on Solaris
#endif
			if (!list[i]) {
				if (!quiet) {
					perror("asprintf");
					fprintf(stderr, "%s: pics_to_file: error: asprintf returned NULL\n", __progname);
				}
				return -1;
			}

			if ((ofd = open(list[i], O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1) {
				if (!quiet) {
					perror("open");
					fprintf(stderr, "%s: pics_to_file: error: open returned -1\n", __progname);
				}
				return -1;
			}
	
			if (write(ofd, COMET_MAGIC, sizeof(COMET_MAGIC)) != sizeof(COMET_MAGIC)) {
				if (!quiet) {
					perror("write");
					fprintf(stderr, "%s: pics_to_file: error: write COMET_MAGIC header\n", __progname);
				}
				close(ofd);
				return -1;
			}

			if (lseek(ofd, COMET_HEADER_SIZE, SEEK_SET) == -1) {
				if (!quiet) {
					perror("lseek");
					fprintf(stderr, "%s: pics_to_file: error: lseek returned -1\n", __progname);
				}
				close(ofd);
				return -1;
			}
	
			if (write(ofd, (char *)pic, image_size) != image_size) {
				if (!quiet) {
					perror("write");
					fprintf(stderr, "%s: pics_to_file: error: write body\n", __progname);
				}
				close(ofd);
				return -1;
			}
		
			close(ofd);
		} else
			list[i] = NULL;
	}

	return 0;
}
