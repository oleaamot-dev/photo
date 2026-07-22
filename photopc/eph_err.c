#ifndef LINT
static char *rcsid="$Id: eph_err.c,v 1.3 2000/09/25 19:37:36 ole Exp $";
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
	$Log: eph_err.c,v $
	Revision 1.3  2000/09/25 19:37:36  ole
	Merge with sf HEAD branch
	
	Revision 1.2  2000/08/24 05:04:27  scottf
	adding language support
	
	Revision 1.1.1.1.2.1  2000/07/05 11:07:49  ole
	Preliminary support for the Olympus C3030-Zoom USB by
	Fabrice Bellet <Fabrice.Bellet@creatis.insa-lyon.fr>.
	(http://lists.styx.net/archives/public/gphoto-devel/2000-July/003858.html)
	
	Revision 2.6  1999/08/01 21:36:54  crosser
	Modify source to suit ansi2knr
	(I hate the style that ansi2knr requires but you don't expect me
	to write another smarter ansi2knr implementation, right?)

	Revision 2.5  1999/07/28 19:57:52  crosser
	reorder includes

	Revision 2.4  1999/03/06 13:37:08  crosser
	Convert to autoconf-style

	Revision 2.3  1998/10/18 13:18:27  crosser
	Put RCS logs and I.D. into the source

	Revision 2.2  1998/01/18 02:16:45  crosser
	DOS support
	
	Revision 2.1  1998/01/03 19:57:13  crosser
	Fix Windows things, improve error reporting
	
	Revision 2.0  1998/01/02 19:20:11  crosser
	Added support for Win32
	
	Revision 1.1  1997/08/17 08:59:54  crosser
	Initial revision
	
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include "eph_io.h"
#include "eph_priv.h"

static char *eph_errmsg[] = {
	/* 10001 */	"Data too long",
	/* 10002 */	"Timeout",
	/* 10003 */	"Unexpected amount of data read",
	/* 10004 */	"Bad packet header received",
	/* 10005 */	"Bad CRC on packet",
	/* 10006 */	"Bad speed value",
	/* 10007 */	"No memory",
	/* 10008 */	"Bad arguments",
	/* 10009 */	"",
	/* 10010 */	"",
	/* 10011 */	"",
	/* 10012 */	"",
	/* 10013 */	"",
	/* 10014 */	"",
	/* 10015 */	"",
};

#ifndef HAVE_STRERROR
char *strerror(int err) {
	static char buf[32];
	sprintf(buf,"System error %d",err);
	return buf;
}
#endif

/*
  We do not do any buffer override checks here because we are sure
  that the function is called *only* from within our library.
*/
void
eph_error (eph_iob *iob,int err,char *fmt,...)
{
	va_list ap;
	char *msg=NULL;
	char msgbuf[512];

	va_start(ap,fmt);

	if (fmt) {
		vsprintf(msgbuf,fmt,ap);
	} else {
		if ((err >= ERR_BASE) && (err < ERR_MAX)) {
			msg=eph_errmsg[err-ERR_BASE];
		} else {
			msg=strerror(err);
		}
		strcpy(msgbuf,msg);
	}
	va_end(ap);
	(iob->errorcb)(err,msgbuf);
}
