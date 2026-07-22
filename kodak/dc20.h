/*
 *	File: dc20.h
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
 *	Definitions for the Kodak DC20 serial protocol.
 *
 *	$Id: dc20.h,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#ifndef _DC20_H_
#define _DC20_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#ifdef USE_TIFF
#include <tiff34/tiffio.h>
#endif /* USE_TIFF */

#ifndef TRUE
#define TRUE	(1==1)  
#endif

#ifndef FALSE
#define FALSE	(!TRUE)
#endif

#ifndef NULL
#define NULL	0L
#endif

typedef struct dc20_info_s {
	unsigned char model;
	unsigned char ver_major;
	unsigned char ver_minor;
	int pic_taken;
	int pic_left;
	struct {
		unsigned int low_res:1;
		unsigned int low_batt:1;
	} flags;
} Dc20Info, *Dc20InfoPtr;

Dc20Info *get_info(int);

#define INIT_PCK	{0x41, 0x00, 0x96, 0x00, 0x00, 0x00, 0x00, 0x1A}
/*                               ^^^^^^^^^^
 *                               Baud rate: (see pkt_speed structure)
 *                                 0x96 0x00 -> 9600 baud
 *                                 0x19 0x20 -> 19200 baud
 *                                 0x38 0x40 -> 38400 baud
 *                                 0x57 0x60 -> 57600 baud
 *                                 0x11 0x52 -> 115200 baud
 */
#define INFO_PCK	{0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A}
#define SHOOT_PCK	{0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A}
#define ERASE_PCK	{0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A}
#define RES_PCK		{0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A}
/*                               ^^^^
 *                               Resolution: 0x00 = high, 0x01 = low
 */
#define THUMBS_PCK	{0x56, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A}
/*                                     ^^^^
 *                                     Thumbnail number
 */
#define PICS_PCK	{0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A}
/*                                     ^^^^
 *                                     Picture number
 */

struct pkt_speed {
	speed_t          baud;
	unsigned char    pkt_code[2];
};

#define NUM_OF_SPEEDS	5

#define SPEEDS	{ {   B9600, { 0x96, 0x00 } }, \
				  {  B19200, { 0x19, 0x20 } }, \
				  {  B38400, { 0x38, 0x40 } }, \
				  {  B57600, { 0x57, 0x60 } }, \
				  { B115200, { 0x11, 0x52 } }  }

#define HIGH_RES		0
#define LOW_RES			1

/*
 *	Image parameters
 */

#define LOW_CAMERA_HEADER	256
#define HIGH_CAMERA_HEADER	512
#define CAMERA_HEADER(r)	( (r) ? LOW_CAMERA_HEADER : HIGH_CAMERA_HEADER )

#define LOW_WIDTH			256
#define HIGH_WIDTH			512
#define WIDTH(r)			( (r) ? LOW_WIDTH : HIGH_WIDTH )

#define HEIGHT				243

#define LEFT_MARGIN			1

#define LOW_RIGHT_MARGIN	5
#define HIGH_RIGHT_MARGIN	10
#define RIGHT_MARGIN(r)		( (r) ? LOW_RIGHT_MARGIN : HIGH_RIGHT_MARGIN )

#define TOP_MARGIN			1

#define BOTTOM_MARGIN		1

#define BLOCK_SIZE			1024

#define LOW_BLOCKS			61
#define HIGH_BLOCKS			122
#define BLOCKS(r)			( (r) ? LOW_BLOCKS : HIGH_BLOCKS )

#define	LOW_IMAGE_SIZE		( LOW_BLOCKS * BLOCK_SIZE )
#define HIGH_IMAGE_SIZE		( HIGH_BLOCKS * BLOCK_SIZE )
#define IMAGE_SIZE(r)		( (r) ? LOW_IMAGE_SIZE : HIGH_IMAGE_SIZE )
#define MAX_IMAGE_SIZE		( HIGH_IMAGE_SIZE )

/*
 *	Comet file
 */

#define COMET_MAGIC			"COMET"
#define COMET_HEADER_SIZE	128
#define COMET_EXT			"cmt"

/*
 *	Pixmap structure
 */

struct pixmap {
	int				 width;
	int				 height;
	int				 components;
	unsigned char	*planes;
};

/*
 *	Rotations
 */

#define ROT_STRAIGHT	0x00
#define ROT_LEFT		0x01
#define ROT_RIGHT		0x02
#define ROT_HEADDOWN	0x03

#define ROT_MASK		0x03

/*
 *	File formats
 */

#define SAVE_RAW		0x01
#define SAVE_GREYSCALE	0x02
#define SAVE_24BITS		0x04
#define SAVE_FILES		0x07
#ifdef USE_JPEG
#define SAVE_JPEG		0x08
#endif /* USE_JPEG */
#ifdef USE_TIFF
#define SAVE_TIFF		0x10
#endif /* USE_TIFF */
#ifdef USE_PNG
#define SAVE_PNG		0x20
#endif /* USE_PNG */
#define SAVE_FORMATS	0x38
#define SAVE_ADJASPECT	0x80

/*
 *	External definitions
 */

extern char		*__progname;		/* Defined in /usr/lib/crt0.o */

#endif /* _DC20_H_ */
