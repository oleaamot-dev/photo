#ifndef LINT
static char *rcsid="$Id: eph_cmd.c,v 1.6 2000/09/25 20:38:13 ole Exp $";
#endif

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
	$Log: eph_cmd.c,v $
	Revision 1.6  2000/09/25 20:38:13  ole
	Several patches by Fabrice.Bellet@creatis.insa-lyon.fr
	
	   gphoto-20000820.progressbar.diff		more fluant progressbar
	   gphoto-20000820.timezone.diff		timezone fix (C3030Z)
	   gphoto-20000820.rescale-in-window.diff	rescale images in windows
	   gphoto-20000820.rescale-in-gallery.diff	rescale images in galleries
	   gphoto-20000820.canvas.diff			preliminary canvas support
	
	Revision 1.5  2000/09/25 20:00:05  ole
	Fabrice Bellet's gphoto-20000820.compilfixes.diff
	
	Revision 1.4  2000/09/25 19:37:36  ole
	Merge with sf HEAD branch
	
	Revision 1.6  2000/09/08 18:40:35  scottf
	fixed divide-by-zero bug for all percentages :P
	
	Revision 1.5  2000/08/25 21:22:46  scottf
	changed update_progress prototype to an int 0<percent<100
	
	Revision 1.4  2000/08/25 20:36:38  scottf
	readded in percentage updates. no more freezing GUI
	
	Revision 1.3  2000/08/24 05:04:27  scottf
	adding language support
	
	Revision 1.2.2.1  2000/07/05 11:07:49  ole
	Preliminary support for the Olympus C3030-Zoom USB by
	Fabrice Bellet <Fabrice.Bellet@creatis.insa-lyon.fr>.
	(http://lists.styx.net/archives/public/gphoto-devel/2000-July/003858.html)
	
	Revision 1.16  2000/02/13 11:15:01  crosser
	Kludge null setint for Nikon
	
	Revision 1.15  1999/08/01 21:36:54  crosser
	Modify source to suit ansi2knr
	(I hate the style that ansi2knr requires but you don't expect me
	to write another smarter ansi2knr implementation, right?)

	Revision 1.14  1999/07/28 19:56:31  crosser
	reorder includes

	Revision 1.13  1999/03/21 20:22:09  crosser
	change retry logic for first block (for Agfa 307)

	Revision 1.12  1999/03/06 13:37:08  crosser
	Convert to autoconf-style

	Revision 1.11  1999/01/21 09:12:56  crosser
	fix of retry logic from Richard Sharman

	Revision 1.10  1998/10/18 13:18:27  crosser
	Put RCS logs and I.D. into the source

	Revision 1.9  1998/08/09 13:24:59  crosser
	cleanup for DOS
	
	Revision 1.8  1998/08/09 12:57:56  crosser
	make right running report on 16bit architecture
	
	Revision 1.7  1998/02/26 00:50:39  crosser
	extra error message
	
	Revision 1.6  1998/02/16 06:15:44  lightner
	Parameterize tmpbuf size; fix tmpbuf memory leak
	
	Revision 1.5  1998/02/08 19:58:38  crosser
	Support low memory: chunked saving etc.
	
	Revision 1.4  1998/02/05 23:29:17  lightner
	Force 32-bit math in eph_getint()
	Only realloc() an extra 2048 bytes when buffer too small (DOS only)
	
	Revision 1.3  1998/01/27 21:52:23  crosser
	add NANOSLEEP
	
	Revision 1.2  1998/01/18 02:16:45  crosser
	DOS support
	
	Revision 1.1  1997/08/17 08:59:54  crosser
	Initial revision
	
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include "eph_io.h"
#include "eph_priv.h"

#define TMPBUF_SIZE (2048)

#define MAYRETRY(rc) ((rc == -2) || (rc == NAK))

int
eph_writecmd(eph_iob *iob,char *data,size_t length)
{
	return eph_writepkt(iob,PKT_CMD,SEQ_CMD,data,length);
}

int
eph_writeicmd(eph_iob *iob,char *data,size_t length)
{
	return eph_writepkt(iob,PKT_CMD,SEQ_INITCMD,data,length);
}

int
eph_setispeed(eph_iob *iob,long val)
{
	unsigned char buf[6];
	int rc;
	int count=0;

	buf[0]=CMD_SETINT;
	buf[1]=REG_SPEED;
	buf[2]=(val)&0xff;
	buf[3]=(val>>8)&0xff;
	buf[4]=(val>>16)&0xff;
	buf[5]=(val>>24)&0xff;
	do {
		if ((rc=eph_writeicmd(iob,buf,6))) return rc;
		rc=eph_waitack(iob,ACKTIMEOUT);
	} while (rc && (count++ < RETRIES));
	if (count >= RETRIES)
		eph_error(iob,ERR_EXCESSIVE_RETRY,
				"excessive retries on setispeed");
	return rc;
}

int
eph_setint(eph_iob *iob,int reg,long val)
{
	unsigned char buf[6];
	int rc;
	int count=0;

	buf[0]=CMD_SETINT;
	buf[1]=reg;
	buf[2]=(val)&0xff;
	buf[3]=(val>>8)&0xff;
	buf[4]=(val>>16)&0xff;
	buf[5]=(val>>24)&0xff;

writeagain:
	if ((rc=eph_writecmd(iob,buf,6))) return rc;
	rc=eph_waitack(iob,(reg == REG_FRAME)?BIGACKTIMEOUT:ACKTIMEOUT);
	if (MAYRETRY(rc) && (count++ < RETRIES)) goto writeagain;
	if (count >= RETRIES)
		eph_error(iob,ERR_EXCESSIVE_RETRY,
				"excessive retries on setint");

	return rc;
}

int
eph_setnullint(eph_iob *iob,int reg)
{
      unsigned char buf[2];
      int rc;
      int count=0;

      buf[0]=CMD_SETINT;
      buf[1]=reg;

writeagain:
      if ((rc=eph_writecmd(iob,buf,2))) return rc;
      rc=eph_waitack(iob,(reg == REG_FRAME)?BIGACKTIMEOUT:ACKTIMEOUT);
      if (MAYRETRY(rc) && (count++ < RETRIES)) goto writeagain;
      if (count >= RETRIES)
              eph_error(iob,ERR_EXCESSIVE_RETRY,
                              "excessive retries on setnullint");

      return rc;
}

int
eph_getint(eph_iob *iob,int reg,long *val)
{
	unsigned char buf[32];
	eph_pkthdr pkt;
	int rc;
	size_t size=32;
	int count=0;

	(*val)=0L;
	buf[0]=CMD_GETINT;
	buf[1]=reg;

writeagain:
	if ((rc=eph_writecmd(iob,buf,2))) return rc;
readagain:
	rc=eph_readpkt(iob,&pkt,buf,&size,BIGDATATIMEOUT);
	if (MAYRETRY(rc) && (count++ < RETRIES)) goto writeagain;
	if ((rc == 0) && (pkt.typ == PKT_LAST) && (pkt.seq == 0)) {
		(*val)=((unsigned long)buf[0]) | ((unsigned long)buf[1]<<8) |
			((unsigned long)buf[2]<<16) | ((unsigned long)buf[3]<<24);
		eph_writeack(iob);
		return 0;
	} else if ((rc == -1) && (count++ < RETRIES)) {
		eph_writenak(iob);
		goto readagain;
	}
	if (count >= RETRIES)
		eph_error(iob,ERR_EXCESSIVE_RETRY,
				"excessive retries on getint");

	return rc;
}

int
eph_action(eph_iob *iob,int reg,char *val,size_t length)
{
	unsigned char buf[2050];
	int rc;
	int count=0;

	if (length > (sizeof(buf) - 2)) {
		eph_error(iob,ERR_DATA_TOO_LONG,"arg action length %ld",
				(long)length);
		return -1;
	}

	buf[0]=CMD_ACTION;
	buf[1]=reg;
	memcpy(buf+2,val,length);

writeagain:
	if ((rc=eph_writecmd(iob,buf,length+2))) return rc;
	rc=eph_waitack(iob,ACKTIMEOUT);

	if (MAYRETRY(rc) && (count++ < RETRIES)) goto writeagain;

	if (rc == 0) rc=eph_waitcomplete(iob);
	if (count >= RETRIES)
		eph_error(iob,ERR_EXCESSIVE_RETRY,
				"excessive retries on action");
	return rc;
}

int
eph_setvar(eph_iob *iob,int reg,char *val,off_t length)
{
	unsigned char buf[2048];
	int rc=0,seq=-1;
	int count=0;
	int pkttyp,pktseq;
	size_t pktsize,maywrite;
	off_t written=0;
	unsigned char *getpoint,*putpoint;

	getpoint=val;
	while (length && !rc) {
		if (seq == -1) {
			pkttyp=PKT_CMD;
			pktseq=SEQ_CMD;
			buf[0]=CMD_SETVAR;
			buf[1]=reg;
			putpoint=buf+2;
			maywrite=sizeof(buf)-2;
			pktsize=2;
		} else {
			pkttyp=PKT_DATA;
			pktseq=seq;
			putpoint=buf;
			maywrite=sizeof(buf);
			pktsize=0;
			(iob->runcb)(written,0);
		}
		if (length <= maywrite) {
			maywrite=length;
			if (pkttyp == PKT_DATA) pkttyp=PKT_LAST;
		}
		memcpy(putpoint,getpoint,maywrite);
		pktsize+=maywrite;
		length-=maywrite;
		getpoint+=maywrite;
		written+=maywrite;
		seq++;
writeagain:
		if ((rc=eph_writepkt(iob,pkttyp,pktseq,buf,pktsize)))
			return rc;
		rc=eph_waitack(iob,ACKTIMEOUT);
		if (MAYRETRY(rc) && (count++ < RETRIES)) goto writeagain;
	}
	if (count >= RETRIES)
		eph_error(iob,ERR_EXCESSIVE_RETRY,
				"excessive retries on setvar");
	return rc;
}

int
eph_getvar(eph_iob *iob,int reg,char **buffer,off_t *bufsize)
{
	unsigned char buf[2];
	eph_pkthdr pkt;
	int rc;
	int count=0;
	unsigned char expect=0;
	off_t index;
	size_t readsize;
	char *ptr;
	char *tmpbuf=NULL;
	size_t tmpbufsize=0;
	off_t total = *bufsize;
	
	long picsize=1, picread=0;

	if ((buffer == NULL) && (iob->storecb == NULL)) {
		eph_error(iob,ERR_BADARGS,
			"NULL buffer and no store callback for getvar");
		return -1;
	}

	if (buffer == NULL) {
		tmpbuf=(iob->realloccb)(NULL,(size_t)TMPBUF_SIZE);
		tmpbufsize=TMPBUF_SIZE;
		if (tmpbuf == NULL) {
			eph_error(iob,ERR_NOMEM,
				"could not alloc %lu for tmpbuf in getvar",
				(long)TMPBUF_SIZE);
			return -1;
		}
	}

	buf[0]=CMD_GETVAR;
	buf[1]=reg;

	if (reg == 14)
		eph_getint(iob, 12, &picsize);
	if (reg == 15)
		eph_getint(iob, 13, &picsize);
writeagain:
	if ((rc=eph_writecmd(iob,buf,2))) return rc;
	index=0;
readagain:
	if (picsize)
		update_progress(100 * picread / picsize);
	if (buffer) { /* read to memory reallocating it */
		if (((*bufsize) - index) < 2048) {
			if (iob->debug)
				printf("reallocing %lu",(unsigned long)(*bufsize));
#ifdef LOWMEMORY
			/* small memory! round up to next 2048 boundary */
			(*bufsize)=(((index + 2048)-1)/2048L+1)*2048L;
#else
			/* multiply current size by 2 and round up to 2048 boundary */
			(*bufsize)=((((*bufsize)*2)-1)/2048+1)*2048;
#endif
			if (iob->debug)
				printf(" -> %lu\n",(unsigned long)(*bufsize));
			(*buffer)=(iob->realloccb)(*buffer,*bufsize);
			if ((*buffer) == NULL) {
				eph_error(iob,ERR_NOMEM,
					"could not realloc %lu for getvar",
					(long)*bufsize);
				return -1;
			}
		}
		ptr=(*buffer)+index;
		readsize=(*bufsize)-index;
	} else { /* pass data to store callback */
		ptr=tmpbuf;
		readsize=tmpbufsize;
	}
	rc=eph_readpkt(iob,&pkt,ptr,&readsize,
			(expect || ((reg != REG_IMG) || (reg != REG_TMN)))?
						DATATIMEOUT:BIGDATATIMEOUT);
	if (MAYRETRY(rc) && (expect == 0) && (count++ < RETRIES)) {
		eph_writenak(iob);
		if (rc == -2) goto readagain;
		else goto writeagain;
	}
	if ((rc == 0) &&
	    ((pkt.seq == expect) || (pkt.seq  == (expect-1)))) {
		count=0;
		if (pkt.seq == expect) {
			index+=readsize;
picread += readsize;
			expect++;
			(iob->runcb)(index,total);
			if (buffer == NULL) {
				if (iob->debug)
					printf("storing %lu at %08lx\n",
						(unsigned long)readsize,
						(unsigned long)ptr);
				if ((iob->storecb)(ptr,readsize))
					return -1;
			}
		}
		eph_writeack(iob);
		if (pkt.typ == PKT_LAST) {
			if (buffer) (*bufsize)=index;
			if (tmpbuf) free(tmpbuf);
			return 0;
		}
		else goto readagain;
	}
	if ((rc <= 0) && (count++ < RETRIES)) {
		eph_writenak(iob);
		goto readagain;
	}
	if (tmpbuf) free(tmpbuf);
	if (count >= RETRIES)
		eph_error(iob,ERR_EXCESSIVE_RETRY,
				"excessive retries on getvar");
	return rc;
}
