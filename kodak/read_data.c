/*
 *	File: read_data.c
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
 *	Functions to read data packets from the Kodak DC20 camera.
 *
 *	$Id: read_data.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "main.h"
#include "read_data.h"

int read_data(int fd, unsigned char *buf, int sz)
{
	unsigned char ccsum;
	unsigned char rcsum;
	unsigned char c;
	int n;
	int r = 0;
	int i;

	for (n = 0; n < sz && (r = read(fd, (char *)&buf[n], sz - n)) > 0; n += r)
		;

	if (r <= 0) {
		if (!quiet) {
			perror("read: r <= 0");
			fprintf(stderr, "%s: read_data: error: read returned -1\n", __progname);
		}
		return -1;
	}

	if (n < sz || read(fd, &rcsum, 1) != 1) {
		if (!quiet) {
			perror("read");
			fprintf(stderr, "%s: read_data: error: buffer underrun or no checksum\n", __progname);
		}
		return -1;
	}

	for (i = 0, ccsum = 0; i < n; i++)
		ccsum ^= buf[i];

	if (ccsum != rcsum) {
		if (!quiet) fprintf(stderr, "%s: read_data: error: bad checksum (%02x != %02x)\n", __progname, rcsum, ccsum);
		return -1;
	}

	c = 0xd2;

	if (write(fd, (char *)&c, 1) != 1) {
		if (!quiet) {
			perror("write");
			fprintf(stderr, "%s: read_data: error: write ack\n", __progname);
		}
		return -1;
	}

	return 0;
}

int end_of_data(int fd)
{
	char c;

	if (read(fd, &c, 1) != 1) {
		if (!quiet) {
			perror("read");
			fprintf(stderr, "%s: end_of_data: error: read returned -1\n", __progname);
		}
		return -1;
	}

	if (c != 0) {
		if (!quiet) fprintf(stderr, "%s: end_of_data: error: bad EOD from camera (%02x)\n", __progname, (unsigned)c);
		return -1;
	}

	return 0;
}
