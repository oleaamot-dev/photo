/* This is the I/O library to talk to the Ricoh RDC-300, or
 * RDC-300Z (or in Japan the DC-3).
 * Much of this code was taken from the dc3play program
 * by Jun-ichiro "itojun" Hagino (http://www.itojun.org/).
 * Much thanks is given to Itojun for his permision to use the
 * software, without which I would not have attempted this library.
 * That code was modified and added to, forming this library
 */

#include "config.h"
#include "crctab.c"

#if HAVE_TERMIOS_H
# include <termios.h>
#else
#  if HAVE_SYS_IOCTL_H
#   include <sys/ioctl.h>
#  endif
#  include <sgtty.h>
# endif
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>

/* debuging macros */
#define dprintf(x)						\
	{ if (ricoh_300_debugflag) {				\
		fprintf(stderr, __FILE__ ":%d: ", __LINE__);	\
		fprintf x;					\
	  }							\
	}
#define dpprintf(x)						\
    {								\
	int i;							\
	if (ricoh_300_debugflag) {				\
	    fprintf(stderr, __FILE__ ":%d: ", __LINE__);	\
	    fprintf x;					\
	    for (i = 0; i < len; i++)				\
		fprintf(stderr, "%02x ", buf[i]);		\
	    fprintf(stderr, "\n");				\
	}							\
    }
#define RICOH_300  1
#define RICOH_300Z 2
#define RICOH_4300 3

/* the following are left as globals so a debugger can set them */
int ricoh_300_debugflag = 0;	/* non-zero means print debug messages */
int ricoh_300_dumpflag = 0;	/* print serial port stream debug flag */
int ricoh_300_dumpmaxlen = 20;	/* max line length for dumpflag data */
int ricoh_300_verbose = 0;	/* flag to print download messages */
int ricoh_300_echobackrate = 5;	/* how often to print download messages */

static char ricoh_buf[1024 * 4];/* buffer for data from the camera */
static size_t ricoh_len;	/* relative pointer of last valid data
				 * in receive buffer */
static int fd0;
static int close_handler_set = 0;/* flag, close handler set for timer */
static int camera_opened = 0;	/* flag, camera is in connected state */
static int disconnecting = 0;	/* flag, camera is disconnecting */
static int camera_mode;		/* 0 = play, 1 = record */
static int camera_model_lib = RICOH_300Z; /* camera model use internaly */

static int quality = -1;	/* jpeg compresion level */
static int exposure = 255;	/* exposure compensation (default auto) */
static int white_balance = 0;	/* white balance (default auto) */

/* forward references to routines */
void dump_stream();
static void *close_handler();
int ricoh_wait();
int ricoh_get();
int ricoh_put();
int ricoh_getpacket();
int ricoh_sendcmd();
int ricoh_setspeed();
int ricoh_hello();
int ricoh_bye();
speed_t baudconv();
int setbaud();
static void *ricoh_exit();

/* establish connection with camera */
ricoh_300_open(serial_port, baudrate, camera_model)
char *serial_port;
int baudrate;
int *camera_model;
{
	static struct sigaction close_ricoh = {
	  (void (*)())close_handler, 0, SA_RESTART};
	struct timeval zero_time = {0, 0};
	struct itimerval it_zero = {zero_time, zero_time};

	if(!close_handler_set) {
	    sigaction(SIGALRM, &close_ricoh, (struct sigaction *)NULL);
	    if(atexit((void (*)())ricoh_exit) != 0)
		perror("error setting atexit function");
	    close_handler_set = 1;
	}
	if(camera_opened) {
	    /* turn off the close delay timer, and return */
	    setitimer(ITIMER_REAL, &it_zero, (struct itimerval*)NULL);
	    return 0;
	}
	/* wait for any previous disconnect to complete */
	while(disconnecting)sleep(10);
	if((fd0 = open(serial_port, O_RDWR|O_NDELAY)) == -1) {
	    fprintf(stderr,"For serial port %s, ", serial_port);
	    perror("Open error");
	    return -1;
	}
	/* camera turns on with an initial baud rate of 2400, so
	 * set the computer to 2400 baud */
	if (setbaud(fd0, 2400)) {
	    fprintf(stderr, "can't set baudrate\n");
	    close(fd0);
	    return -1;
	}

	/* initialize receive buffer pointer */
	ricoh_len = 0;

	/* connect to the camera */
	if(ricoh_hello(camera_model) == 1) {
	    dprintf((stderr, "hello: No response, trying %d baud\n", baudrate));
	    if (setbaud(fd0, baudrate)) {
	    	fprintf(stderr, "can't set baudrate\n");
	    	close(fd0);
	    	return -1;
	    }
	    if(ricoh_hello(camera_model) == 1) {
	    	close(fd0);
	    	return -1;
	    }
	}

	/* set the desired baudrate */
	if(ricoh_setspeed(baudrate) == 1) {
	    close(fd0);
	    return -1;
	}

	/* check what mode the camera is in */
	ricoh_300_getcam_mode(&camera_mode);
	/* set the desired picture taking quality to the quality
	 * the camera is set to initialy */
	if(quality < 0)
	    ricoh_300_getqual(&quality);
	camera_opened = 1;
}

/* disconnect from the camera */
ricoh_300_close()
{
    struct timeval zero_time = {0, 0};
    struct timeval noactivity_time = {12, 0};
    struct itimerval it_noactivity = {zero_time, noactivity_time};

    /* set a close delay timer, that allows a close when no activity
     * occurs for 12 seconds. */
    setitimer(ITIMER_REAL, &it_noactivity, (struct itimerval*)NULL);
    return 0;
}

static void*
close_handler(n)
int n;		/* signal number. should only be SIGALRM */
{
    struct timeval zero_time = {0, 0};
    struct timeval wait_disconnect = {5, 0};
    struct itimerval it_wait_disc = {zero_time, wait_disconnect};

    if(disconnecting)
	disconnecting = 0;
    else {
	ricoh_bye();
	close(fd0);
	setitimer(ITIMER_REAL, &it_wait_disc, (struct itimerval*)NULL);
	disconnecting = 1;
	camera_opened = 0;
    }
}

ricoh_300_getcam_mode(n)
int *n;
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	/* check what mode the camera is in */
	buf[0] = 0x12;
	ricoh_sendcmd('Q', buf, 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "get camera mode: Q 12 -> "));
	*n = buf[2];
	return err ? 1 : 0;
}

ricoh_300_getnpicts(n)
long *n;
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	ricoh_sendcmd('Q', "\0\001", 2, 0x00);
	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "getnpicts: Q 00 01 -> "));

	*n = buf[2];

	return err ? 1 : 0;
}

/* take a picture */
ricoh_300_takepicture()
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	/* set camera to record mode */
	ricoh_sendcmd('P', "\022\001", 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	camera_mode = 1;

	/* set jpeg compresion level */
	buf[0] = 8;
	buf[1] = quality & 0xff;
	buf[2] = 1;
	ricoh_sendcmd('P', buf, 3, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set quality: P 08 %02X 01 -> ", quality));

	/* set exposure compensation */
	buf[0] = 3;
	buf[1] = exposure & 0xff;
	ricoh_sendcmd('P', buf, 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set exposure: P 03 %02X -> ", exposure));

	/* Request ? */
	ricoh_sendcmd('Q', "\001", 1, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);

	/* Set ? */
	ricoh_sendcmd('P', "\001\001", 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);

	/* Take the picture */
	ricoh_sendcmd(0x60, "\001", 1, 0x00);
	do {
	    do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	    } while (ack);
	    dpprintf((stderr, "take picture: 60 01 -> "));
	} while ((buf[0] != 0 || buf[1] != 0) && err == 0);
	ricoh_sendcmd('Q', "\001", 1, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);

	return err ? 1 : 0;
}

/* get picture size */
/* Note: the 300 may not support this, only the 300Z */
ricoh_300_getsize(n, size)
int n;		/* picture number */
int *size;	/* size of picture in bytes */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	if(camera_model_lib == RICOH_300) {
	    *size = 200000;
	    return 0;
	}
	err = 0;

	buf[0] = 4;
	buf[1] = n & 0xff;
	buf[2] = 0;
	ricoh_sendcmd(0x95, buf, 3, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "getsize: 95 04 %02X 00 -> ", n));
	*size = buf[5] << 24 | buf[4] << 16 | buf[3] << 8 | buf[2];

	return err ? 1 : 0;
}

/* get picture date */
ricoh_300_getdate(n, date)
int n;		/* picture number */
unsigned char date[6];	/* date picture was taken */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	buf[0] = 3;
	buf[1] = n & 0xff;
	buf[2] = 0;
	ricoh_sendcmd(0x95, buf, 3, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "get date: 95 03 %02X 00 -> ", n));

	memmove(date, buf +3, 6);
	return err ? 1 : 0;
}

/* download picture to memory */
ricoh_300_getpict(n, image)
	int n;		/* picture number to download */
	char *image;	/* memory pointer to store image */
{
	u_char ch;
	u_char buf[1024 * 4];
	int len;
	int ack;
	u_char blk;

	int totlen;
	int flen;
	int err;

	err = 0;

	/* put camera in play mode, if not already */
	if(camera_mode != 0) {
	    buf[0] = 0x12;
	    buf[1] = 0x00;
	    ricoh_sendcmd('P', buf, 2, 0x00);
	    do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	    } while (ack);
	    dpprintf((stderr, "set play: 50 12 00 -> "));
	    camera_mode = 0;
	}

	/* send picture number to retreive to start download */
	buf[0] = n & 0xff;
	buf[1] = 0x00;
	ricoh_sendcmd(0xa0, buf, 2, 0x00);
	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "get picture: A0 %02X 00 -> ", n));

	totlen = (buf[16] << 16) | (buf[15] << 8) | buf[14];
	flen = 0;
	while (flen < totlen) {
		do {
		    err += ricoh_getpacket(&ch, &image[flen], &len, &ack, &blk);
		} while (ack);

		flen += len;

		if (ricoh_300_verbose && (blk % ricoh_300_echobackrate == 0)) {
			fprintf(stderr,
				"got block %3d: %d/%d \r",
					blk, flen, totlen);
		}

	}
	if (ricoh_300_verbose) {
		fprintf(stderr,
			"got block %3d: %d/%d ...done%s\n",
			blk, flen, totlen, err ? " with err" : "");
	}

	return err ? 1 : 0;
}

/* delete image from camera */
ricoh_300_deletepict(n)
	int n;		/* picture number to delete */
{
	u_char ch;
	u_char buf[1024 * 4];
	int len;
	int ack;
	u_char blk;

	int totlen;
	int flen;
	int err;

	err = 0;

	/* put camera in delete mode */
	ricoh_sendcmd(0x97, buf, 0, 0x00);
	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set delete mode: 97 -> "));

	/* find picture to delete */
	buf[0] = n & 0xff;
	buf[1] = 0x00;
	ricoh_sendcmd(0x93, buf, 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "pre delete picture: 93 %02X 00 -> ", n));

	/* send picture number to delete */
	buf[0] = n & 0xff;
	buf[1] = 0;
	ricoh_sendcmd(0x92, buf, 2, 0x00);
	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "delete picture: 92 %02X 00 -> ", n));
	return err ? 1 : 0;
}

/* get camera quality desired setting */
ricoh_300_getdesqual(n)
int *n;
{
    *n = quality;
    return 0;
}

/* get camera quality setting */
ricoh_300_getqual(n)
int *n;
{
    u_char ch;
    u_char buf[1024];
    int len;
    int ack;
    u_char blk;
    int err;

    err = 0;

    /* get camera jpeg compresion level */
    buf[0] = 8;
    ricoh_sendcmd('Q', buf, 1, 0x00);
    do {
	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
    } while (ack);
    dpprintf((stderr, "get quality: Q 08 -> "));
    *n = buf[2];
    return err ? 1 : 0;
}


/* set picture quality */
ricoh_300_setqual(n)
int n;		/* quality 1 = economy, 2 = normal, 4 = fine */
{
	quality = n;
	return 0;
}

/* get camera flash setting */
ricoh_300_getflash(n)
int *n;		/* flash mode 0 = auto, 1 = no flash, 2 = forced */
{
    u_char ch;
    u_char buf[1024];
    int len;
    int ack;
    u_char blk;
    int err;

    err = 0;

    /* get camera flash setting */
    buf[0] = 6;
    ricoh_sendcmd('Q', buf, 1, 0x00);
    do {
	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
    } while (ack);
    dpprintf((stderr, "get flash: Q 06 -> "));
    *n = buf[2];
    return err ? 1 : 0;
}

/* set camera flash mode */
ricoh_300_setflash(n)
int n;		/* flash mode 0 = auto, 1 = no flash, 2 = forced */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	/* Put the camera in the required record mode to set the flash */
	if(camera_mode != 1) {
	    buf[0] = 0x12;
	    buf[1] = 1;
	    ricoh_sendcmd('P', buf, 2, 0x00);
	    do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	    } while (ack);
	    dpprintf((stderr, "set record mode: P 12 01 -> "));
	    camera_mode = 1;
	}

	/* set white balance, as this effects the ability to set
	 * flash, if not automatic, the flash can not be set */
	buf[0] = 4;
	buf[1] = white_balance & 0xff;
	ricoh_sendcmd('P', buf, 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set white balance: P 04 %02X -> ", white_balance));
	usleep(100*1000);

	/* set the flash mode */
	if(white_balance == 0) { /* 0 = automatic */
	    buf[0] = 6;
	    buf[1] = n & 0xff;
	    ricoh_sendcmd('P', buf, 2, 0x00);
	    do {
	    	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	    } while (ack);
	    dpprintf((stderr, "set flash: P 06 %02X -> ", n));
	}

	return err ? 1 : 0;
}

/* get camera desired white balance setting */
ricoh_300_getdeswhite(n)
int *n;
{
    *n = white_balance;
    return 0;
}

/* get camera white balance setting */
ricoh_300_getwhite(n)
int *n;		/* white balance mode 0 = auto, 1 = outdoors,
		 * 2 = fluorescent, 3 = incandescent, 4 = b & w */
{
    u_char ch;
    u_char buf[1024];
    int len;
    int ack;
    u_char blk;
    int err;

    err = 0;

    /* get camera white balance setting */
    buf[0] = 4;
    ricoh_sendcmd('Q', buf, 1, 0x00);
    do {
	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
    } while (ack);
    dpprintf((stderr, "get white balance: Q 04 -> "));
    *n = buf[2];
    return err ? 1 : 0;
}

/* set camera white balance */
ricoh_300_setwhite(n)
int n;		/* white balance mode 0 = auto,
		 * 1 = outdoors, 2 = fluorescent, 3 = incandescent */
{
	white_balance = n;
	return 0;
}

/* get camera desired exposure compensation setting */
ricoh_300_getdesexposure(n)
int *n;
{
    *n = exposure;
    return 0;
}

/* get camera exposure compensation setting */
ricoh_300_getexposure(n)
int *n;		/* exposure compensation values range 1 - 9, 255 = automatic */
{
    u_char ch;
    u_char buf[1024];
    int len;
    int ack;
    u_char blk;
    int err;

    err = 0;

    /* get camera exposure compenstation setting */
    buf[0] = 3;
    ricoh_sendcmd('Q', buf, 1, 0x00);
    do {
	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
    } while (ack);
    dpprintf((stderr, "get exposure compensation: Q 03 -> "));
    *n = buf[2];
    return err ? 1 : 0;
}

/* set camera exposure compensation */
ricoh_300_setexposure(n)
int n;		/* exposure compensation values range 1 - 9, 255 = automatic */
{
	exposure = n;
	return 0;
}

/* get camera zoom lens position */
ricoh_300_getzoom(n)
int *n;		/* Zoom lens position, values range 0 - 8, 0 = no zoom */
{
    u_char ch;
    u_char buf[1024];
    int len;
    int ack;
    u_char blk;
    int err;

    err = 0;

    /* get camera zoom position */
    buf[0] = 5;
    ricoh_sendcmd('Q', buf, 1, 0x00);
    do {
	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
    } while (ack);
    dpprintf((stderr, "get zoom: Q 05 -> "));
    *n = buf[2];
    return err ? 1 : 0;
}

/* set camera zoom lens position */
ricoh_300_setzoom(n)
int n;		/* Zoom lens position, values range 0 - 8, 0 = no zoom */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	/* put camera in record mode, if not already,
	 * so one can preview the setting's effect */
	if(camera_mode != 1) {
	    buf[0] = 0x12;
	    buf[1] = 1;
	    ricoh_sendcmd('P', buf, 2, 0x00);
	    do {
	    	err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	    } while (ack);
	    dpprintf((stderr, "set record mode: P 12 01 -> "));
	    camera_mode = 1;
	}

	/* set zoom position */
	buf[0] = 5;
	buf[1] = n & 0xff;
	ricoh_sendcmd('P', buf, 2, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set zoom: P 05 %02X -> ", n));

	return err ? 1 : 0;
}

/* get camera ID (or copyright) */
ricoh_300_getID(id)
char *id;		/* string to put id */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	buf[0] = 0xf;
	ricoh_sendcmd('Q', buf, 1, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "get ID: Q 0F -> "));
	memmove(id, buf + 2, 20);
	id[20] = 0;

	return err ? 1 : 0;
}

/* set camera ID (or copyright) */
ricoh_300_setID(id)
char *id;		/* string containing id */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	if(strlen(id) == 0) return 0;
	err = 0;

	buf[0] = 0xf;
	sprintf(buf + 1,"%-20.20s",id);
	ricoh_sendcmd('P', buf, 21, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set ID: P 0F %-20.20s -> ", id));

	return err ? 1 : 0;
}

/* get camera date */
ricoh_300_getcamdate(date)
time_t *date;	/* camera real time date */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;
	struct tm time;

	err = 0;

	buf[0] = 0xa;
	ricoh_sendcmd('Q', buf, 1, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "get camera date: Q 0A -> "));

	/* since the camera is unlikely to last > 90 years,
	 * the following y2k conversion should be OK */
	time.tm_year = ((buf[3] & 0xf0) >> 4) * 10 + (buf[3] & 0xf);
	if(time.tm_year < 90) time.tm_year += 100;
	time.tm_mon = ((buf[4] & 0xf0) >> 4) * 10 + (buf[4] & 0xf) - 1;
	time.tm_mday = ((buf[5] & 0xf0) >> 4) * 10 + (buf[5] & 0xf);
	time.tm_hour = ((buf[6] & 0xf0) >> 4) * 10 + (buf[6] & 0xf);
	time.tm_min = ((buf[7] & 0xf0) >> 4) * 10 + (buf[7] & 0xf);
	time.tm_sec = ((buf[8] & 0xf0) >> 4) * 10 + (buf[8] & 0xf);
	time.tm_isdst = -1;
	*date = mktime(&time);

	return err ? 1 : 0;
}

/* set camera date */
ricoh_300_setcamdate(date)
time_t date;	/* camera real time date */
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;
	struct tm *time;
	int temp;

	err = 0;

	buf[0] = 0xa;
	time = localtime(&date);
#define HEX(x) (x / 10 << 4) + (x % 10)
	temp = time->tm_year/100 + 19;
	buf[1] = HEX(temp);
	temp = time->tm_year % 100;
	buf[2] = HEX(temp);
	buf[3] = HEX((time->tm_mon + 1));
	buf[4] = HEX(time->tm_mday);
	buf[5] = HEX(time->tm_hour);
	buf[6] = HEX(time->tm_min);
	buf[7] = HEX(time->tm_sec);
	dprintf((stderr, "set date = %02X %02X %02X %02X %02X %02X %02X\n",
	  buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));
	ricoh_sendcmd('P', buf, 8, 0x00);
	do {
	    err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "set camera date: P 0A date -> "));

	return err ? 1 : 0;
}

setbaud(fd, baud)
	int fd;
	int baud;
{
#if HAVE_TERMIOS_H
	/* termios */
	struct termios tio;

	if (tcgetattr(fd, &tio) < 0) {
		perror("tcgetattr");
		return 1;
	}
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;
	cfsetispeed(&tio, baudconv(baud));
	cfsetospeed(&tio, baudconv(baud));
	if (tcsetattr(fd, TCSANOW, &tio) < 0) {
		perror("tcsetattr");
		return 1;
	}
# else
	/* sgtty */
	struct sgttyb ttyb;

	if (ioctl(fd, TIOCGETP, &ttyb) < 0) {
		perror("ioctl(TIOCGETP)");
		return 1;
	}
	ttyb.sg_ispeed = baud;
	ttyb.sg_ospeed = baud;
	ttyb.sg_flags = 0;
	if (ioctl(fd, TIOCSETP, &ttyb) < 0) {
		perror("ioctl(TIOCSETP)");
		return 1;
	}
# endif
	dprintf((stderr, "baudrate set to %d\n", baud));
	return 0;
}

speed_t
baudconv(int baud)
{
#define	BAUDCASE(x)	case (x): { ret = B##x; break; }
	speed_t ret;

	ret = (speed_t) baud;
	switch (baud) {
	/* POSIX defined baudrates */
	BAUDCASE(0);	/*is it meaningful? */
	BAUDCASE(50);
	BAUDCASE(75);
	BAUDCASE(110);
	BAUDCASE(134);
	BAUDCASE(150);
	BAUDCASE(200);
	BAUDCASE(300);
	BAUDCASE(600);
	BAUDCASE(1200);
	BAUDCASE(1800);
	BAUDCASE(2400);
	BAUDCASE(4800);
	BAUDCASE(9600);
	BAUDCASE(19200);
	BAUDCASE(38400);

	/* non POSIX values */
#ifdef B7200
	BAUDCASE(7200);
#endif
#ifdef B14400
	BAUDCASE(14400);
#endif
#ifdef B28800
	BAUDCASE(28800);
#endif
#ifdef B57600
	BAUDCASE(57600);
#endif
#ifdef B115200
	BAUDCASE(115200);
#endif
#ifdef B230400
	BAUDCASE(230400);
#endif

	/* last resort */
	default:
		fprintf(stderr, "no defined value for baudrate %d found, "
			"use the value without conversion\n", baud);
	}

	return ret;
#undef BAUDCASE
}

/* makes initial connection to camera  */
ricoh_hello(camera_model)
int *camera_model;
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	ricoh_sendcmd(0x31, "\0\0\0", 3, 0x00);
	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack || blk);
	dpprintf((stderr, "hello: 31 00 00 00 -> "));
	if(err == 0) {
	    if(buf[0] == 0 && buf[1] == 0 &&
	       buf[2] == 3 && buf[3] == 0 && buf[4] == 0 && buf[5] == 0)
		*camera_model = RICOH_300;
	    else if(buf[0] == 0 && buf[1] == 0 &&
	      buf[2] == 3 && buf[3] == 1 && buf[4] == 0 && buf[5] == 0)
		*camera_model = RICOH_300Z;
	    else if(buf[0] == 0 && buf[1] == 0 &&
	      buf[2] == 4 && buf[3] == 0 && buf[4] == 0 && buf[5] == 0)
		*camera_model = RICOH_4300;
	    else
		*camera_model = 0;
	    camera_model_lib = *camera_model;
	}

	return err ? 1 : 0;
}

/* return camera control to the user */
ricoh_bye()
{
	u_char ch;
	u_char buf[1024];
	int len;
	int ack;
	u_char blk;
	int err;

	err = 0;

	ricoh_sendcmd(0x37, buf, 0, 0x00);
	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "bye: 37 00 -> "));

	return err ? 1 : 0;
}

/* change baud rate of camera, and computer */
ricoh_setspeed(baud)
	int baud;
{
	u_char ch;
	u_char buf[10];
	int len;
	int ack;
	u_char blk;
	int err;
	unsigned char value;

	err = 0;

#if HAVE_TERMIOS_H
	/*termios*/
	tcdrain(fd0);
#else
	/*sgtty*/
	ioctl(fd0, TCDRAIN, 0);
#endif

	switch (baud) {
	case -1:
	case 2400:
		value = 0x00;
		break;
	case 4800:
		value = 0x01;
		break;
	case 9600:
		value = 0x02;
		break;
	case 19200:
		value = 0x03;
		break;
	case 38400:
		value = 0x04;
		break;
	case 57600:
		value = 0x05;
		break;
	case 115200:
		value = 0x07;
		break;
	default:
		/* unsupported baud rate. */
		dprintf((stderr, "unsupported baudrate %d\n", baud));
		return 1;
	}

	/* set baudrate */
	buf[0] = value;
	ricoh_sendcmd('2', buf, 1, 0x00);

#if HAVE_TERMIOS_H
	/*termios*/
	tcdrain(fd0);
#else
	/*sgtty*/
	ioctl(fd0, TCDRAIN, 0);
#endif

	usleep(20*1000);

	do {
		err += ricoh_getpacket(&ch, buf, &len, &ack, &blk);
	} while (ack);
	dpprintf((stderr, "setspeed: 2 %02X -> ", value));

#if HAVE_TERMIOS_H
	/*termios*/
	tcdrain(fd0);
#else
	/*sgtty*/
	ioctl(fd0, TCDRAIN, 0);
#endif

	usleep(20*1000);

	if (baud == -1)
		err += setbaud(fd0, 2400);
	else
		err += setbaud(fd0, baud);

	usleep(1000*1000);

	return err ? 1 : 0;
}

ricoh_sendcmd(ch, buf, len, blkno)
	u_char ch;		/* class of command */
	u_char *buf;		/* specific command */
	int len;		/* length of specific command */
	u_char blkno;
{
	u_char tbuf[2];
	u_short crc;
	size_t i;
	int err;

	/* make sure camera is ready for next command packet
	 * by waiting 100 ms after any last data sent */
#if HAVE_TERMIOS_H
	/*termios*/
	tcdrain(fd0);
#else
	/*sgtty*/
	ioctl(fd0, TCDRAIN, 0);
#endif
	usleep(100*1000);

	/* generate crc sent at the end of the packet */
	crc = 0;
	crc = updcrc(ch & 0xff, crc);
	crc = updcrc(len & 0xff, crc);
	for (i = 0; i < len; i++) {
		crc = updcrc(buf[i] & 0xff, crc);
	}

	/* send the command type packet header */
	tbuf[0] = 0x10;
	tbuf[1] = 0x02;
	err = ricoh_put(tbuf, 2);

	/* send the command class, and length of command */
	tbuf[0] = ch;
	tbuf[1] = len & 0xff;
	err = ricoh_put(tbuf, 2);

	/* send the command */
	for (i = 0; i < len; i++) {
		/* 0x10 must be escaped */
		if (buf[i] == 0x10)
			err += ricoh_put(&buf[i], 1);
		err += ricoh_put(&buf[i], 1);
	}

	/* send the crc identifier */
	tbuf[0] = 0x10;
	tbuf[1] = 0x03;
	err = ricoh_put(tbuf, 2);

	/* send the crc */
	tbuf[0] = crc & 0x00ff;
	tbuf[1] = (crc & 0xff00) >> 8;
	err += ricoh_put(tbuf, 2);

	/* send length of data that was crc'd, and block number */
	tbuf[0] = len + 2;
	tbuf[1] = blkno;
	err += ricoh_put(tbuf, 2);

	return err ? 1 : 0;
}

/* Get a packet sent by the camera, and check the crc */
ricoh_getpacket(ch, buf, plen, ack, blk)
	u_char *ch;
	u_char *buf;
	int *plen;
	int *ack;
	u_char *blk;
{
	enum { PRESYNC, SYNC, PACKET, CRC, SKIPCRC } state;
	int gotpacket;
	int gotcrc;
	int gotack;
	int badcrc;

	u_char tbuf[2];
	u_char len;
	int i;

	/* initialize */
	state = PRESYNC;
	gotpacket = gotcrc = gotack = badcrc = 0;
	*plen = 0;
	*ack = 0;
	*blk = 0;

	while (1) {
		switch (state) {
		case PRESYNC:
			if (ricoh_get(tbuf, 1)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"no data received while "
						"getting sync\n"));
				goto fatal;
			}
			if (tbuf[0] == 0x10)
				state = SYNC;
			else {
				dprintf((stderr, "err in ricoh_getpacket: "
						"bad sync %02x: retrying\n",
						tbuf[0]));
			}
			break;

		case SYNC:
			if (ricoh_get(tbuf, 1)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"getting type\n"));
				goto fatal;
			}
			sync2:
			/* check packet type, and change state as required */
			switch(tbuf[0]) {
			case 0x06:		/* ACK */
			    *ack = 1;
			    goto done;
			case 0x15:		/* NACK */
			    goto fatal;
			case 0x02:		/* data */
			    state = PACKET;
			    break;
			case 0x03:		/* crc */
			case 0x17:		/* image crc */
			    if (gotpacket)
				state = CRC;
			    else
				state = SKIPCRC;
			    break;
			case 0x10:		/* should not occur */
			    dprintf((stderr, "err in ricoh_getpacket: "
					     "dup sync char: syncagain\n"));
				/* keep state as is */
			    break;
			default:
			    dprintf((stderr, "err in ricoh_getpacket: "
					     "bad type %02x: sync again\n",
						tbuf[0]));
				state = PRESYNC;
			}
			break;

		case PACKET:
			if (ricoh_get(ch, 1)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"getting cmdch\n"));
				goto fatal;
			}
			if (ricoh_get(&len, 1)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"getting len\n"));
				goto fatal;
			}
			i = 0;
			while (i < len) {
				if (ricoh_get(&buf[i], 1)) {
					dprintf((stderr,
						"err in ricoh_getpacket: "
						"getting body\n"));
					goto fatal;
				}
				if (buf[i] != 0x10) {
					i++;
					continue;
				}

				/*
				 * 0x10 case: just drop and get next byte.
				 * which should also be a 0x10, otherwise
				 * resync
				 */
				if (ricoh_get(&buf[i], 1)) {
					dprintf((stderr,
						"err in ricoh_getpacket: "
						"getting body(after 0x10)\n"));
					goto fatal;
				}
				if(buf[i] != 0x10) {
				    state = SYNC;
				    goto sync2;
				}
				i++;
			}
			gotpacket++;
			state = PRESYNC;
			break;

		case CRC:
			if (ricoh_get(tbuf, 2)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"getting crc\n"));
				goto fatal;
			}

		    {
			u_short crc;
			int i;

			crc = 0;
			crc = updcrc(*ch & 0xff, crc);
			crc = updcrc(len & 0xff, crc);
			for (i = 0; i < len; i++) {
				crc = updcrc(buf[i] & 0xff, crc);
			}

			if ((crc & 0xff) == tbuf[0]
			 && ((crc >> 8) & 0xff) == tbuf[1])
				badcrc = 0;
			else
				badcrc = 1;
			if(badcrc)dprintf((stderr, "crc: %04x<->%02x%02x: %s\n",
				crc, tbuf[1], tbuf[0],
				badcrc ? "bad" : "good"));
		    }

			if (ricoh_get(tbuf, 2)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"getting crc tail\n"));
				goto fatal;
			}
			if (tbuf[0] != len + 2) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"bad crc length (%d %d)\n",
						tbuf[0], len + 2));
				badcrc++;
			}
			*blk = tbuf[1];
			gotcrc++;
			state = PRESYNC;
			break;

		case SKIPCRC:
			if (ricoh_get(tbuf, 4)) {
				dprintf((stderr, "err in ricoh_getpacket: "
						"skipping crc\n"));
				goto fatal;
			}
			state = PRESYNC;
			break;
		}

		if (gotpacket && gotcrc) {
			if (!badcrc) {
				if(*ch != 0xa2)
				    usleep(70*1000);
				/* send packet acknowledge to camera */
				tbuf[0] = 0x10;
				tbuf[1] = 0x06;
				ricoh_put(tbuf, 2);
				/* Check packet if it is 00 04 ff then the
				 * camera is busy and hasn't really responded
				 * yet. We need to go back and get the packet
				 * again.
				 */
				if(len != 3 ||
				  buf[0] != 0x00 || buf[1] != 0x04 ||
				  buf[2] != 0xff) {
				    *plen = len;
				    goto done;
				}
			} else {
				/* bad crc, send NAK to camera */
				tbuf[0] = 0x10;
				tbuf[1] = 0x15;
				ricoh_put(tbuf, 2);
			}
			gotpacket = gotcrc = 0;
			state = PRESYNC;
			/* get packet all over again */
		}
	}

done:
	return 0;
fatal:
	return 1;
}
/* send bytes to the camera */
ricoh_put(buf, len)
	u_char *buf;
	int len;
{
	int wlen;
	int i;

	wlen = write(fd0, buf, len);
	if (wlen != len) {
	    dprintf((stderr, "failed in ricoh_put\n"));
	    return 1;
	}
	/* wait for data to reach the camera before returning */
#if HAVE_TERMIOS_H
	    /*termios*/
	    tcdrain(fd0);
#else
	    /*sgtty*/
	    ioctl(fd0, TCDRAIN, 0);
#endif

	/* print stream debug info if dumpflag is set */
	dump_stream('<', buf, len);
	return 0;
}

/* get data from the ricoh receive buffer */
ricoh_get(buf, len)
	u_char *buf;
	int len;
{
	switch (ricoh_wait(len)) {
	case 1:
		dprintf((stderr, "timed out in ricoh_get\n"));
		fprintf(stderr, "camera not ready.\n");
		return 1;
	case 0:
		break;
	}

	if (ricoh_len < len)
		abort();

	memcpy(buf, ricoh_buf, len);
	if (ricoh_len - len)
		memmove(ricoh_buf, ricoh_buf + len, ricoh_len - len);
	ricoh_len -= len;

	return 0;
}

/* If the ricoh receive buffer does not have enough data to satisfy the
 * length requested, this routine will get data from the camera
 */
ricoh_wait(rlen)
	int rlen;
{
	fd_set rdfd;
	int maxfd;
	int len;
	struct timeval t;

	while (ricoh_len < rlen) {
		/* obtain chars from the camera */
		FD_ZERO(&rdfd);
		FD_SET(fd0, &rdfd);
		maxfd = fd0;
		/* timeout: 4sec */
		t.tv_sec = 4;
		t.tv_usec = 0;
		switch (select(maxfd + 1, &rdfd, NULL, NULL, &t)) {
		case -1:
			if (errno == EINTR)
				break;
			perror("select");
			exit(1);
		case 0:
			dprintf((stderr, "read timeout.\n"));
			return 1;
		default:
			break;
		}

		if (FD_ISSET(fd0, &rdfd)) {
			len = read(fd0, ricoh_buf + ricoh_len,
				sizeof(ricoh_buf) - ricoh_len);
			ricoh_len += len;
			dump_stream('>', ricoh_buf + ricoh_len - len, len);
		} else {
			dprintf((stderr, "something wrong in ricoh_get\n"));
			return 1;
		}
	}

	return 0;
}
/*  when debug flag dumpflag is set, print the serial port stream */
void
dump_stream(dir, buf, len)
	int dir;
	char *buf;
	int len;
{
	size_t i;
	int truncate;

	if (!ricoh_300_dumpflag)
		return;

	truncate = 0;
	if (ricoh_300_dumpmaxlen < len) {
		len = ricoh_300_dumpmaxlen;
		truncate = 1;
	}

	dir &= 0xff;
	if (dir == '>')
		fprintf(stderr, "camera>cpu: ");
	else
		fprintf(stderr, "cpu>camera: ");

	for (i = 0; i < len; i++)
		fprintf(stderr, "%02x ", buf[i] & 0xff);
	if (truncate)
		fprintf(stderr, "...");
	fprintf(stderr, "\n");
}

/* on terminating the program, disconnect the camera, if still connected */
static void*
ricoh_exit()
{
    if(camera_opened)
	close_handler(SIGALRM);
}
