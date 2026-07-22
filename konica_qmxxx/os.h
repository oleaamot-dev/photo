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

typedef unsigned char 			byte;
typedef unsigned short 			word;
typedef enum { false=0, true=1} 	bool;
typedef enum { XON_XOFF, CRTS_CTS}	os_sio_mode_t;
#define OS_NULL				((void *)0)

ok_t	os_sio_debug(int b);
int	os_sio_open(char *sio_dev_name, os_sio_mode_t mode);
int	os_sio_close(void);
ok_t	os_sio_set_bps(int bps);
ok_t	os_sio_putchar(byte c);
long	os_sio_getchar(void);
void    os_sio_getchar_abort_sec(int sec);
long	os_sio_read_msec( char *buf, long len, long msec );
ok_t	os_file_save(byte *buf, long size, char *fname);
void	*os_malloc(long size);
void	*os_realloc(void *buf, long size);
void	os_free(void *buf);
void	os_msec_sleep(long msec);
void	os_exit(int n);
void	os_sprintf(char *buf, char *fmt, ... );
char	*os_name_of_char(byte c);

#define STX	((byte)0x02)		/* ESC quote */
#define ETX	((byte)0x03)		/* ESC quote */
#define ENQ	((byte)0x05)		/* ESC quote */
#define ACK	((byte)0x06)		/* ESC quote */
#define XOFF	((byte)0x11)		/* ESC quote */
#define XON	((byte)0x13)		/* ESC quote */
#define NACK	((byte)0x15)		/* ESC quote */
#define ETB	((byte)0x17)		/* ESC quote */
#define ESC	((byte)0x1b)		/* ESC quote */
#define EOT	((byte)0x04)		/* NOT ESC quote */
