/*
 *	Konica-qm-sio-sample version 1.00
 *
 *	Copyright (C) 1999 Konica corporation .
 *
 *                                <qm200-support@konica.co.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

#define MAX_MSG_LEN 256

void
_log(char *fname, int line, char *title, char *msg)
{
	static  int is_file_line_print = 1;
	char	fname_line[MAX_MSG_LEN];
	char	title_buf[MAX_MSG_LEN];
	char	*s;

	if( is_file_line_print == 1 ){
		sprintf(fname_line, "%s:%d:", fname, line );
		sprintf(title_buf, "[%s]", title );
		printf("%-12s %-6s ", fname_line, title_buf );
		is_file_line_print = 0;
	}
	printf( "%s", msg);
	s = msg;
	if( *s != '\0' ){
		while( *s != '\0' ){
			++s;
		}
		is_file_line_print = ( s[-1] == '\n' );
	}
	fflush(stdout);
}

char *
_ksprintf(const char *fmt, ...)
{
	static char	msg[MAX_MSG_LEN];
	va_list ap;

	sprintf(msg, "xxxx");
	va_start(ap, fmt);
	(void)vsnprintf(msg, MAX_MSG_LEN, fmt, ap);
	va_end(ap);
	return msg;
}

ok_t
_log_fatal(char *file, int line, char *msg)
{
	extern void fatal_cleanup();

	_log(file, line, "FATAL", msg );
	fatal_cleanup();
	return NG;
}

void
_log_msg(char *file, int line, char *msg)
{
	_log(file, line, "MSG", msg );
}

void
dummp(void)
{
	DB("");
}
