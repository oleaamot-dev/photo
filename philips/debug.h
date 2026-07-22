/*  $Id: debug.h,v 1.6 2000/10/28 10:48:55 ole Exp $ */

/* 
 * Philips Digital Camera interface library
 *
 * Portions of this code were adapted from the ricoh_300z library
 * Copyright (C) 1998,1999 Clifford Wright.  
 * 
 * Portions of this code are from dc3play program Copyright (C)
 * 1997 Jun-ichiro Itoh.
 *
 * Copyright (c) 1999 Bob Paauwe
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

/* debuging macros */
#define dprintf(x)						\
	{ if (philips_debugflag) {				\
		fprintf(stderr, __FILE__ ":%d: ", __LINE__);	\
		fprintf x;					\
	  }							\
	}
#define dpprintf(x)						\
    {								\
	int i;							\
	if (philips_debugflag) {				\
	    fprintf(stderr, __FILE__ ":%d: ", __LINE__);	\
	    fprintf x;					\
	    for (i = 0; i < cam_data.length; i++)				\
		fprintf(stderr, "%02x ", cam_data.data[i]);		\
	    fprintf(stderr, "\n");				\
		}							\
    }

#define dcprintf() \
    {								\
	if (philips_debugflag) { \
	    fprintf(stderr, __FILE__ ":%d: ", __LINE__); \
	    fprintf(stderr, "%s -> ", command_name(cmd, data, len)); \
	    for (i = 0; i < cam_data->length; i++) \
			fprintf(stderr, "%02X ", cam_data->data[i]); \
	    fprintf(stderr, "\n"); \
		} \
	}

/* the following are left as globals so a debugger can set them */
int philips_debugflag = 0;	/* non-zero means print debug messages */
int philips_dumpflag = 0;	/* print serial port stream debug flag */
int philips_dumpmaxlen = 256;	/* max line length for dumpflag data */
int philips_verbose = 0;	/* flag to print download messages */
int philips_echobackrate = 5;	/* how often to print download messages */
/*
 *  Give the command bytes a name to make debugging easier
 */
struct	PHILIPS_CMD	{
	char	*name;
	u_char	*codes;
	int		len;
	} philips_commands[] = {
		{"hello", "\x31\x00\x00\x00", 4},
		{"set baud", "\x32\x07", 2},
		{"bye", "\x37", 1},
		{"set exposure", "\x50\x03", 2},
		{"set white", "\x50\x04", 2},
		{"set zoom", "\x50\x05", 2},
		{"set flash", "\x50\x06", 2},
		{"set record mode", "\x50\x07", 2},
		{"set compression", "\x50\x08", 2},
		{"set resolution", "\x50\x09", 2},
		{"set date/time", "\x50\x0A", 2},
		{"set copyright", "\x50\x0F", 2},
		{"set camera mode", "\x50\x12", 2},
		{"set macro", "\x50\x16", 2},
		{"get camera operating mode", "\x51\x00\x00", 3},
		{"get # images", "\x51\x00\x01", 3},
		{"get total bytes", "\x51\x00\x05", 3},
		{"get available bytes", "\x51\x00\x06", 3},
		{"get exposure", "\x51\x03", 2},
		{"get white", "\x51\x04", 2},
		{"get zoom", "\x51\x05", 2},
		{"get flash", "\x51\x06", 2},
		{"get record mode", "\x51\x07", 2},
		{"get compression", "\x51\x08", 2},
		{"get resolution", "\x51\x09", 2},
		{"get date/time", "\x51\x0A", 2},
		{"get copyright", "\x51\x0F", 2},
		{"get camera mode", "\x51\x12", 2},
		{"get macro", "\x51\x16", 2},
		{"take picture", "\x60\x01", 2},
		{"delete image", "\x92", 1},
		{"select image", "\x93", 1},
		{"get image name", "\x95\x00", 2},
		{"get image memo", "\x95\x02", 2},
		{"get image date/time", "\x95\x03", 2},
		{"get image size", "\x95\x04", 2},
		{"get current image #", "\x96", 1},
		{"set mode erase", "\x97", 1}, 
		{"get image", "\xA0", 1},
		{"put image", "\xA1", 1},
		{"put image data", "\xA2", 1},
		{"get image thumbnail", "\xA4", 1}
		};
int	num_philips_commands = 42;

/*****  Debugging Commands   *****************************************/

char *hexstr ( u_char hex )
{
	static char str[10];

	sprintf ( str, "%02X ", hex );
	return ( str );
}

char *command_name ( class, command, c_len )
u_char	class;
u_char	*command;
int		c_len;
{
	int		i, x, match, len;
	static	char	buf[128];

	if ( c_len > 5 ) c_len = 5;
	sprintf ( buf, "unknown command: %02X ", class );
	for ( i = 0; i < c_len; i++ )
		strcat ( buf, hexstr(command[i]) );

	for ( i = 0; i < num_philips_commands; i++ ) {
		len = philips_commands[i].len;
		if ( philips_commands[i].codes[0] == class ) {
			if ( --len == 0 ) {
				sprintf ( buf, "%s: %02X", philips_commands[i].name, class );
				return ( buf );
				}
			else {
				match = 1;
				for ( x = 0; x < len; x++ ) {
					if ( philips_commands[i].codes[x+1] != command[x] ) match = 0;
					}
				if ( match ) {
					sprintf ( buf, "%s: %02X ", philips_commands[i].name, class );
					for ( x = 0; x < c_len; x++ )
						strcat ( buf, hexstr(command[x]) );
					return ( buf );
					}
				}
			}
		}
	return ( buf );
}
		
/*  when debug flag dumpflag is set, print the serial port stream */
void
philips_dump_stream(dir, buf, len)
	int dir;
	char *buf;
	int len;
{
	size_t i;
	int truncate;

	if (!philips_dumpflag)
		return;

	truncate = 0;
	if (philips_dumpmaxlen < len) {
		len = philips_dumpmaxlen;
		truncate = 1;
	}

	dir &= 0xff;
	if (dir == '>')
		fprintf(stderr, "camera>cpu: ");
	else
		fprintf(stderr, "cpu>camera: ");

	for (i = 0; i < len; i++)
		fprintf(stderr, "%02x ", buf[i] & 0xff);
	if (truncate)
		fprintf(stderr, "...");
	fprintf(stderr, "\n");
}

