/*
	$Id: eph_priv.h,v 1.3 2000/09/25 19:37:36 ole Exp $
*/

/*
	Copyright (c) 1997,1998 Eugene G. Crosser
	Copyright (c) 1998 Bruce D. Lightner (DOS/Windows support)

	You may distribute and/or use for any purpose modified or unmodified
	copies of this software if you preserve the copyright notice above.

	THIS SOFTWARE IS PROVIDED AS IS AND COME WITH NO WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED.  IN NO EVENT WILL THE
	COPYRIGHT HOLDER BE LIABLE FOR ANY DAMAGES RESULTING FROM THE
	USE OF THIS SOFTWARE.
*/

/*
	$Log: eph_priv.h,v $
	Revision 1.3  2000/09/25 19:37:36  ole
	Merge with sf HEAD branch
	
	Revision 1.2  2000/08/24 05:04:27  scottf
	adding language support
	
	Revision 1.1.1.1.2.2  2000/07/09 21:40:14  bellet
	2000-07-09  Fabrice Bellet  <Fabrice.Bellet@creatis.insa-lyon.fr>
	
		* photopc/eph_io.c : Fixed the ugly packet reading procedure
		previously using a huge buffer in the heap.
		* src/main.c : gpio_usb_init() is called in gpio_new().
		* photopc/photopc-usb.c : gpio_usb_find_device() is
		called after gpio_new().
	
	Revision 1.1.1.1.2.1  2000/07/05 11:07:49  ole
	Preliminary support for the Olympus C3030-Zoom USB by
	Fabrice Bellet <Fabrice.Bellet@creatis.insa-lyon.fr>.
	(http://lists.styx.net/archives/public/gphoto-devel/2000-July/003858.html)
	
	Revision 2.7  1999/01/17 09:50:55  crosser
	increase WRT*DELAYs

	Revision 2.6  1998/12/06 08:40:34  crosser
	chnage iniitimeout for Nikon

	Revision 2.5  1998/10/18 13:18:27  crosser
	Put RCS logs and I.D. into the source

	Revision 2.4  1998/08/28 22:01:48  crosser
	increase INITTIMEOUT to suite Nikon CP900
	
	Revision 2.3  1998/08/01 12:30:50  crosser
	flushinput function
	
	Revision 2.2  1998/01/27 21:52:55  crosser
	delays for smart multi-chunk writes
	
	Revision 2.1  1998/01/18 02:16:45  crosser
	DOS support
	
	Revision 2.0  1998/01/02 19:20:11  crosser
	Added support for Win32
	
	Revision 1.3  1997/11/03 23:25:11  crosser
	add immediate session finish command
	
	Revision 1.2  1997/09/12 09:42:35  crosser
	skip possible NULs prior to `camera signature'
	
	Revision 1.1  1997/08/17 08:59:54  crosser
	Initial revision
	
*/

#ifndef _EPH_PRIV_H
#define _EPH_PRIV_H

#include <sys/types.h>

#define RETRIES              3

#ifdef USE_VMIN_AND_VTIME
#define INITTIMEOUT    1700000L
#define DATATIMEOUT    1100000L
#define BIGDATATIMEOUT 1500000L
#define ACKTIMEOUT     1100000L
#define BIGACKTIMEOUT  1100000L
#define EODTIMEOUT     1100000L
#define CMDTIMEOUT    15000000L
#else
#define INITTIMEOUT    3000000L
#define DATATIMEOUT     200000L
#define BIGDATATIMEOUT 1500000L
#define ACKTIMEOUT      400000L
#define BIGACKTIMEOUT   800000L
#define EODTIMEOUT      400000L
#define CMDTIMEOUT    15000000L
#endif

/* Bruce and others say that adding 1ms delay before all writes is good.
   I think that they should rather be fine-tuned. */
#if 1
#define WRTPKTDELAY       1250L
#define WRTCMDDELAY       1250L
#define WRTPRMDELAY       1500L
#define WRTDELAY          2000L
#else
#define WRTPKTDELAY        250L
#define WRTCMDDELAY        250L
#define WRTPRMDELAY        500L
#define WRTDELAY          1000L
#endif
#define SPEEDCHGDELAY   100000L

#define SKIPNULS           200

#define ACK 0x06
#define DC1 0x11
#define NAK 0x15
/*#define NAK 0x11*/

#define CMD_SETINT 0
#define CMD_GETINT 1
#define CMD_ACTION 2
#define CMD_SETVAR 3
#define CMD_GETVAR 4

#define PKT_CMD 0x1b
#define PKT_DATA 0x02
#define PKT_LAST 0x03

#define SEQ_INITCMD 0x53
#define SEQ_CMD 0x43

typedef struct _eph_pkthdr {
	unsigned char typ;
	unsigned char seq;
} eph_pkthdr;

size_t eph_readt(eph_iob *iob,char *buf,size_t length,long timeout_usec,int *rc);

void eph_error(eph_iob *iob,int err,char *fmt,...);
int eph_flushinput(eph_iob *iob);
void eph_writeinit(eph_iob *iob);
void eph_writeack(eph_iob *iob);
void eph_writenak(eph_iob *iob);
int eph_waitack(eph_iob *iob,long timeout_usec);
int eph_waitcomplete(eph_iob *iob);
int eph_waitsig(eph_iob *iob);
int eph_waiteot(eph_iob *iob);

int eph_writepkt(eph_iob *iob,int typ,int seq,char *data,size_t length);
int eph_writecmd(eph_iob *iob,char *data,size_t length);
int eph_writeicmd(eph_iob *iob,char *data,size_t length);
int eph_readpkt(eph_iob *iob,eph_pkthdr *pkthdr,unsigned char *buf,size_t *length,long timeout_usec);

int eph_setispeed(eph_iob *iob,long val);

#endif
