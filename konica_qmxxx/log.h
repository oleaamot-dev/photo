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

typedef enum {  OK=0, NG=-1 }      ok_t;

void _log(char *fname, int line, char *title, char *msg);
char *_ksprintf(const char *fmt, ...);

ok_t _log_fatal(char *fname, int line, char *msg);
void _log_msg(char *fname, int line, char *msg);

#ifdef DEBUG
	#define DB(s)	_log_db(__FILE__, __LINE__, s )
	static int	is_debug = 1;
	static void _log_db(char *fname, int line, char *msg)
	{
		if( is_debug ){
			_log(fname,line,"DB",msg);
		}
	}
#else
	#define DB(s)	/* none */
#endif

#define FATAL(s)	_log_fatal(__FILE__, __LINE__, s);
#define MSG(s)		_log_msg(__FILE__, __LINE__, s);

#define OK(func)         if( (func) == NG ){ return FATAL(_ksprintf("ERR\n")); }
