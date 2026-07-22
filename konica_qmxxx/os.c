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
#include <stdlib.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

#ifdef sun
#include <sys/filio.h>
#endif

#include "log.h"
#include "os.h"

#ifdef __FreeBSD__
	#if 3 <= __FreeBSD__
		#define FreeBSD_3
	#else
		#error FreeBSD-2.2.x is not support for kernel sio bug
	#endif
#endif

#ifdef DEBUG
void
os_debug(int on_off)
{
	is_debug = on_off;
}
#endif

static int			sio_fd = -1;
static struct termios		sio_org_termios;
static struct termios		sio_termios;

long
os_sio_read_msec( char *buf, long len, long msec )
{
	struct timeval	timeout;
	fd_set		fd_list;
	int		result;
	int		n;

	FD_ZERO( &fd_list );		/* clear fd_list */
	FD_SET(sio_fd, &fd_list );	/* add sio_fd to fd_list */

	timeout.tv_sec = 0;
	timeout.tv_usec = msec * 1000; /* msec * 1000microsecond */
	result = select(FD_SETSIZE,&fd_list,(fd_set *)0,(fd_set *)0,&timeout);
	if( result == 0 ){
		return 0; /* return zero byte if time out */
	}
	if( result == -1 ){
		return FATAL(_ksprintf("sio select error\n"));
	}
	if( FD_ISSET(sio_fd, &fd_list) ){
		if( ioctl(sio_fd, FIONREAD, &n) == -1 ){
			return FATAL(_ksprintf("illegual ioctl\n"));
		}
		if( n == 0 ){
			return FATAL(_ksprintf("sio EOF error\n"));
		}
		if( n > len ){
			n = len;
		}
		if( n != read(sio_fd, buf, n ) ){
			return FATAL(_ksprintf("sio illegual read\n"));
		}
		return n;
	}
	return FATAL(_ksprintf("sio illegual fd_list\n"));
}

ok_t
os_sio_open(char *sio_dev_name, os_sio_mode_t mode)
{
	tcflag_t	iflag;

	DB(_ksprintf("sio now open\n"));
	sio_fd = open(sio_dev_name, O_RDWR|O_NOCTTY|O_NDELAY);
	if( sio_fd == -1 ){
		return FATAL(_ksprintf("sio_open: %s open fail\n", sio_dev_name));
	}

	DB(_ksprintf("sio open end\n"));
	tcgetattr( sio_fd, &sio_org_termios );
	tcgetattr( sio_fd, &sio_termios );
	#if 0
	printf("i=%08x\n", (int)sio_termios.c_iflag );
	printf("o=%08x\n", (int)sio_termios.c_oflag );
	printf("c=%08x\n", (int)sio_termios.c_cflag );
	printf("l=%08x\n", (int)sio_termios.c_lflag );
	printf("xstart=0x%02x\n", sio_termios.c_cc[VSTART] );
	printf("xstop=0x%02x\n", sio_termios.c_cc[VSTOP] );
	printf(" 0 VEOF      0x%02x\n", sio_termios.c_cc[VEOF    ] );
	printf(" 1 VEOL      0x%02x\n", sio_termios.c_cc[VEOL    ] );
	printf(" 2 VEOL2     0x%02x\n", sio_termios.c_cc[VEOL2   ] );
	printf(" 3 VERASE    0x%02x\n", sio_termios.c_cc[VERASE  ] );
	printf(" 4 VWERASE   0x%02x\n", sio_termios.c_cc[VWERASE ] );
	printf(" 5 VKILL     0x%02x\n", sio_termios.c_cc[VKILL   ] );
	printf(" 6 VREPRINT  0x%02x\n", sio_termios.c_cc[VREPRINT] );
	printf(" 7 V_spare   0x%02x\n", 0 );
	printf(" 8 VINTR     0x%02x\n", sio_termios.c_cc[VINTR   ] );
	printf(" 9 VQUIT     0x%02x\n", sio_termios.c_cc[VQUIT   ] );
	printf("10 VSUSP     0x%02x\n", sio_termios.c_cc[VSUSP   ] );
	printf("11 VDSUSP    0x%02x\n", sio_termios.c_cc[VDSUSP  ] );
	printf("12 VSTART    0x%02x\n", sio_termios.c_cc[VSTART  ] );
	printf("13 VSTOP     0x%02x\n", sio_termios.c_cc[VSTOP   ] );
	printf("14 VLNEXT    0x%02x\n", sio_termios.c_cc[VLNEXT  ] );
	printf("15 VDISCARD  0x%02x\n", sio_termios.c_cc[VDISCARD] );
	printf("16 VMIN      0x%02x\n", sio_termios.c_cc[VMIN    ] );
	printf("17 VTIME     0x%02x\n", sio_termios.c_cc[VTIME   ] );
	printf("18 VSTATUS   0x%02x\n", sio_termios.c_cc[VSTATUS ] );
	printf("19 V_spare2  0x%02x\n", 0 );
	#endif

	#if defined(linux) || defined(FreeBSD_3) || defined(__NetBSD__) || defined(sun)
		switch( mode ){
		case XON_XOFF:	iflag = (IXON|IXOFF|IGNBRK);	break;
		case CRTS_CTS:	iflag = (CRTSCTS);		break;
		default:	return FATAL(_ksprintf("illegual mode"));
		}
		sio_termios.c_iflag     = iflag;
		sio_termios.c_cflag     = (CREAD|CS8);
		sio_termios.c_oflag     = 0;
		sio_termios.c_lflag     = 0;
		//sio_termios.c_cc[VMIN]  = 0;    
		//sio_termios.c_cc[VTIME] = 100;  
	#else
		#error This OS is not support
	#endif

	cfsetospeed(&sio_termios, B9600);
	cfsetispeed(&sio_termios, B9600);

	tcsetattr( sio_fd, TCSANOW, &sio_termios );
	DB(_ksprintf("sio open ok\n"));
	return OK;
}

int
os_sio_close(void)
{
	return close(sio_fd);
}

ok_t
os_sio_set_bps(int bps)
{
	int	val;

	switch( bps ){
	case 300:	val = B300;	break;
	case 600:	val = B600;	break;
	case 1200:	val = B1200;	break;
	case 2400:	val = B2400;	break;
	case 4800:	val = B4800;	break;
	case 9600:	val = B9600;	break;
	case 19200:	val = B19200;	break;
	case 38400:	val = B38400;	break;
	case 57600:	val = B57600;	break;
	case 115200:	val = B115200;	break;
	default:	return FATAL(_ksprintf("sio_set_speed: illegal bps=%d\n", bps ));
	}

	cfsetospeed(&sio_termios, val );
	cfsetispeed(&sio_termios, val );
	tcsetattr( sio_fd, TCSANOW, &sio_termios );
	return OK;
}

ok_t
os_sio_putchar(byte c)
{
	if( write(sio_fd, &c, sizeof(byte) ) != sizeof(byte) ){
		return FATAL(_ksprintf("Can't sio_putchar\n"));
	}
	DB(_ksprintf("putchar[%s]\n", os_name_of_char(c) ));
	return OK;
}

#define DEFAULT_GETCHAR_WAIT  60
static long getchar_wait_100msec_count = DEFAULT_GETCHAR_WAIT; /* 6sec */

void
os_sio_getchar_abort_sec(int sec)
{
	getchar_wait_100msec_count = 10*sec;	
}

long
os_sio_getchar(void)
{
	int	retry_count;
	int	read_bytes;
	byte	c;

	retry_count = getchar_wait_100msec_count;
	getchar_wait_100msec_count = DEFAULT_GETCHAR_WAIT;

	for(;;){
		read_bytes = os_sio_read_msec( &c, sizeof(byte), 100 );
		if( read_bytes == sizeof(byte) ){
			DB(_ksprintf("getchar[%s]\n", os_name_of_char(c) ));
			return (long)c;
		}
		if( read_bytes != 0 ){
			return FATAL(_ksprintf("sio_getchar: unknown error (%d)\n", read_bytes));
		}
		if( retry_count-- == 0 ){
		 	return FATAL(_ksprintf("sio_getchar: retry count over\n"));
		}
	}
}

ok_t
os_file_save(byte *buf, long size, char *fname)
{
	FILE	*fp;

	if( (fp=fopen(fname, "w")) == NULL ){
		return FATAL(_ksprintf("Can't open %s\n", fname ));
	}
	if( fwrite(buf, sizeof(byte), size, fp ) != size ){
		return FATAL(_ksprintf("Can't write to %s\n", fname ));
	}
	fclose(fp);
	return OK;
}

void
os_msec_sleep(long msec)
{
	usleep(msec*1000);
}

void
fatal_cleanup(void)
{
	if( sio_fd != -1 ){
		close(sio_fd);
	}
}

void
os_exit(int n)
{
	exit(n);
}

void	*
os_malloc(long size)
{
	void	*p;

	if( (p = malloc(size)) == NULL){
		FATAL(_ksprintf("Can not alloc"));
		return NULL;
	}
	return p;
	
}

void
os_free(void *ptr)
{
	free(ptr);
}

void *
os_realloc(void *buf, long size)
{
	void *p;
	if( (p=realloc( buf, size )) == NULL ){
		FATAL(_ksprintf("err on os_realloc"));
		return NULL;
	}
	return p;
}

void	os_sprintf(char *buf, char *fmt, ... )
{
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
}

char *
os_name_of_char(byte c)
{
	static char ans[3];

	switch( c ){
	case STX:	return "STX";
	case ETX:	return "ETX";
	case ETB:	return "ETB";
	case ENQ:	return "ENQ";
	case ACK:	return "ACK";
	case XOFF:	return "OFF";
	case XON:	return "XON";
	case ESC:	return "ESC";
	case EOT:	return "EOT";
	case NACK:	return "NACK";
	default:
			os_sprintf(ans, "%02x", c );
			return ans;
	}
}
