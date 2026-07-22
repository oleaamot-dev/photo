/*  $Id: philips_io.h,v 1.6 2000/10/28 10:48:55 ole Exp $ */

/* 
 * Philips Digital Camera interface library
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

/* Give the command classes some real names */
#define P_HELLO		0x31
#define P_SETBAUD	0x32
#define P_DISCONECT	0x37
#define P_GETVAL	0x51
#define P_SETVAL	0x50
#define P_SNAPPIC	0x60
#define P_DELPICT	0x92
#define P_SELECT	0x93
#define P_PUTIMG	0x94
#define P_GETIMG	0x95
#define P_NUMPICT	0x96
#define P_DELETE	0x97
#define P_GETPICT	0xA0
#define P_PUTPICT   0xA1
#define P_BLOCK		0xA2
#define P_GETTHUMB	0xA4

/* 
 * Camera model numbers.
 *
 * Each camera returns an identification number when it is
 * initialized. 
 *
 */


#define RDC_1		5		/* placeholder, not yet correct */
#define RDC_2		3		/* placeholder, not yet correct */
#define RDC_2E		4		/* placeholder, not yet correct */
#define RDC_100G	7		/* placeholder, not yet correct */
#define RDC_300		3000
#define RDC_300Z	3100
#define RDC_4200	4200
#define RDC_4300	4300
#define RDC_5000	5000
#define ESP80SXG	4000
#define ESP60SXG	1		/* placeholder, not yet correct */
#define ESP2		2		/* placeholder, not yet correct */
#define ESP50		6		/* placeholder, not yet correct */

struct CAM_DATA {
	u_char	class;
	u_char	data[4096];
	int		length;
	int		ack;
	int		blockno;
	int		ack_only;
	};

typedef struct	PHILIPS_CFG {
	long	picts;		/* number of pictures in camera memory */
	int		a_memory;	/* available camera memory */
	int		memory;		/* total camera memory */
	time_t	date;		/* camera date & time */
	int		date_dirty;	/* has date changed? */
	char	copyright[21];	/* picture copyright string */
	int		resolution;	/* camera resolution setting */
	int		compression;	/* camera compression setting */
	int		white;		/* camera white balance setting */
	int		exposure;	/* camera exposure setting */
	int		mode;		/* camera recording mode */
	int		flash;		/* camera flash mode */
	int		macro;		/* camera macro mode */
	int		zoom;		/* camera zoom setting */
	} PhilipsCfgInfo;

/* Camera Return Codes */

#define P_OK   "\x00\x00"       /* command completed OK */
#define P_BUSY "\x00\x04\xff"   /* camera busy, please wait */
#define P_INC  "\x03\x00"       /* incomplete command */
#define P_INV  "\x04\x00"       /* invalid parameter */
#define P_ERR  "\x05\x00"       /* Error ????? */
#define P_MODE "\x06\x00"       /* command not valid in this mode */
#define P_ERR1 "\x07\x00"       /* Error ????? */
#define P_ERR2 "\x18\x00"       /* Error ????? */


/* Miscellenous defines */

#define PHILIPS_RESENDS	2		/* number of retransmissions */


/* function prototypes (eventually) */

int philips_get_mode ();
int philips_gettotalbytes();
int philips_getavailbytes();
char *philips_getthumb();
int philips_getexposure();
int philips_getwhitelevel();
int philips_getzoom();
int philips_getflash();
int philips_getrecordmode();
int philips_getcompression();
int philips_getresolution();
int philips_getcopyright();
int philips_getmacro();
int philips_getnpicts();
int philips_getpictnum();
int philips_getpictname();
int philips_getpictmemo();
int philips_getpictsize();
int philips_getpictdate();
int philips_getpict();
int philips_set_mode();
int philips_setexposure();
int philips_setwhitelevel();
int philips_setzoom();
int philips_setflash();
int philips_setrecordmode();
int philips_setcompression();
int philips_setresolution();
int philips_setcopyright();
int philips_setmacro();
int philips_putpict();
int philips_wait();
int philips_read();
int philips_put();
int philips_getpacket();
int philips_setspeed();
int philips_hello();
int philips_init_query();
int philips_bye();
int philips_execcmd();
void philips_flush ();
void philips_dump_stream();
static void *philips_close_handler();
speed_t philips_baudconv();
int philips_setbaud();
char *philips_model();
PhilipsCfgInfo *philips_getcfginfo();
int philips_setcfginfo();
void philips_progress_bar( float progress, char *message );
