/*
 *	File: session.c
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
 *	Track the session number writing in a dot file.
 *
 *	$Id: session.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "session.h"
#include "main.h"

#define RC_NAME	".dc20ctrlrc"
#define RC_LEN	512

static int	rcd;

/*
 *	Get the session number from the dot file
 */

int get_session(void)
{
	int		 retval = 0;
	char	 rc_name[RC_LEN],
			*home_dir;

	if ((rcd = open(RC_NAME, O_RDWR, 0644)) < 0) {
		if ((home_dir = getenv("HOME")) == NULL) {
			if (!quiet) fprintf(stderr, "%s: get_session: error: cannot get home directory\n", __progname);
			return -1;
		}
		sprintf(rc_name, "%s/" RC_NAME, home_dir);
		if ((rcd = open(rc_name, O_RDWR | O_CREAT, 0644)) < 0) {
			if (!quiet) fprintf(stderr, "%s: get_session: warning: cannot open rc file\n", __progname);
		}
	}

	if (rcd >= 0) read(rcd, &retval, sizeof(retval));

	return retval;
}

/*
 *	Record the new session number to the dot file
 */

int put_session(int session)
{
	int		 retval = 0;
	char	 rc_name[RC_LEN],
			*home_dir;

	if (rcd < 0) {
		if ((home_dir = getenv("HOME")) == NULL) {
			if (!quiet) fprintf(stderr, "%s: put_session: error: cannot get home directory\n", __progname);
			return -1;
		}
		sprintf(rc_name, "%s/" RC_NAME, home_dir);
		if ((rcd = open(rc_name, O_RDWR | O_CREAT, 0644)) < 0) {
			if (!quiet) fprintf(stderr, "%s: put_session: warning: cannot open rc file\n", __progname);
		}
	}

	if (rcd >= 0) {
		lseek(rcd, 0, SEEK_SET);
		write(rcd, &session, sizeof(session));
		close(rcd);
	}

	return retval;
}
