#ifndef LINT
static char *rcsid="$Id: eph_read.c,v 1.3 2000/09/25 19:37:36 ole Exp $";
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
	$Log: eph_read.c,v $
	Revision 1.3  2000/09/25 19:37:36  ole
	Merge with sf HEAD branch
	
	Revision 1.2  2000/08/24 05:04:27  scottf
	adding language support
	
	Revision 1.1.1.1.2.1  2000/07/05 11:07:49  ole
	Preliminary support for the Olympus C3030-Zoom USB by
	Fabrice Bellet <Fabrice.Bellet@creatis.insa-lyon.fr>.
	(http://lists.styx.net/archives/public/gphoto-devel/2000-July/003858.html)
	
	Revision 2.7  1999/08/01 21:36:54  crosser
	Modify source to suit ansi2knr
	(I hate the style that ansi2knr requires but you don't expect me
	to write another smarter ansi2knr implementation, right?)

	Revision 2.6  1999/03/06 13:37:08  crosser
	Convert to autoconf-style

	Revision 2.5  1998/10/18 13:18:27  crosser
	Put RCS logs and I.D. into the source

	Revision 2.4  1998/08/01 13:12:41  lightner
	change Windows logic and timeouts
	
	Revision 2.3  1998/01/18 02:16:45  crosser
	DOS support
	
	Revision 2.2  1998/01/05 19:49:14  lightner
	Win32 syntax error fixed: fd changed to iob->fd
	
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
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>

#include "eph_io.h"

#ifdef MSWINDOWS
#define ERRNO GetLastError()
#else
#define ERRNO errno
#endif

/*
	Platform-dependant implementation of read with timeout
*/

#if defined(MSWINDOWS)

#include <windows.h>
#include <raserror.h>
#include <stdio.h>
#include <time.h>

#define MIN_TIMEOUT (1000)  /* 1 second...doesn't change so often! */

size_t
eph_readt(eph_iob *iob,char *buf,size_t length,long timeout_usec,int *rc)
{
	BOOL stat;
	DWORD rlen;
	DWORD timeout_ms;
	DWORD interval;
	DWORD multiplier;
	DWORD constant;

	if (timeout_usec == 0) {
		/* return immediately if no read data ready */
		interval = MAXDWORD;
		multiplier = 0;
		constant = 0;
	} else {
		/* normal read timeout */
		timeout_ms = timeout_usec/1000;
		if (timeout_ms < MIN_TIMEOUT) timeout_ms = MIN_TIMEOUT;
		interval = 0;
		multiplier = 0;
		constant = timeout_ms;
	}

	/* reset timeout intervals if changed */
	if (interval != iob->worktimeouts.ReadIntervalTimeout ||
	    multiplier != iob->worktimeouts.ReadTotalTimeoutMultiplier ||
	    constant != iob->worktimeouts.ReadTotalTimeoutConstant) {
		iob->worktimeouts.ReadIntervalTimeout = interval;
		iob->worktimeouts.ReadTotalTimeoutMultiplier = multiplier;
		iob->worktimeouts.ReadTotalTimeoutConstant = constant;
		if (!SetCommTimeouts(iob->fd, &iob->worktimeouts)) {
			eph_error(iob,ERRNO,
				"SetCommTimeouts set error %s",
				strerror(ERRNO));
			return (size_t)-1;
		}
	}

	*rc=0;
	stat = ReadFile(iob->fd,buf,length,&rlen,NULL);
	/* no timeout error if timeout was zero */
	if ((!stat || (rlen == 0)) && (timeout_usec != 0)) {
		SetLastError((DWORD)ERROR_SEM_TIMEOUT);
		return (size_t)-1;
	} else {
		SetLastError((DWORD)0L);
		*rc = rlen ? 1 : 0;
		return (size_t)rlen;
	}
}

#elif defined(UNIX)

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif


size_t
eph_readt(eph_iob *iob,char *buf,size_t length,long timeout_usec,int *rc)
{
	fd_set rfds,wfds,efds;
	struct timeval timer;

	if (length == 0) return 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);
	FD_SET(iob->fd,&rfds);
	FD_SET(iob->fd,&efds);
	timer.tv_sec=timeout_usec / 1000000L;
	timer.tv_usec=timeout_usec % 1000000L;

	(*rc)=select((iob->fd)+1,&rfds,&wfds,&efds,&timer);
	if ((*rc) == 0) {
		return 0;
	}
	if (((*rc) < 0) || (FD_ISSET(iob->fd,&efds))) return -1;

	return read(iob->fd,buf,length);
}

#elif defined(DOS)

#include <dos.h>
#include "comio.h"
#include "usleep.h"

#define TO_USECS (1L * 1000000L)

size_t
eph_readt(eph_iob *iob,char *buf,size_t length,long timeout_usec,int *rc)
{
	int ch, reset = 0, rlen = 0;
	unsigned char *p = buf;

	if (length == 0) return 0;

	*rc = 0;
	start_time();
#if 0
	while (elasped_usecs() <= ((unsigned long)TO_USECS + (length * 1000L))) 
#else
	while (elasped_usecs() <= (unsigned long)timeout_usec)
#endif
	{
		while (ttchk()) {
			*rc = 0;
			reset = 1;
			if (rlen < 0) rlen = 0;
			ch = ttinc();
			*(p++) = ch;
			if (++rlen >= length) {
				return (size_t)rlen;
			}
		}
		if (reset) {
			start_time();
			reset = 0;
		}
	}
	return (size_t)rlen;
}

#else
 # error platform not defined
#endif
