#ifndef LINT
static char *rcsid="$Id: eph_iob.c,v 1.4 2000/09/25 20:38:13 ole Exp $";
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
	$Log: eph_iob.c,v $
	Revision 1.4  2000/09/25 20:38:13  ole
	Several patches by Fabrice.Bellet@creatis.insa-lyon.fr
	
	   gphoto-20000820.progressbar.diff		more fluant progressbar
	   gphoto-20000820.timezone.diff		timezone fix (C3030Z)
	   gphoto-20000820.rescale-in-window.diff	rescale images in windows
	   gphoto-20000820.rescale-in-gallery.diff	rescale images in galleries
	   gphoto-20000820.canvas.diff			preliminary canvas support
	
	Revision 1.3  2000/09/25 19:37:36  ole
	Merge with sf HEAD branch
	
	Revision 1.2  2000/08/24 05:04:27  scottf
	adding language support
	
	Revision 1.1.1.1.2.1  2000/07/05 11:07:49  ole
	Preliminary support for the Olympus C3030-Zoom USB by
	Fabrice Bellet <Fabrice.Bellet@creatis.insa-lyon.fr>.
	(http://lists.styx.net/archives/public/gphoto-devel/2000-July/003858.html)
	
	Revision 2.7  1999/12/11 14:10:15  crosser
	Support sgtty terminal control
	Proper "fake speed" handling (needed two values)
	
	Revision 2.6  1999/08/01 21:36:54  crosser
	Modify source to suit ansi2knr
	(I hate the style that ansi2knr requires but you don't expect me
	to write another smarter ansi2knr implementation, right?)

	Revision 2.5  1999/07/28 19:58:59  crosser
	reorder includes

	Revision 2.4  1999/03/06 13:37:08  crosser
	Convert to autoconf-style

	Revision 2.3  1998/10/18 13:18:27  crosser
	Put RCS logs and I.D. into the source

	Revision 2.2  1998/02/08 19:58:38  crosser
	Support low memory: chunked saving etc.
	
	Revision 2.1  1998/01/18 02:16:45  crosser
	DOS support
	
	Revision 2.0  1998/01/02 19:20:11  crosser
	Added support for Win32
	
	Revision 1.1  1997/08/17 08:59:54  crosser
	Initial revision
	
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#include <stdio.h>
#include "eph_io.h"

static void
deferrorcb(int errcode,char *errstr)
{
	fprintf(stderr,"Error %d: %s\n",errcode,errstr);
}

static void *
defrealloccb(void *old,size_t length)
{
	if (old) return realloc(old,length);
	else return malloc(length);
}

static void
defruncb(off_t count, off_t total)
{
	return;
}

eph_iob *
eph_new(void (*errorcb)(int errcode,char *errstr),
		void *(*realloccb)(void *old,size_t length),
		void (*runcb)(off_t count, off_t total),
		int (*storecb)(char *data,size_t size),
		int debug)
{
	eph_iob *iob;

	iob=(eph_iob *)malloc(sizeof(eph_iob));
	if (!iob) return iob;
	if (errorcb) iob->errorcb=errorcb;
	else iob->errorcb=deferrorcb;
	if (realloccb) iob->realloccb=realloccb;
	else iob->realloccb=defrealloccb;
	if (runcb) iob->runcb=runcb;
	else iob->runcb=defruncb;
	if (storecb) iob->storecb=storecb;
	else iob->storecb=NULL;
	iob->debug=debug;
#if defined(MSWINDOWS)
	iob->fd=INVALID_HANDLE_VALUE;
	memset((char*)&iob->savedcb,0,sizeof(iob->savedcb));
	memset((char*)&iob->savetimeouts,0,sizeof(iob->savetimeouts));
#elif defined(UNIX)
	iob->fd=-1;
#if defined(USE_TERMIOS)
	memset((char*)&iob->savetios,0,sizeof(iob->savetios));
#elif defined(USE_SGTTY)
	memset((char*)&iob->savesgtty,0,sizeof(iob->savesgtty));
#elif defined(USE_TERMIO)
	memset((char*)&iob->savetio,0,sizeof(iob->savetio));
#endif
#elif defined(DOS)
	iob->fd=-1;
#else
 # error "platform not defined"
#endif
	return iob;
}

void
eph_free(eph_iob *iob)
{
	free(iob);
}
