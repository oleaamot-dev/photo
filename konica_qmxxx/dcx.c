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
#include "log.h"
#include "os.h"
#include "dcx.h"
#ifdef DEBUG
	extern void	os_debug(int on_off);
	void		dcx_debug(int on_off) { is_debug = on_off; }
#endif

	/*------------------*/        /*---------------------------------*/
	/*    Status Code   */        /*  Description                    */
	/*------------------*/        /*---------------------------------*/
#define	RETVAL_SUCCESS         0x0000 /* Command successful              */
#define	RETVAL_ILLEGUAL_PARAM  0x0800 /* Illegal parameter               */
#define	RETVAL_SYSTEM_ERROR    0x0501 /* System error                    */
#define	RETVAL_UNSUPPORT       0x0C01 /* Unsupported command             */ 
				      /* or command not defined          */
#define	RETVAL_OTHER_EXECUTING 0x0C02 /* Other command executing         */
#define	RETVAL_UNKNOWN_ERROR   0x0FFF /* Unknown error                   */
#define	RETVAL_CARD_NOT_READ   0x0340 /* Card can not read 		 */
#define RETVAL_DCX_ERROR	RETVAL_UNKNOWN_ERROR

typedef struct {
	long	info_size;
	long	image_id;
	long	image_kbytes;
	bool	image_is_protect;
} dc_get_image_info_ans_t;

typedef struct {
	word status;
	word card_size;
	word picture_count;
	word pics_left;
	byte year;
	byte month;
	byte day;
	byte hour;
	byte minute;
	byte second;
	byte flash;
	byte iq_mode;
	byte self_macro;
	byte exp_comp;
	word total_pict;
	word total_strobo;
} dc_get_status_t;

static struct {
	long			all_bytes;
	dcx_percent_disp_func_t	func;
	int			old_disp;
} disp_progress;

/*----------------------------------------------------------*/
static byte is_esc_mask[256] = {
	/* 00 *//* 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f */
	/* 00 */    0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 10 */    0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
	/* 20 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 30 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 40 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 50 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 60 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 70 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 80 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 90 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* a0 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* b0 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* c0 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* d0 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* e0 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* f0 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 *	sio_check_sum_esc_read( byte *buf, long max_buf, long len )
 *
 *          o read len bytes from sio
 *          o [ESC+c] is changed to [~c] ( this byte not counted by len )
 *          o return check sum
 */
static long
sio_check_sum_esc_read( byte *buf, long max_buf, long len )
{
	register byte	c;
	register char	*out_pos;
	char		*inp_pos;
	long		rest_len;
	bool		is_esc = false;
	int		read_bytes;
	byte		check_sum = 0;
	const int	RETRY_MAX_COUNT  = 200; /* 2sec */
	int		retry_count      = 0;
	byte		*tmp_buf;
	// #define DEBUG_V
	#ifdef DEBUG_V
	int		adr = 0;
	#endif

	DB(_ksprintf("esc_read_max_buf=%d\n", max_buf));
	DB(_ksprintf("start sio_read checksum len=%d\n", len));
	if( len == 0 ){
		DB(_ksprintf("xxxxxxxx len == 0 xxxxxxxxx\n"));
		return 0;
	}
	if( (tmp_buf=os_malloc( len * 2 )) == OS_NULL ){
		return FATAL(_ksprintf("Can't alloc memory\n"));
	}
	out_pos = buf;
	inp_pos = buf;

	rest_len = len;
	do{
		for(retry_count=0; ; ){
			if( (read_bytes = os_sio_read_msec( tmp_buf, rest_len, 100 )) != 0 ){
				break;
			}
			if( retry_count++ == RETRY_MAX_COUNT ){
				os_free(tmp_buf);
				return FATAL(_ksprintf("sio_recv_esc_quoted_data: time over\n"));
			}
		}
		#ifdef DEBUG_V
		{ int i;
		DB(_ksprintf("read_bytes=%d [", read_bytes ));
		i = read_bytes;
		for(i=0; i<read_bytes; i++){
			DB(_ksprintf("%02x", tmp_buf[i]));
			if( i == read_bytes-1 ){
				break;
			}
			DB(_ksprintf(" "));
		}
		DB(_ksprintf("]\n"));
		}
		#endif
		inp_pos = tmp_buf;
		while(read_bytes--){
			c = *inp_pos++;
			/* if( is_euc==false c==ESC ){ is_esc=true;  } */
			/* if( is_euc==true  c==ESC ){ c=~c;   store } */
			/* if( is_euc==true  c==XXX ){ c=~c;   store } */
			/* if( is_euc==false c==XXX ){         store } */
			if( is_esc ==  false && c == ESC ){
				is_esc = true;
				#ifdef DEBUG_V
				DB(_ksprintf("XXXX: ESC\n"));
				#endif
			}else{
				if( is_esc_mask[c] && c != ESC ){
					return FATAL(_ksprintf("esc_quote(0x%02x)\n", c ));
				}
				if( is_esc == true ){
					c = ~c;
					is_esc = false;
				}
				#ifdef DEBUG_V
				DB(_ksprintf("%04x: %02x\n", adr++, c  ));
				#endif
				*out_pos++ = c;
				if( --max_buf < 0 ){
					return FATAL(_ksprintf("buffer over flow\n"));
				}
				check_sum += c;
				rest_len--;
				if( rest_len == 0 ){
					break;
				}
			}
		}
	}while( rest_len != 0 );

	os_free(tmp_buf);

	if( read_bytes != 0 ){
		return FATAL(_ksprintf("BUG: read_bytes=%d\n", read_bytes ));
	}

	#ifdef DEBUG
	if( is_debug ){ int i = 0; 
		DB(_ksprintf("===SIO== read["));
		if( 16 < len ){
			DB(_ksprintf("... "));
			i = len - 16;
		}
		for(;;){
			DB(_ksprintf("%02x", buf[i]));
			if( ++i == len ){
				break;
			}
			DB(_ksprintf(" "));
		}
		DB(_ksprintf("]+=%02x\n", (byte)check_sum ));
	}
	#endif
	return (long)check_sum;
}

/*
 *	sio_check_sum_esc_write( byte *buf, long max_buf, long len )
 *
 *          o write len bytes to sio
 *          o is_esc_mask[c] is changed to [ESC+~c] ( this byte not counted by len )
 *          o return check sum
 */
static long
sio_check_sum_esc_write(byte *buf, long len)
{
	long		i;
	byte		c;
	register byte	*p;
	byte		check_sum = 0;

	i = len;
	p = buf;
	while(i--){
		c = *p++;
		check_sum += c;
		if( is_esc_mask[c] ){
			if( os_sio_putchar(ESC) == NG ){
				return FATAL(_ksprintf("Can't write to sio\n"));
			}
			c = ~c;
		}
		if( os_sio_putchar(c) == NG ){
			return FATAL(_ksprintf("Can't write to sio\n"));
		}
	}

	#ifdef DEBUG
	if( is_debug ){
		DB(_ksprintf("===SIO== write["));
		for(i=0;;){
			DB(_ksprintf("%02x", buf[i]));
			if( ++i == len ){
				break;
			}
			DB(_ksprintf(" "));
		}
		DB(_ksprintf("] sum=%02x\n", check_sum));
	}
	#endif

	return (long)check_sum;
}

static ok_t
sio_send_ENQ_and_recv_ACK(void)
{
	byte	c;
	int	i;
	int	read_bytes;
	int	max_try = 5;

	for(i=0;i<max_try; i++){
		c = ENQ;
		DB(_ksprintf("putchar[%s]\n", os_name_of_char(c) ));
		if( os_sio_putchar(c) == NG ){
			return FATAL(_ksprintf("Can't send ENQ\n"));
		}
		read_bytes = os_sio_read_msec( &c , sizeof(byte), 300 );
		if( read_bytes == sizeof(byte) ){
			DB(_ksprintf("getchar[%s]\n", os_name_of_char(c) ));
			if( c == ACK ){
				return OK;
			}
			DB(_ksprintf("NO ACK !! read is 0x%02x, try again send ENQ\n", c ));
		}else{
			if( read_bytes == 0 ){
				DB(_ksprintf("send_command: no response try again send ENQ\n"));
			}else{
				return FATAL(_ksprintf("send_commnad: read_bytes=%d\n", read_bytes ));
			}
		}
	}
	DB(_ksprintf("try %d ENQ fail\n", max_try));
	return NG;
}

static ok_t
sio_print_rest_data(void)
{
	byte	buf[4000];
	long	len, i;

	os_msec_sleep(3000);
	len = os_sio_read_msec( buf, 4000, 100 );

	DB(_ksprintf("===SIO== rest data["));
	for(i=0;;){
		DB(_ksprintf("%02x", buf[i]));
		if( ++i == len ){
			break;
		}
		DB(_ksprintf(" "));
	}
	DB(_ksprintf("]\n"));
	return OK;
}

/*----------------------------------------------------------*/
static word
get_word(byte *buf)
{
	return (buf[1] << 8) | buf[0];
}

static word
get_long(byte *buf)
{
	return (buf[1]<<24) | (buf[0]<<16) | (buf[3]<<8) | (buf[2]);
}

static void
set_word(byte *buf, word data)
{
	buf[0] = data ;		/* byte0 */
	buf[1] = data >> 8;	/* byte1 */
}

static void
set_long(byte *buf, long data)
{
	buf[0] = data >> 16;	/* byte2 */
	buf[1] = data >> 24;	/* byte3 */
	buf[2] = data;		/* byte0 */
	buf[3] = data >> 8;	/* byte1 */
}


static ok_t
send_data_block(byte *inp_buf, word inp_len)
{
	#define MAX_DATA_TRANSFER_SIZE		2048
	#define MAX_DATA_TRANSFER_BUF_SIZE	(1+2+MAX_DATA_TRANSFER_SIZE*2+1+1)
						/*                         *2      means for ESC+XX */
	byte		buf[MAX_DATA_TRANSFER_BUF_SIZE];
	byte		check_sum = 0;
	long		d;

	/*
	 *	send STX
	 */
	DB(_ksprintf("send STX\n"));
	OK( os_sio_putchar(STX) );
	/*
	 *	send length
	 */
	set_word( buf, inp_len );
	DB(_ksprintf("send length=[%02x %02x]\n", buf[0], buf[1] ));
	OK(d = sio_check_sum_esc_write( buf, 2 ));
	check_sum += d;

	/*
	 *	send datablock with ESC quote conversion
	 */
	OK( d = sio_check_sum_esc_write( inp_buf, inp_len ) );
	check_sum += d;

	/*
	 *	send ETX
	 */
	DB(_ksprintf("send ETX\n"));
	OK( os_sio_putchar(ETX) );
	check_sum += ETX;

	/*
	 *	send check sum
	 */
	DB(_ksprintf("send check sum=0x%02x\n", check_sum));
	OK( sio_check_sum_esc_write( &check_sum, sizeof(byte) ) );
	return OK;
}

static long
recv_one_data_block(byte *answer_buf, long answer_max_buf_size, long *answer_received_byte_size)
{
	register byte	c;
	word		len;
	byte		len_buf[2];
	byte		calc_sum = 0;
	byte		recv_sum;
	long		etb_etx;
	long		d;

	/*
	 *	(1) recv STX
	 */
	DB(_ksprintf("wait STX\n"));
	if( (c=os_sio_getchar()) != STX ){
		DB(_ksprintf("must be STX in recv_one_data_block s(read is %s )\n", os_name_of_char(c) ));
		return -1;
	}
	DB(_ksprintf("DATA BLOCK reading ... first STX is OK\n"));
	/*
	 *	(2) recv length
	 */
	calc_sum = 0;
	OK( d = sio_check_sum_esc_read( len_buf, sizeof(word), sizeof(word) ) );
	calc_sum += d;
	len = ( (len_buf[1] << 8 ) | len_buf[0] );
	*answer_received_byte_size = (long)len;
	DB(_ksprintf("block_length=[0x%04x]\n", len));
	if( answer_max_buf_size < len ){
		FATAL(_ksprintf("recv_one_data_block: max_buf=%d get_len=%d\n", answer_max_buf_size, len ));
		return -1;
	}
	/*
	 *	(3) recv data block
	 */
	OK( d = sio_check_sum_esc_read( answer_buf, answer_max_buf_size, len ) );
	calc_sum += d;

	/*
	 *	(4) recv ETX
	 */
	OK( etb_etx = os_sio_getchar() );	/* no ESC mask */
	calc_sum += etb_etx; 
	if( !( etb_etx == ETB || etb_etx == ETX ) ){
		FATAL(_ksprintf("not ETB or ETX  (read is %s)\n", os_name_of_char(etb_etx) ));
		OK( sio_print_rest_data() );
		FATAL(_ksprintf("\n"));
		return -1;
	}
	DB(_ksprintf("OK ETX\n"));
	/*
	 *	(5) recv Check Sum
	 */
	OK( sio_check_sum_esc_read( &recv_sum, sizeof(byte), sizeof(byte) ) );
	if( recv_sum != calc_sum ){
		FATAL(_ksprintf("Check sum error (recv=0x%02x calc=0x%02x)\n", recv_sum, calc_sum ));
		return -1;
	}
	DB(_ksprintf("CHECK SUM OK. now get next block\n")); /* 5a is ok */
	if( etb_etx == ETB ){
		return ETB;
	}
	DB(_ksprintf("ETX OK\n"));
	return ETX;
}

	/*
	 *		Fig.1 Protcol Packet Sequence
    	 *
    
	         Transmitter                   Receiver
	              |                           |
	              |---------- ENQ ----------->|	(1s)
	              |                           |
	              |<--------- ACK ------------|	(1r)
	         +--->|                           |
	         |    |------ Data Block -------->|	(2s)
	 if NACK |    |                           |
	         |    |<------- ACK/NACK ---------|	(2r)
	         +----|                           |
	              |---------- EOT ----------->|	(3s)
	*/

static ok_t
send_command( byte *buf, word len )
{
	#define WAIT_MSEC	200
	#define WAIT_COUNT	10	/* 2sec ( 200msec*10 ) */
	long		c;

	/*
	 *	(1s) send ENQ until get ACK
	 */
	OK( sio_send_ENQ_and_recv_ACK() );
	DB(_ksprintf("OK ENQ->ACK. now send data block\n"));
	/*
	 *	(2s) send DATA BLOCK
	 *	(2r) recv ACK(or NACK)
	 */
	for(;;){
		OK(send_data_block( buf, len ));
		OK( c = os_sio_getchar() );
		if( c == ACK ){
			break;
		}
		if( c == NACK ){
			DB(_ksprintf("!!! datablock NACK\n"));
			continue;
		}else{
			DB(_ksprintf("must be ACK or NACK (read is %s)\n", os_name_of_char(c) ));
			return NG;
		}
	}
	DB(_ksprintf("OK datablock ACK\n"));
	/*
	 *	(3s) send EOT
	 */
	DB(_ksprintf("send EOT\n"));
	OK( os_sio_putchar(EOT) );

	return OK;
}

static void
disp_progress_func(long current_got_bytes)
{
	long	percent;

	if( disp_progress.func == (void *)0 ){
		return;
	}
	percent = (100 * current_got_bytes ) / disp_progress.all_bytes;
	if( percent > 100 ) percent = 100;

	if( percent == disp_progress.old_disp ){
		return;
	}
	disp_progress.old_disp = percent;
	disp_progress.func( percent );
}

static long	/* return value is received_byte_size */
recv_data_blocks( byte *recv_buf, long max_buf_size )
{
	long		c;
	int		ans;
	long		received_byte_size;
	long		all_received_byte_size = 0;

	DB(_ksprintf("recv_data_block_max_buf=%d\n", max_buf_size));
	/*
	 *	(1s) recv ENQ
	 *	(1r) send ACK
	 */
	OK( c = os_sio_getchar() );
	if ( c  != ENQ ){
		DB(_ksprintf("must be ENQ (read is %s)\n", os_name_of_char(c) ));
		FATAL(_ksprintf("next is 0x%02x\n", os_sio_getchar() ));
		return NG;
	}
	DB(_ksprintf("recv ENQ, next send ACK\n"));
	OK( os_sio_putchar(ACK) );
	/*
	 *	(2s) recv DATA BLOCK
	 *	(2r) send ACK/NACK
	 */
	for(;;){
		OK( ans = recv_one_data_block( recv_buf, max_buf_size, &received_byte_size ) );
		DB(_ksprintf("rec=%08x\n", received_byte_size ));
		all_received_byte_size += received_byte_size;
		recv_buf     += received_byte_size;
		max_buf_size -= received_byte_size;
		DB(_ksprintf("recv data block all ok, send ACK and wait EOT\n"));
		OK( os_sio_putchar(ACK) );
		/*
		 *	(3s) get EOT
		 */
		OK( c = os_sio_getchar() );
		if( c != EOT ){
			DB(_ksprintf("not EOT read is %s\n", os_name_of_char(c) ));
		}
		if( ans == ETX ){
			DB(_ksprintf("recv ETX, EOT all OK\n"));
			break;
		}
		if( ans == ETB ){
			DB(_ksprintf("recv ETB, EOT all OK, wait ENQ\n"));
			OK( c = os_sio_getchar() );
			if( c  != ENQ ){
				FATAL(_ksprintf("not EOT,  read is %s\n", os_name_of_char(c) ));
				return -1;
			}
			DB(_ksprintf("send ACK\n"));
			OK( os_sio_putchar(ACK) );
			disp_progress_func( all_received_byte_size );
		}else{
			FATAL(_ksprintf("no ETX or ETB\n"));
		}
	}
	os_msec_sleep(500);
	DB(_ksprintf("ans get=%08x\n", all_received_byte_size ));
	return 	all_received_byte_size;
}

#define OxXX	(-1)

static ok_t
comp_command( const short *recv_buf_const, byte *recv_buf, short len )
{
	short	i;

	for(i=0; i<len; i++){
		DB(_ksprintf("const=0x%02x rec=0x%02x\n", 0xff&recv_buf_const[i], recv_buf[i] ));
		if( recv_buf_const[i] == OxXX ){
			continue;
		}
		if( (byte)recv_buf_const[i] != recv_buf[i] ){
			return FATAL(_ksprintf("command compare fail\n"));
		}
	}
	return OK;
}

/*	4.3.1  dc_get_exif();			*/
/*	4.3.2  dc_get_thumbnail();		*/
/*	4.3.3  dc_get_jpeg();			*/
/*	4.3.4  dc_get_preview_image();		*/
/*	4.3.5  dc_get_image_info();		*/
/*	4.3.6  dc_erase_image();		*/
/*	4.3.7  dc_erase_all();			*/
/*	4.3.8  dc_format();			*/
/*	4.3.9  dc_protect_image();		*/
/*	4.4.1  dc_take_picture();		*/
/*	4.4.2  dc_cancel();			*/
/*	4.4.3  dc_get_io_capability();		*/
/*	4.4.4  dc_set_io();			*/
/*	4.4.5  dc_get_info();			*/
/*	4.4.6  dc_get_status();			*/
/*	4.4.7  dc_get_preference();		*/
/*	4.4.8  dc_set_preference();		*/
/*	4.4.9  dc_reset_preferences();		*/
/*	4.4.10 dc_get_date_and_time();		*/
/*	4.4.11 dc_set_date_and_time();		*/
/*	4.4.12 dc_get_operation_parameter();	*/
/*	4.4.13 dc_set_operation_parameter();	*/

/*======================================================*/
/* 4.3.1 dc_get_exif					*/
static long
qm100_dc_get_exif(int no, byte *ans_exif_buf, long max_buf_size, long *ans_exif_size)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x30,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      7 */	OxXX	/* High order byte of image ID               */
	};
	byte *send_cmd_buf_image_id = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x30,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word		retval;
	/*===================================================================*/
	/* comments:                                                         */
        /*                                                                   */

	set_word( send_cmd_buf_image_id,  no );
	DB(_ksprintf("============= send command QM100\n"));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );

	DB(_ksprintf("============= recv exif image start \n"));
	OK( *ans_exif_size = recv_data_blocks( ans_exif_buf, max_buf_size  ) );
	if( *ans_exif_size == -1 ){
		FATAL(_ksprintf("exif image recv fail\n"));
		return RETVAL_UNKNOWN_ERROR;
	}
	DB(_ksprintf("============= recv exif end\n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ));
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );
	retval=get_word(recv_buf_status);
	DB(_ksprintf("============= dc_get_exif retval=0x%02x\n", retval ));
	return retval;
}

static long
qm200_dc_get_exif(int no, byte *ans_exif_buf, long max_buf_size, long *ans_exif_size)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x30,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned long)*/
	/*      7 */	OxXX,	/* byte1 image ID                            */
	/*      8 */	OxXX,	/* byte2 image ID                            */
	/*      9 */	OxXX	/* High order byte of image ID               */
	};
	byte *send_cmd_buf_image_id = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x30,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word		retval;
	/*===================================================================*/
	/* comments:                                                         */
        /*                                                                   */

	set_long( send_cmd_buf_image_id,  no );
	DB(_ksprintf("============= send command QM200 max_buf=%d \n", max_buf_size ));
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));

	DB(_ksprintf("============= recv exif image start \n"));
	OK( *ans_exif_size = recv_data_blocks( ans_exif_buf, max_buf_size  ));
	if( *ans_exif_size == -1 ){
		FATAL(_ksprintf("exif image recv fail\n"));
	}
	DB(_ksprintf("============= recv exif end\n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));
	retval=get_word(recv_buf_status);
	DB(_ksprintf("============= dc_get_exif retval=0x%02x\n", retval ));
	return retval;
}
/*======================================================*/
/* 4.3.2 dc_get_thumbnail				*/
static long
qm100_dc_get_thumbnail(int no, byte *ans_jpeg_buf, long max_buf, long *ans_jpeg_size)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      7 */	OxXX	/* High order byte of image ID               */
	};
	byte *send_cmd_buf_image_id = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word		retval;
	/*===================================================================*/
	/* comments:                                                         */
        /*                                                                   */

	set_word( send_cmd_buf_image_id,  no );
	DB(_ksprintf("============= send command QM100\n"));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );

	DB(_ksprintf("============= recv thumbnail start \n"));
	OK( *ans_jpeg_size = recv_data_blocks( ans_jpeg_buf, max_buf ) );
	if( *ans_jpeg_size == -1 ){
		FATAL(_ksprintf("thumbnail recv fail\n"));
	}
	DB(_ksprintf("============= recv thumbnail end size=%d\n", *ans_jpeg_size ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );
	retval=get_word(recv_buf_status);
	DB(_ksprintf("============= dc_get_thumbnail retval=0x%02x\n", retval ));
	return retval;
}

static long
qm200_dc_get_thumbnail(int id, byte *ans_jpeg_buf, long max_buf, long *ans_jpeg_size)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned long)*/
	/*      7 */	OxXX,	/* byte1 image ID                            */
	/*      8 */	OxXX,	/* byte2 image ID                            */
	/*      9 */	OxXX	/* High order byte of image ID               */
	};
	byte *send_cmd_buf_image_id = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word		retval;
	/*===================================================================*/
	/* comments:                                                         */
        /*                                                                   */

	set_long( send_cmd_buf_image_id,  id );
	DB(_ksprintf("============= send command to QM200 image_id=0x%04x\n", id));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );

	DB(_ksprintf("============= recv thumbnail start \n"));
	OK( *ans_jpeg_size = recv_data_blocks( ans_jpeg_buf, max_buf ) );
	if( *ans_jpeg_size == -1 ){
		FATAL(_ksprintf("thumbnail recv fail\n"));
	}
	DB(_ksprintf("============= recv thumbnail end size=%d\n", *ans_jpeg_size ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );
	retval=get_word(recv_buf_status);
	DB(_ksprintf("============= dc_get_thumbnail retval=0x%02x\n", retval ));
	return retval;
}
/*======================================================*/
/* 4.3.3 dc_get_jpeg				*/
static long
qm100_dc_get_jpeg(int no, byte *ans_jpeg_buf, long max_buf, long *ans_jpeg_size)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x10,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      7 */	OxXX	/* High order byte of image ID               */
	};
	byte *send_cmd_buf_image_id  = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x10,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word		retval;
	/*===================================================================*/
	/* comments:                                                         */
        /*                                                                   */

	set_word( send_cmd_buf_image_id,  no );
	DB(_ksprintf("============= send command \n"));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );

	DB(_ksprintf("============= recv jpeg image start \n"));
	OK( *ans_jpeg_size = recv_data_blocks( ans_jpeg_buf, max_buf ) );
	if( *ans_jpeg_size == -1 ){
		FATAL(_ksprintf("jpeg image recv fail\n"));
	}
	DB(_ksprintf("============= recv jpeg end\n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));
	retval=get_word(recv_buf_status);
	DB(_ksprintf("============= dc_get_exif retval=0x%02x\n", retval ));
	return retval;
}
static long
qm200_dc_get_jpeg(int no, byte *ans_jpeg_buf, long max_buf, long *ans_jpeg_size)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x10,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* byte3 of image ID (unsigned long)         */
	/*      7 */	OxXX,	/* byte2 of image ID                         */
	/*      8 */	OxXX,	/* byte0 of image ID                         */
	/*      9 */	OxXX	/* byte1 of image ID                         */
	};
	byte *send_cmd_buf_image_id  = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x10,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word		retval;
	/*===================================================================*/
	/* comments:                                                         */
        /*                                                                   */

	set_long( send_cmd_buf_image_id,  no );
	DB(_ksprintf("============= send command \n"));
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));

	DB(_ksprintf("============= recv jpeg image start \n"));
	OK( *ans_jpeg_size = recv_data_blocks( ans_jpeg_buf, max_buf ));
	if( *ans_jpeg_size == -1 ){
		FATAL(_ksprintf("jpeg image recv fail\n"));
	}
	DB(_ksprintf("============= recv jpeg end\n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ));
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );
	retval=get_word(recv_buf_status);
	DB(_ksprintf("============= dc_get_exif retval=0x%02x\n", retval ));
	return retval;
}
/*======================================================*/
/* 4.3.4 dc_get_preview_image				*/
static long
dc_get_preview_image(void)
{
	/* future define */
	return 0;
}
/*======================================================*/
/* 4.3.5 dc_get_image_info				*/
static long
qm100_dc_get_image_info(int no, byte *info_buf, long info_max, dc_get_image_info_ans_t *ans )
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x20,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image number            */
	/*      7 */	OxXX	/* High order byte of image number           */
	};
	byte *send_cmd_buf_image_no  = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[10];
	const short recv_buf_const[10] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x20,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      5 */	OxXX,	/* High order byte of image ID               */
	/*      6 */	OxXX,	/* Low order byte of Exif image size (KByte) */
	/*      7 */	OxXX,	/* High order byte of Exif image size        */
	/*      8 */	OxXX,	/* 0x00: not protected   0x01: protected     */
	/*      9 */	0x00,	/* High order byte of protected status       */
	};
	byte *recv_buf_status           = recv_buf + 2;
	byte *recv_buf_image_id         = recv_buf + 4;
	byte *recv_buf_image_size       = recv_buf + 6;
	byte *recv_buf_image_is_protect = recv_buf + 8;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	set_word( send_cmd_buf_image_no,  no );
	DB(_ksprintf("============= send command no=%d\n", no ));
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));

	DB(_ksprintf("============= recv info data start \n"));
	OK( ans->info_size = recv_data_blocks( info_buf, info_max ));

	DB(_ksprintf("============= recv command \n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ));
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));

	ans->image_id         = (long)get_word(recv_buf_image_id);
	ans->image_kbytes     = (long)get_word(recv_buf_image_size);
	ans->image_is_protect = ( get_word(recv_buf_image_is_protect) == 0x01 );

	DB(_ksprintf("image_id          = 0x%04x\n", get_word(recv_buf_image_id)         ));
	DB(_ksprintf("image_size(Kbyte) = 0x%04x\n", get_word(recv_buf_image_size)       ));
	DB(_ksprintf("image_is_protect  = %d\n", get_word(recv_buf_image_is_protect) ));
	return get_word(recv_buf_status);
}

static void
os_bzero(char *buf, int size )
{
	while(size--){
		*buf++ ='\0';
	}
}

static long
qm200_dc_get_image_info(int no, byte *info_buf, long info_max, dc_get_image_info_ans_t *ans )
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	/*-------------------------------------------------------------------*/
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x20,   /* Low order byte of command identifier      */
	/*      1 */    0x88,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* byte2 of image number (unsigned long)     */
	/*      7 */	OxXX,	/* byte3 of image number (unsigned long)    */
	/*      8 */	OxXX,	/* byte0 of image number (unsigned long)     */
	/*      9 */	OxXX,	/* byte1 of image number (unsigned long)    */
	};
	byte *send_cmd_buf_image_no  = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	/*-------------------------------------------------------------------*/
	byte        recv_buf[12];
	const short recv_buf_const[12] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x20,	/* Low order byte of command identifier	     */
	/*      1 */	0x88,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* byte2 of image ID (unsigned long)         */
	/*      5 */	OxXX,	/* byte3 of image ID                         */
	/*      6 */	OxXX,	/* byte0 of image ID                         */
	/*      7 */	OxXX,	/* byte1 of image ID                         */
	/*      8 */	OxXX,	/* Low order byte of Exif image size (KByte) */
	/*      9 */	OxXX,	/* High order byte of Exif image size        */
	/*     10 */	OxXX,	/* 0x00: not protected   0x01: protected     */
	/*     11 */	0x00,	/* High order byte of protected status       */
	};
	byte *recv_buf_status           = recv_buf + 2;
	byte *recv_buf_image_id         = recv_buf + 4;
	byte *recv_buf_image_size       = recv_buf + 8;
	byte *recv_buf_image_is_protect = recv_buf + 10;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	set_long( send_cmd_buf_image_no,  no );
	DB(_ksprintf("============= send command no=%d\n", no));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );

	DB(_ksprintf("============= recv info data start \n"));
	OK( ans->info_size = recv_data_blocks( info_buf, info_max ) );
	if( ans->info_size == -1 ){
		FATAL(_ksprintf("get image info fail\n"));
	}

	DB(_ksprintf("============= recv command \n"));
	os_bzero( recv_buf, sizeof(recv_buf) );
	OK( recv_data_blocks(recv_buf, sizeof(recv_buf)) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf)) );

	ans->image_id         = (long)get_long(recv_buf_image_id);
	ans->image_kbytes     = (long)get_word(recv_buf_image_size);
	ans->image_is_protect = ( get_word(recv_buf_image_is_protect) == 0x01 );
	DB(_ksprintf("image_id          = 0x%04x\n", ans->image_id         ));
	DB(_ksprintf("image_size(Kbyte) = 0x%04x\n", ans->image_kbytes     ));
	DB(_ksprintf("image_is_protect  = %d\n",     ans->image_is_protect ));
	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.3.6 dc_erase_image				*/
static long
qm100_dc_erase_image(long image_id)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x80,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      7 */	OxXX	/* High order byte of image ID               */
	};
	byte *send_buf_image_id         = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x80,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word	retval;

	set_word( send_buf_image_id, image_id);

	/*
	 *
	 */
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf)) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	/*
	 *
	 */
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	retval=get_word(recv_buf_status);
	return retval;
}

static long
qm200_dc_erase_image(long image_id)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x80,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* byte2 of image ID (unsigned long)         */
	/*      7 */	OxXX,	/* byte3 of image ID                         */
	/*      6 */	OxXX,	/* byte0 of image ID                         */
	/*      7 */	OxXX	/* byte1 of image ID                         */
	};
	byte *send_buf_image_id         = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x80,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word	retval;

	set_long( send_buf_image_id, image_id);
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf)) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf)) );

	retval=get_word(recv_buf_status);
	return retval;
}
/*======================================================*/
/* 4.3.7 dc_erase_all				*/
static long
dc_erase_all(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[6] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x20,   /* Low order byte of command identifier      */
	/*      1 */    0x80,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[6];
	const short recv_buf_const[6] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x80,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Low order byte of number of images not erased  */
	/*      5 */	OxXX,	/* High order byte of number of images not erased */
	};
	byte *recv_buf_status           = recv_buf + 2;
	byte *recv_buf_no_erased        = recv_buf + 4;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word	retval;

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf)) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf)) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf)) );

	MSG(_ksprintf("no erased is %d\n", get_word(recv_buf_no_erased)));

	retval=get_word(recv_buf_status);
	return retval;
}
/*======================================================*/
/* 4.3.8 dc_format				*/
static long
dc_format(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[6] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x10,   /* Low order byte of command identifier      */
	/*      1 */    0x80,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x10,	/* Low order byte of command identifier	     */
	/*      1 */	0x80,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf)) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.3.9 dc_protect_image				*/
static long
qm100_dc_protect_image(long id, bool is_protect)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x30,   /* Low order byte of command identifier      */
	/*      1 */    0x80,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      7 */	OxXX,	/* High order byte of image ID               */
	/*      8 */	OxXX,	/* Protect(1) or unprotect(0) the specified image */
	/*      9 */	OxXX	/* High order byte of protected flag         */
	};
	byte *send_image_id      = send_cmd_buf + 6;
	byte *send_image_protect = send_cmd_buf + 8;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x30,	/* Low order byte of command identifier	     */
	/*      1 */	0x80,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/
	word	w_id      = (word)id;
	word	w_protect = is_protect ? (word)1 : (word)0;

	set_word( send_image_id,      w_id );
	set_word( send_image_protect, w_protect );
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
static long
qm200_dc_protect_image(long id, bool is_protect)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[12] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x30,   /* Low order byte of command identifier      */
	/*      1 */    0x80,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	/*      6 */	OxXX,	/* byte3 of image ID (unsigned long)         */
	/*      7 */	OxXX,	/* byte2 of image ID                         */
	/*      8 */	OxXX,	/* byte0 of image ID                         */
	/*      9 */	OxXX,	/* byte1 of image ID                         */
	/*     10 */	OxXX,	/* byte0 of protcet flag Protect=1 Unprotect=0  */
	/*     11 */	OxXX	/* byte1 of protect flag                     */
	};
	byte *send_image_id      = send_cmd_buf + 6;
	byte *send_image_protect = send_cmd_buf + 10;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x30,	/* Low order byte of command identifier	     */
	/*      1 */	0x80,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/
	long	w_id      = id;
	word	w_protect = is_protect ? (word)1 : (word)0;

	set_long( send_image_id,      w_id );
	set_word( send_image_protect, w_protect );
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.1 dc_take_picture				*/
static long
qm100_dc_take_picture(byte *info_buf, long info_max, long *info_size )
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x91,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[10];
	const short recv_buf_const[10] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x91,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Low order byte of image ID (unsigned int) */
	/*      5 */	OxXX,	/* High order byte of image ID               */
	/*      6 */	OxXX,	/* Low order byte of Exif image size        */
	/*      7 */	OxXX,	/* High order byte of Exif image size        */
	/*      8 */	OxXX,	/* 0x00: not protected   0x01: protected     */
	/*      9 */	OxXX,	/* High order byte of protected status       */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	// dcx_debug(1);
	// os_debug(1);
	DB(_ksprintf("take pict start\n"));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	os_sio_getchar_abort_sec(20);
	DB(_ksprintf("recv info block\n"));
	OK( *info_size = recv_data_blocks( info_buf, info_max ) );
	DB(_ksprintf("recv status block\n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );
	// dcx_debug(0);
	// os_debug(0);

	return get_word(recv_buf_status);
}
static long
qm200_dc_take_picture(byte *info_buf, long info_max, long *info_size )
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x91,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of device ID (unsigned int)*/
	/*      5 */	0x00,	/* High order byte of device ID              */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[12];
	const short recv_buf_const[12] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x91,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* byte2 of image ID (unsigned long)         */
	/*      5 */	OxXX,	/* byte3 of image ID                         */
	/*      6 */	OxXX,	/* byte0 of image ID                         */
	/*      7 */	OxXX,	/* byte1 of image ID                         */
	/*      8 */	OxXX,	/* Low order byte of Exif image size         */
	/*      9 */	OxXX,	/* High order byte of Exif image size        */
	/*     10 */	OxXX,	/* 0x00: not protected   0x01: protected     */
	/*     11 */	OxXX,	/* High order byte of protected status       */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );
	OK( *info_size = recv_data_blocks( info_buf, info_max ) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.2 dc_cancel				*/
static long
dc_cancel(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[4] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x9e,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[6];
	const short recv_buf_const[6] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x9e,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Low order byte of canceled command */
	/*      5 */	OxXX	/* High order byte of canceled command */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.3 dc_get_io_capability				*/
static long
dc_get_io_capability(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[4] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x00,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00    /* Reserved				     */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[8];
	const short recv_buf_const[8] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x00,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	0xFF,	/* Low order byte of supported bit rates     */
	/*      5 */	0x03,	/* High order byte of supported bit rates    */
	/*      6 */	0x1F,	/* Low order byte of flags	             */
	/*      7 */	0x00	/* High order byte of flags	             */
	};
	enum { RECV_RET_VAL_H = 3, RECV_RET_VAL_L = 2 };
	/*===================================================================*/
	/* Bit rate values: (recv_buf[5] << 8 | recv_buf[4] )	     */
	typedef enum {
		BIT_RATE_300BPS	= (1<<0),
		BIT_RATE_600BPS	= (1<<1),
		BIT_RATE_1200BPS	= (1<<2),
		BIT_RATE_2400BPS	= (1<<3),
		BIT_RATE_4800BPS	= (1<<4),
		BIT_RATE_9600BPS	= (1<<5),
		BIT_RATE_19200BPS	= (1<<6),
		BIT_RATE_38400BPS	= (1<<7),
		BIT_RATE_57600BPS	= (1<<8),
		BIT_RATE_115200BPS	= (1<<9),
		BIT_RATE_RESERVE_10	= (1<<10),
		BIT_RATE_RESERVE_11	= (1<<11),
		BIT_RATE_RESERVE_12	= (1<<12),
		BIT_RATE_RESERVE_13	= (1<<13),
		BIT_RATE_RESERVE_14	= (1<<14),
		BIT_RATE_RESERVE_15	= (1<<15)
	} recv_bit_rate_t;
	/*===================================================================*/
	/* Flag values:   (recv_buf[7] << 8 | recv_buf[6] )	     */
	typedef enum {
	BIT_FLAG_7_8_BITS   = (1<<0),  /* Data length	(0: 7 bits only)     */
				       /*               (1: 7 or 8 bits)     */
	BIT_FLAG_STOP_1_2   = (1<<1),  /* Stop bits	(0: 1 bit only )     */
				       /*               (1: 1 or 2 bits)     */
	BIT_FLAG_PAR_ON_OFF = (1<<2),  /* Parity	(0: no parity only)  */
				       /*               (1:parity on or off) */
	BIT_FLAG_EVEN_ODD   = (1<<3),  /* Parity Setting(0: even only)       */
				       /*               (1: even or odd)     */
	BIT_FLAG_HWFLOW_USE = (1<<4),  /* Hardware flow control - RTS/CTS    */
				       /*               (0: none)            */
				       /*               (1: used)            */
	BIT_FLAG_RESERVE_5  = (0<<5),  /* Reserved                           */
	BIT_FLAG_RESERVE_6  = (0<<6),  /* Reserved                           */
	BIT_FLAG_RESERVE_7  = (0<<7),  /* Reserved                           */
	BIT_FLAG_RESERVE_8  = (0<<8),  /* Reserved                           */
	BIT_FLAG_RESERVE_9  = (0<<9),  /* Reserved                           */
	BIT_FLAG_RESERVE_10 = (0<<10), /* Reserved                           */
	BIT_FLAG_RESERVE_11 = (0<<11), /* Reserved                           */
	BIT_FLAG_RESERVE_12 = (0<<12), /* Reserved                           */
	BIT_FLAG_RESERVE_13 = (0<<13), /* Reserved                           */
	BIT_FLAG_RESERVE_14 = (0<<14), /* Reserved                           */
	BIT_FLAG_RESERVE_15 = (0<<15), /* Reserved                           */
	} recv_flags_t;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word retval;
	/*===================================================================*/
	/* comments:                                                         */
	/*	Although the camera can support 7-bit communications, it is  */
        /*      required to be set to 8 bits for commands and responses to   */
        /*      be transmitted correctly.                                    */

	/*
	 *	
	 */

	DB(_ksprintf("============= send command \n"));
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));

	DB(_ksprintf("============= recv command \n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));

	retval=(recv_buf[RECV_RET_VAL_H]<<8)|recv_buf[RECV_RET_VAL_L];
	return retval;
}
/*======================================================*/
/* 4.4.4 dc_set_io				*/
static long
dc_set_io(int bps)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x80,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	OxXX,	/* Low order byte of bit rates selection     */
	/*      5 */	OxXX,	/* High order byte of bit rates selection    */
	/*      6 */	OxXX,	/* Low order byte of flags to be set         */
	/*      7 */	OxXX	/* High order byte of flags to be set        */
	};
	byte *send_cmd_buf_rate  = send_cmd_buf + 4;
	byte *send_cmd_buf_flags = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[8];
	const short recv_buf_const[8] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x80,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Low order byte of bit rates selection     */
	/*      5 */	OxXX,	/* High order byte of bit rates selection    */
	/*      6 */	OxXX,	/* Low order byte of flags to be set         */
	/*      7 */	OxXX	/* High order byte of flags to be set        */
	};
	byte *recv_buf_status = recv_buf + 2;
	/*===================================================================*/
	/* Bit rate values: (recv_buf[5] << 8 | recv_buf[4] )	     */
	typedef enum {
		BIT_RATE_300BPS		= (1<<0),
		BIT_RATE_600BPS		= (1<<1),
		BIT_RATE_1200BPS	= (1<<2),
		BIT_RATE_2400BPS	= (1<<3),
		BIT_RATE_4800BPS	= (1<<4),
		BIT_RATE_9600BPS	= (1<<5),
		BIT_RATE_19200BPS	= (1<<6),
		BIT_RATE_38400BPS	= (1<<7),
		BIT_RATE_57600BPS	= (1<<8),
		BIT_RATE_115200BPS	= (1<<9),
		BIT_RATE_RESERVE_10	= (1<<10),
		BIT_RATE_RESERVE_11	= (1<<11),
		BIT_RATE_RESERVE_12	= (1<<12),
		BIT_RATE_RESERVE_13	= (1<<13),
		BIT_RATE_RESERVE_14	= (1<<14),
		BIT_RATE_RESERVE_15	= (1<<15)
	} recv_bit_rate_t;
	/*===================================================================*/
	/* Flag values:   (recv_buf[7] << 8 | recv_buf[6] )	     */
	typedef enum {
	BIT_FLAG_8_BITS     = (1<<0),  /* Data length	(0: 7 bits )         */
				       /*               (1: 8 bits )         */
	BIT_FLAG_STOP2BITS  = (1<<1),  /* Stop bits	(0: 1 bits )         */
				       /*               (1: 2 bits )         */
	BIT_FLAG_PAR_ON     = (1<<2),  /* Parity	(0: no parityly)     */
				       /*               (1: parity on )      */
	BIT_FLAG_ODD        = (1<<3),  /* Parity Setting(0: even )           */
				       /*               (1: odd )            */
	BIT_FLAG_HWFLOW_USE = (1<<4),  /* Hardware flow control - RTS/CTS    */
				       /*               (0: none)            */
				       /*               (1: used)            */
	BIT_FLAG_RESERVE_5  = (0<<5),  /* Reserved                           */
	BIT_FLAG_RESERVE_6  = (0<<6),  /* Reserved                           */
	BIT_FLAG_RESERVE_7  = (0<<7),  /* Reserved                           */
	BIT_FLAG_RESERVE_8  = (0<<8),  /* Reserved                           */
	BIT_FLAG_RESERVE_9  = (0<<9),  /* Reserved                           */
	BIT_FLAG_RESERVE_10 = (0<<10), /* Reserved                           */
	BIT_FLAG_RESERVE_11 = (0<<11), /* Reserved                           */
	BIT_FLAG_RESERVE_12 = (0<<12), /* Reserved                           */
	BIT_FLAG_RESERVE_13 = (0<<13), /* Reserved                           */
	BIT_FLAG_RESERVE_14 = (0<<14), /* Reserved                           */
	BIT_FLAG_RESERVE_15 = (0<<15), /* Reserved                           */
	}	recv_flags_t;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	word	retval;
	/*===================================================================*/
	int	bit_rate_bps;
	/*===================================================================*/
	/* comments:                                                         */
	/*	Although the camera can support 7-bit communications, it is  */
        /*      required to be set to 8 bits for commands and responses to   */
        /*      be transmitted correctly.                                    */

	switch( bps ){
	case 300:	bit_rate_bps = BIT_RATE_300BPS;		break;
	case 600:	bit_rate_bps = BIT_RATE_600BPS;		break;
	case 1200:	bit_rate_bps = BIT_RATE_1200BPS;	break;
	case 2400:	bit_rate_bps = BIT_RATE_2400BPS;	break;
	case 4800:	bit_rate_bps = BIT_RATE_4800BPS;	break;
	case 9600:	bit_rate_bps = BIT_RATE_9600BPS;	break;
	case 19200:	bit_rate_bps = BIT_RATE_19200BPS;	break;
	case 38400:	bit_rate_bps = BIT_RATE_38400BPS;	break;
	case 57600:	bit_rate_bps = BIT_RATE_57600BPS;	break;
	case 115200:	bit_rate_bps = BIT_RATE_115200BPS;	break;
	default:	FATAL(_ksprintf("illegal bps=%d\n", bps ));
	}
	set_word( send_cmd_buf_rate,  bit_rate_bps );
	set_word( send_cmd_buf_flags, BIT_FLAG_8_BITS );

	DB(_ksprintf("============= send command setio\n"));
	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );

	DB(_ksprintf("============= recv command setio\n"));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));

	DB(_ksprintf("============= os_sio_set_bps(%d)\n", bps ));
	OK( os_sio_set_bps( bps ) );

	retval=get_word(recv_buf_status);
	return retval;
}
/*======================================================*/
/* 4.4.5 dc_get_info				*/
static long
dc_get_info(void)
{
	/* currentry not using */
	return 0;
}
/*======================================================*/
/* 4.4.6 dc_get_status				*/
static long
dc_get_status(dc_get_status_t *ans)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[6] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x20,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x00,	/* Low order byte of infomation type   (0x00)*/
	/*      5 */	0x00,	/* High order byte of information type (0x00)*/
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[34];
	const short recv_buf_const[34] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x20,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Low order byte of Self Test Resule */
	/*      5 */	OxXX,	/* High order byte of Self Test Resule */
	/*      6 */	OxXX,	/* PowerLevel */
	/*      7 */	OxXX,	/* PowerSource */
	/*      8 */	OxXX,	/* Card */
	/*      9 */	OxXX,	/* ViewStatus */
	/*     10 */	OxXX,	/* Low order byte of Card Size */
	/*     11 */	OxXX,	/* High order byte of Card Size */
	/*     12 */	OxXX,	/* Low order byte of Picture */
	/*     13 */	OxXX,	/* High order byte of Picture */
	/*     14 */	OxXX,	/* Low order byte of PicsLeft */
	/*     15 */	OxXX,	/* High order byte of PicsLeft */
	/*     16 */	OxXX,	/* DateAndTime - year */
	/*     17 */	OxXX,	/* DateAndTime - month */
	/*     18 */	OxXX,	/* DateAndTime - day */
	/*     19 */	OxXX,	/* DateAndTime - hour */
	/*     20 */	OxXX,	/* DateAndTime - minute */
	/*     21 */	OxXX,	/* DateAndTime - second */
	/*     22 */	OxXX,	/* IO Seting low order byte of bit rate */ 
	/*     23 */	OxXX,	/* IO Seting high order byte of bit rate */
	/*     24 */	OxXX,	/* IO Seting low order byte of flags */ 
	/*     25 */	OxXX,	/* IO Seting high order byte of flags */
	/*     26 */	OxXX,	/* Flash */
	/*     27 */	OxXX,	/* IQ Mode */
	/*     28 */	OxXX,	/* Self Timer / Macro */
	/*     29 */	OxXX,	/* ExpComp */
	/*     30 */	OxXX,	/* Low order byte of Total Pictures */
	/*     31 */	OxXX,	/* High order byte of Total Pictures */
	/*     32 */	OxXX,	/* Low order byte of Total Strobes */
	/*     33 */	OxXX	/* High order byte of Total Strobes */
	};
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));

	ans->status           = get_word(recv_buf+ 2);
	ans->card_size        = get_word(recv_buf+10);
	ans->picture_count    = get_word(recv_buf+12);
	ans->pics_left        = get_word(recv_buf+12);
	ans->year             = recv_buf[16];
	ans->month            = recv_buf[17];
	ans->day              = recv_buf[18];
	ans->hour             = recv_buf[19];
	ans->minute           = recv_buf[20];
	ans->second           = recv_buf[21];
	ans->flash            = recv_buf[26];
	ans->iq_mode          = recv_buf[27];
	ans->self_macro       = recv_buf[28];
	ans->exp_comp         = recv_buf[29];
	ans->total_pict       = get_word(recv_buf+30);
	ans->total_strobo     = get_word(recv_buf+32);

	return ans->status;
}
/*======================================================*/
/* 4.4.7 dc_get_preference				*/
static long
dc_get_preference(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[6] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x40,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of information type        */
	/*      5 */	0x00,	/* High order byte of information type       */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[8];
	const short recv_buf_const[8] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x40,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* ShutOff time                              */
	/*      5 */	OxXX,	/* Setf Timer Time                           */
	/*      6 */	OxXX,	/* Beep                                      */
	/*      7 */	OxXX	/* Slide Show Interval                       */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ));

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.8 dc_set_preference				*/
word
dc_set_preference(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0xc0,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	OxXX,	/* Low order byte of Field ID                */
	/*      5 */	OxXX,	/* High order byte of Field ID               */
	/*      6 */	OxXX,	/* Low order byte of Field Value             */
	/*      7 */	OxXX,	/* High order byte of Field Value            */
	};
	byte	*send_field_id  = send_cmd_buf + 4;
	byte	*send_field_val = send_cmd_buf + 6;
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0xc0,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/
	enum {
		FIELD_ID_FLASH		= 0xd000,
		FIELD_ID_IQ		= 0xc000,
	};
	enum {
		FIELD_VAL_FLASH_OFF	= 0,
		FIELD_VAL_IQ_SUPER_FINE	= 1,
	};
	word	field_id  = FIELD_ID_IQ;
	word	field_val = FIELD_VAL_IQ_SUPER_FINE;

	set_word( send_field_id,  field_id );
	set_word( send_field_val, field_val );

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ) );
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.9 dc_reset_preferences				*/
static long
dc_reset_preferences(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[8] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0xc1,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	0x02,	/* Low order byte of kind of preference of reset */
	/*      5 */	0x00,	/* High order byte of kind of preference of reset */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0xc1,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK( send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.10dc_get_date_and_time				*/
static long
dc_get_date_and_time(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[6] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0x30,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[10];
	const short recv_buf_const[10] = {
	/* Offset	Value	Description				     */
	/*      0 */	0x30,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	/*      4 */	OxXX,	/* Year 1996-1999(96-99) or 2000-2037(0-37)  */
	/*      5 */	OxXX,	/* Month (1-12)                              */
	/*      6 */	OxXX,	/* Day (1-31)                                */
	/*      7 */	OxXX,	/* Hours (0-23)                              */
	/*      8 */	OxXX,	/* Minutes (0-59)                            */
	/*      9 */	OxXX	/* Seconds (0-59)                            */
	};
	byte *recv_buf_status           = recv_buf + 2;
	byte	*d = recv_buf;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	MSG(_ksprintf("%02d/%02d/%02d %02d:%02d.%02d\n", d[4], d[5], d[6], d[7], d[8], d[9] ));
	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.11dc_set_date_and_time				*/
static long
dc_set_date_and_time(void)
{
	/*===================================================================*/
	/* The command has the following structure:                          */
	byte	send_cmd_buf[10] = {
	/* Offset 	Value	Description				     */
	/*      0 */    0xb0,   /* Low order byte of command identifier      */
	/*      1 */    0x90,   /* High order byte of command identifier     */
	/*      2 */    0x00,   /* Reserved				     */
	/*      3 */    0x00,   /* Reserved				     */
	/*      4 */	OxXX,	/* Year 1996-1999(96-99) or 2000-2037(0-37)  */
	/*      5 */	OxXX,	/* Month (1-12)                              */
	/*      6 */	OxXX,	/* Day (1-31)                                */
	/*      7 */	OxXX,	/* Hours (0-23)                              */
	/*      8 */	OxXX,	/* Minutes (0-59)                            */
	/*      9 */	OxXX	/* Seconds (0-59)                            */
	};
	/*===================================================================*/
	/* The corresponding response is as follows:                         */
	byte        recv_buf[4];
	const short recv_buf_const[4] = {
	/* Offset	Value	Description				     */
	/*      0 */	0xb0,	/* Low order byte of command identifier	     */
	/*      1 */	0x90,	/* High order byte of command identifier     */
	/*      2 */	OxXX,	/* Low order byte of return status	     */
	/*      3 */	OxXX,	/* High order byte of return status	     */
	};
	byte *recv_buf_status           = recv_buf + 2;
	/*===================================================================*/
	/* Return values: <common codes>                                     */
	/*===================================================================*/

	send_cmd_buf[4] = 99; /* Year 1996-1999(96-99) or 2000-2037(0-37)  */
	send_cmd_buf[5] =  1; /* Month (1-12)                              */
	send_cmd_buf[6] =  2; /* Day (1-31)                                */
	send_cmd_buf[7] =  3; /* Hours (0-23)                              */
	send_cmd_buf[8] =  4; /* Minutes (0-59)                            */
	send_cmd_buf[9] =  5; /* Seconds (0-59)                            */
	
	OK(send_command( send_cmd_buf, sizeof(send_cmd_buf) ));
	OK( recv_data_blocks( recv_buf, sizeof(recv_buf) ) );
	OK( comp_command( recv_buf_const, recv_buf, sizeof(recv_buf) ) );

	return get_word(recv_buf_status);
}
/*======================================================*/
/* 4.4.12dc_get_operation_parameter				*/
static long
dc_get_operation_parameter(void)
{
	/* currently not using */
	return 0;
}
/*======================================================*/
/* 4.4.13dc_set_operation_parameter				*/
static long
dc_set_operation_parameter(void)
{
	/* currently not using */
	return 0;
}

#define FUNC_NULL (void *)0
long (*dc_get_exif)(int no, byte *ans_exif_buf, long max_buf_size, long *ans_exif_size) = FUNC_NULL;
long (*dc_get_thumbnail)(int no, byte *ans_jpeg_buf, long max_buf, long *ans_jpeg_size) = FUNC_NULL;
long (*dc_get_jpeg)(int no, byte *ans_jpeg_buf, long max_buf, long *ans_jpeg_size) = FUNC_NULL;
long (*dc_get_image_info)(int no, byte *info_buf, long info_max, dc_get_image_info_ans_t *ans ) = FUNC_NULL;
long (*dc_erase_image)(long image_id) = FUNC_NULL;
long (*dc_protect_image)(long id, bool is_protect) = FUNC_NULL;
long (*dc_take_picture)(byte *info_buf, long info_max, long *info_size ) = FUNC_NULL;

static dcx_camera_model_t	camera_model = UNKNOWN_MODEL;

typedef struct {
	char		*name;
	os_sio_mode_t	mode;
} sio_t;

static sio_t	sio;

static ok_t
dcx_open(void)
{
	OK( os_sio_open(sio.name, sio.mode) );
	OK( dc_set_io(115200) );
	return OK;
}

static ok_t
dcx_close(void)
{
	OK( dc_set_io(9600) );
	OK( os_sio_close() );
	return OK;
}

static long
max_bytes_of_jpeg_image(void)
{
	switch( camera_model ){
	case QM100:	return 1300*1000;
	case QM200:	return 2000*1000;
	default:	FATAL(_ksprintf("illegual model"));
	}
	return -1;
}

ok_t
dcx_init(char *dev, dcx_camera_model_t model)
{
	sio.name = dev;

	switch( camera_model = model ){
	case QM100:
		sio.mode		= CRTS_CTS;
		dc_get_exif		= qm100_dc_get_exif;
		dc_get_thumbnail	= qm100_dc_get_thumbnail;
		dc_get_jpeg		= qm100_dc_get_jpeg;
		dc_get_image_info	= qm100_dc_get_image_info;
		dc_erase_image		= qm100_dc_erase_image;
		dc_protect_image	= qm100_dc_protect_image;
		dc_take_picture		= qm100_dc_take_picture;
		break;
	case QM200:
		sio.mode		= XON_XOFF;
		dc_get_exif		= qm200_dc_get_exif;
		dc_get_thumbnail	= qm200_dc_get_thumbnail;
		dc_get_jpeg		= qm200_dc_get_jpeg;
		dc_get_image_info	= qm200_dc_get_image_info;
		dc_erase_image		= qm200_dc_erase_image;
		dc_protect_image	= qm200_dc_protect_image;
		dc_take_picture		= qm200_dc_take_picture;
		break;
	default:
		FATAL(_ksprintf("unsupoort model\n"));
	}
	return OK;
}

long
dcx_get_number_of_pictures(void)
{
	dc_get_status_t ans;

	OK( dcx_open() );
	OK( dc_get_status(&ans));
	OK( dcx_close() );

	return (int)ans.picture_count;
}

ok_t
dcx_take_picture(void)
{
	long	ans_bytes;
	byte	info_buf[2000000];

	OK( dcx_open() );
	OK( dc_take_picture(info_buf, sizeof(info_buf), &ans_bytes) );
	OK( dcx_close() );
	return OK;
}

ok_t
dcx_alloc_and_get_thum(long no, dcx_image_t *ans_image)
{
	word		id;
	byte		*jpeg_buf;
	long		jpeg_size;
	dc_get_image_info_ans_t	ans_info;
	byte		info_buf[4096];
	long		max_size;

	OK( dcx_open() );

	OK( dc_get_image_info(no, info_buf, sizeof(info_buf), &ans_info) );
	id = ans_info.image_id;

	max_size = max_bytes_of_jpeg_image();
	jpeg_buf = os_malloc(max_size);

	OK( dc_get_thumbnail(id, jpeg_buf, max_size, &jpeg_size ) );

	if( (jpeg_buf =  os_realloc(jpeg_buf, jpeg_size)) == OS_NULL ){
		return FATAL(_ksprintf("realloc error\n"));
	}
	
	OK( dcx_close() );

	ans_image->alloc_buf  = jpeg_buf;
	ans_image->size       = jpeg_size;
	return OK;
}

ok_t
dcx_alloc_and_get_exif(long no, dcx_image_t *ans_image, void (*func)(long percent) )
{
	word		id;
	byte		*jpeg_buf;
	long		jpeg_size;
	dc_get_image_info_ans_t	ans_info;
	byte		info_buf[4096];
	long		max_size;

	OK( dcx_open() );

	OK( dc_get_image_info(no, info_buf, sizeof(info_buf), &ans_info) );
	id = ans_info.image_id;
	disp_progress.all_bytes = ans_info.image_kbytes * 1024;
	disp_progress.func      = func;
	disp_progress.old_disp  = -1;

	max_size = max_bytes_of_jpeg_image();
	if( (jpeg_buf = os_malloc(max_size)) == (void *)0 ){
		return FATAL(_ksprintf("malloc"));
	}

	OK( dc_get_exif(id, jpeg_buf, max_size, &jpeg_size ) );

	if( (jpeg_buf=os_realloc(jpeg_buf, jpeg_size )) == OS_NULL ){
		return FATAL(_ksprintf("realloc"));
	}
	ans_image->alloc_buf = jpeg_buf;
	ans_image->size      = jpeg_size;
	OK( dcx_close() );
	return OK;
}

ok_t
dcx_delete_picture(int no)
{
	dc_get_image_info_ans_t	ans;
	byte	info_buf[2000000];

	OK( dcx_open() );

	OK( dc_get_image_info(no, info_buf, sizeof(info_buf), &ans) );

	if( ! ans.image_is_protect ){
		dc_erase_image(ans.image_id);
	}

	OK( dcx_close() );
	return OK;
}

ok_t
dcx_format_cf_card(void)
{
	OK( dcx_open() );
	OK( dc_format() );
	OK( dcx_close() );
	return OK;
}

ok_t
dcx_get_summary(dcx_summary_t *summary)
{
	dc_get_status_t ans;

	OK( dcx_open() );
	OK( dc_get_status(&ans) );
	OK( dcx_close() );

	summary->total_pict     = (long)ans.total_pict     ;
	summary->picture_count  = (long)ans.picture_count  ;
	summary->hour           = (byte)ans.hour           ;
	summary->minute         = (byte)ans.minute         ;
	summary->second         = (byte)ans.second         ;
	summary->day            = (byte)ans.day            ;
	summary->month          = (byte)ans.month          ;
	summary->year           = (byte)ans.year           ;
	return OK;
}

ok_t for_lint(void)
{
	OK( dc_get_preview_image() );
	OK( dc_erase_all() );
	OK( dc_cancel() );
	OK( dc_get_io_capability() );
	OK( dc_get_info() );
	OK( dc_get_preference() );
	OK( dc_reset_preferences() );
	OK( dc_get_date_and_time() );
	OK( dc_set_date_and_time() );
	OK( dc_get_operation_parameter() );
	OK( dc_set_operation_parameter() );
	return OK;
}

#ifdef DCX_MAIN
void main(void)
{
	//dcx_debug(1);
	os_debug(1);
#ifdef __NetBSD__
	dcx_init("/dev/tty00", QM100 );
#else
	dcx_init("/dev/ttyS0", QM100 );
#endif
	dcx_open();
	dcx_close();
}
#endif
