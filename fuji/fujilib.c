/*
 * $Id: fujilib.c,v 1.24 2001/01/18 23:26:25 ole Exp $
 
 Fuji Camera library for the gphoto project, 
 (C) 2000 Matthew G. Martin <matt.martin@ieee.org>
  This routine works for Fuji DS-7 and DX-5,7,10 and 
  MX-500,600,700,1200,1700,2700,2900,  Apple QuickTake 200,
  Samsung Kenox SSC-350N,Leica Digilux Zoom cameras and possibly others.

   Preview and take_picture fixes and preview conversion integrated 
   by Michael Smith <michael@csuite.ns.ca>.

   This driver was reworked from the "fujiplay" package:
      * A program to control Fujifilm digital cameras, like
      * the DS-7 and MX-700, and their clones.
      * Written by Thierry Bousch <bousch@topo.math.u-psud.fr>
      * and released in the public domain.

    Portions of this code were adapted from
    GDS7 v0.1 interactive digital image transfer software for DS-7 camera
    Copyright (C) 1998 Matthew G. Martin

    Some of which was derived from get_ds7 , a Perl Language library
    Copyright (C) 1997 Mamoru Ohno

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "exif.h"
#include "../src/gphoto.h"
#include "../src/util.h"
#include "../src/callbacks.h"

extern unsigned char *fuji_exif_convert(exifparser *exifdat);
extern GtkWidget *open_fuji_config_dialog();

#ifndef CLK_TCK
#include <sys/param.h>
#define CLK_TCK HZ
#endif

#if !defined(B57600) && defined(EXTA)
#define B57600 EXTA
#endif

#if !defined(B115200) && defined(EXTB)
#define B115200 EXTB
#endif

#define STX 0x2  /* Start of data */
#define ETX 0x3  /* End of data */
#define EOT 0x4  /* End of session */
#define ENQ 0x5  /* Enquiry */
#define ACK 0x6
#define ESC 0x10
#define ETB 0x17 /* End of transmission block */
#define NAK 0x15

struct pict_info {
	char *name;
	int number;
	int size;
	short ondisk;
	short transferred;
};

int fuji_initialized=0; 
int fuji_count;
int fuji_size;
int devfd = -1;
int maxnum;
int fuji_debug=0;

struct termios oldt, newt;
char has_cmd[256];
int pictures;
int interrupted = 0;
int pending_input = 0;
struct pict_info *pinfo = NULL;

#define FUJI_MAXBUF_DEFAULT 2000000

unsigned char answer[5000];
static char *fuji_buffer;
long fuji_maxbuf=FUJI_MAXBUF_DEFAULT;
int answer_len = 0;

static int get_raw_byte (void)
{
	static unsigned char buffer[128];
	static unsigned char *bufstart;
	int ret;

	while (!pending_input) {
		/* Refill the buffer */
		ret = read(devfd, buffer, 128);
		if (ret == 0)
			return -1;  /* timeout */
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;  /* error */
		}
		pending_input = ret;
		bufstart = buffer;
	}
	pending_input--;
	return *bufstart++;
}

int wait_for_input (int seconds)
{
	fd_set rfds;
	struct timeval tv;

	if (pending_input)
		return 1;
	if (!seconds)
		return 0;

	FD_ZERO(&rfds);
	FD_SET(devfd, &rfds);
	tv.tv_sec = seconds;
	tv.tv_usec = 0;

	return select(1+devfd, &rfds, NULL, NULL, &tv);
}

int get_byte (void)
{
	int c;

	c = get_raw_byte();
	if (c < 255)
		return c;
	c = get_raw_byte();
	if (c == 255)
		return c;	/* escaped '\377' */
	if (c != 0)
		fprintf(stderr, "get_byte: impossible escape sequence following 0xFF\n");
	/* Otherwise, it's a parity or framing error */
	get_raw_byte();
	return -1;
}

int put_bytes (int n, unsigned char* buff)
{
	int ret;

	while (n > 0) {
		ret = write(devfd, buff, n);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		n -= ret;
		buff += ret;
	}
	return 0;
}

int put_byte (int c)
{
	unsigned char buff[1];

	buff[0] = c;
	return put_bytes(1, buff);
}

/* See if camera is "there" */
int attention (void)
{
	int i;

	/* drain input */
	while (get_byte() >= 0)
		continue;
	for (i = 0; i < 3; i++) {
		put_byte(ENQ);
		if (get_byte() == ACK)
			return 0;
	}
	update_status("The camera does not respond.");
	return(-1);
}

void send_packet (int len, unsigned char *data, int last)
{
	unsigned char *p, *end, buff[3];
	int check;

	last = last ? ETX : ETB;
	check = last;
	end = data + len;
	for (p = data; p < end; p++)
		check ^= *p;

	/* Start of frame */
	buff[0] = ESC;
	buff[1] = STX;
	put_bytes(2, buff);

	/* Data */
	for (p = data; p < end; p++)
		if (*p == ESC) {
			/* Escape the last character */
			put_bytes(p-data+1, data);
			data = p+1;
			put_byte(ESC);
		}
	put_bytes(end-data, data);

	/* End of frame */
	buff[1] = last;
	buff[2] = check;
	put_bytes(3, buff);
}

int read_packet (void)
{
	unsigned char *p = answer;
	int c, check, incomplete;

	if (get_byte() != ESC || get_byte() != STX) {
bad_frame:
		/* drain input */
		while (get_byte() >= 0)
			continue;
		return -1;
	}
	check = 0;
	while(1) {
		if ((c = get_byte()) < 0)
			goto bad_frame;
		if (c == ESC) {
			if ((c = get_byte()) < 0)
				goto bad_frame;
			if (c == ETX || c == ETB ) {
				incomplete = (c == ETB);
				break;
			}
		}
		*p++ = c;
		check ^= c;
	}
	/* Append a sentry '\0' at the end of the buffer, for the convenience
	   of C programmers */
	*p = '\0';
	answer_len = p - answer;
	check ^= c;
	c = get_byte();
	if (c != check)
		return -1;
	if (answer[2] + (answer[3]<<8) != answer_len - 4)
		return -1;
	/* Return 0 for the last packet, 1 otherwise */
	return incomplete;
}


int cmd (int len, unsigned char *data)
{
	int i, c, timeout=50;
	fuji_count=0;

	if (fuji_debug) fprintf(stderr,"cmd %d\n",data[1]);

	/* Some commands require large timeouts */
	switch (data[1]) {
	  case 0x27:	/* Take picture */
	  case 0x34:	/* Recharge the flash */
	  case 0x64:	/* Take preview */
	    timeout = 12;
	    break;
	  case 0x19:	/* Erase a picture */
	    timeout = 1;
	    break;
	}
		
	for (i = 0; i < 3; i++) {
		send_packet(len, data, 1);
		wait_for_input(timeout);
		c = get_byte();

		if (c == ACK )
			goto rd_pkt;
		if (c == NAK) return (-1); /* NAK=Unrecognized command */

		/* Make sure camera is still alive */
		if(attention()) return(-1);

	}
	update_status( "Cannot issue command, aborting.");
	return(-1);
rd_pkt:
	wait_for_input(timeout);

	for (i = 0; i < 3; i++) {
		if (i) put_byte(NAK);
		c = read_packet();
		if (c < 0)
			continue;
		if (c && interrupted) {
			fprintf(stderr, "\nInterrupted!\n");
			return -1;
		}
		put_byte(ACK);
		if (fuji_buffer != NULL) {
		  if ((fuji_count+answer_len-4)<fuji_maxbuf){
		    memcpy(fuji_buffer+fuji_count,answer+4,answer_len-4);
		    fuji_count+=answer_len-4;
		  } else fprintf(stderr,"fujilib: buffer overflow\n");
		    
		  if (fuji_debug){
		    printf("Recd %d of %d\n",fuji_count,fuji_size);
		  };
		  if (fuji_size)
			  update_progress((1.0*fuji_count/fuji_size>1.0)?100:
					  100*fuji_count/fuji_size);

		};
		/* More packets ? */
		if (c != 0)
			goto rd_pkt;
		update_progress(0); /* Clean up the indicator */
		return 0;
	}		
	fprintf(stderr, "Cannot receive answer, aborting.\n");
	return -1;
}

int cmd0 (int c0, int c1)
{
	unsigned char b[4];

	b[0] = c0; b[1] = c1;
	b[2] = b[3] = 0;
	return cmd(4, b);
}

int cmd1 (int c0, int c1, int arg)
{
	unsigned char b[5];

	b[0] = c0; b[1] = c1;
	b[2] =  1; b[3] =  0;
	b[4] = arg;
	return cmd(5, b);
}

int cmd2 (int c0, int c1, int arg)
{
	unsigned char b[6];

	b[0] = c0; b[1] = c1;
	b[2] =  2; b[3] =  0;
	b[4] = arg; b[5] = arg>>8;
	return cmd(6, b);
}

char* dc_version_info (void)
{
	cmd0 (0, 0x09);
	return answer+4;
}

char* dc_camera_type (void)
{
	cmd0 (0, 0x29);
	return answer+4;
}

char* dc_camera_id (void)
{
	cmd0 (0, 0x80);
	return answer+4;
}

int dc_set_camera_id (const char *id)
{
	unsigned char b[14];
	int n = strlen(id);

	if (n > 10)
		n = 10;
	b[0] = 0;
	b[1] = 0x82;
	b[2] = n;
	b[3] = 0;
	memcpy(b+4, id, n);
	return cmd(n+4, b);
}

char* dc_get_date (void)
{
	char *fmtdate = answer+50;
	
	cmd0 (0, 0x84);
	strcpy(fmtdate, "YYYY/MM/DD HH:MM:SS");
	memcpy(fmtdate,    answer+4,   4);	/* year */
	memcpy(fmtdate+5,  answer+8,   2);	/* month */
	memcpy(fmtdate+8,  answer+10,  2);	/* day */
	memcpy(fmtdate+11, answer+12,  2);	/* hour */
	memcpy(fmtdate+14, answer+14,  2);	/* minutes */
	memcpy(fmtdate+17, answer+16,  2);	/* seconds */

	return fmtdate;
}

int dc_set_date (struct tm *pt)
{
	unsigned char b[50];

	sprintf(b+4, "%04d%02d%02d%02d%02d%02d", 1900 + pt->tm_year, pt->tm_mon+1, pt->tm_mday,
		pt->tm_hour, pt->tm_min, pt->tm_sec);
	b[0] = 0;
	b[1] = 0x86;
	b[2] = strlen(b+4);	/* should be 14 */
	b[3] = 0;
	return cmd(4+b[2], b);
}

int dc_get_flash_mode (void)
{
	cmd0 (0, 0x30);
	return answer[4];
}

int dc_set_flash_mode (int mode)
{
	cmd1 (0, 0x32, mode);
	return answer[4];
}

int dc_nb_pictures (void)
{
	if (cmd0 (0, 0x0b)) return(-1);
	return answer[4] + (answer[5]<<8);
}

char *dc_picture_name (int i)
{
	cmd2 (0, 0x0a, i);
	return answer+4;
}

int dc_picture_size (int i)
{
	cmd2 (0, 0x17, i);
	return answer[4] + (answer[5] << 8) + (answer[6] << 16) + (answer[7] << 24);
}

int charge_flash (void)
{
	cmd2 (0, 0x34, 200);
	return answer[4];
}

int take_picture (void)
{
	cmd0 (0, 0x27);
	return answer[4] + (answer[5] << 8) + (answer[6] << 16) + (answer[7] << 24);
}

int del_frame (int i)
{
	cmd2 (0, 0x19, i);
	return answer[4];
}

void get_command_list (void)
{
	int i;

	/* Set all commands as unsupported */
	memset(has_cmd, 0, 256);

	/* Attempt to fetch supported list */
	if (cmd0 (0, 0x4c)) return ;

	/* Note supported commands if list was returned */
	for (i = 4; i < answer_len; i++)
	  has_cmd[answer[i]] = 1;
}

int get_picture_info(int num,char *name){

          if(fuji_debug)  fprintf(stderr,"Getting name...");

	  fflush(stdout);
	  strncpy(name,dc_picture_name(num),100);

	  if (fuji_debug) fprintf(stderr,"%s\n",name);

	  /*
	   * To find the picture number, go to the first digit. According to
	   * recent Exif specs, n_off can be either 3 or 4.
	   */
	  if (has_cmd[0x17])   fuji_size=dc_picture_size(num);
	  else {
	    fuji_size=70000;  /* this is an overestimation for DS7 */
	    if (fuji_debug)
	    fprintf(stderr,"Image size not obtained, guessing %d\n",fuji_size);
	  };
	  return (fuji_size);
};

void get_picture_list (void)
{
	int i, n_off;
	char *name;
	struct stat st;

	pictures = dc_nb_pictures();
	maxnum = 100;
	free(pinfo);
	pinfo = calloc(pictures+1, sizeof(struct pict_info));
	for (i = 1; i <= pictures; i++) {
	        if (fuji_debug) fprintf(stderr,"Getting name...");
	        fflush(stdout);
	        name = strdup(dc_picture_name(i));
	        pinfo[i].name = name;

		if (fuji_debug) fprintf(stderr,"%s\n",name);

		/*
		 * To find the picture number, go to the first digit. According to
		 * recent Exif specs, n_off can be either 3 or 4.
		 */
		n_off = strcspn(name, "0123456789");
		if ((pinfo[i].number = atoi(name+n_off)) > maxnum)
			maxnum = pinfo[i].number;
		if (has_cmd[0x17])	pinfo[i].size = dc_picture_size(i);
		else pinfo[i].size=65000;
		pinfo[i].ondisk = !stat(name, &st);
	}
}

void list_pictures (void)
{
	int i;
	struct pict_info* pi;
	char ex;

	for (i = 1; i <= pictures; i++) {
		pi = &pinfo[i];
		ex = pi->ondisk ? '*' : ' ';
		fprintf(stderr,"%3d%c  %12s  %7d\n", i, ex,pi->name, pi->size);
	}
}

void close_connection (void)
{
	put_byte(EOT);
	tcdrain(devfd);
	usleep(50000);
}

void reset_serial (void)
{
        if (fuji_debug) fprintf(stderr,"Fujilib:Reset Serial\n");
	if (devfd >= 0) {
		close_connection();
		tcsetattr(devfd, TCSANOW, &oldt);
		close(devfd);
	}
	devfd = -1;
}

int init_serial (const char *devname)
{
        /* Attempt to open the serial device*/
        if (devfd<0) {
                if (fuji_debug) 
		        fprintf(stderr,"Fujilib:Opening Serial Device\n");

		devfd = open(devname, O_RDWR|O_NOCTTY);

		if (devfd < 0) { /* Open didn't work */
		        fprintf(stderr,"Fujilib:Cannot open device %s\n",\
				devname);
		        return -1;
		}
	}

	if (tcgetattr(devfd, &oldt) < 0) {
	        fprintf(stderr,"Fujilib:tcgetattr error %s\n",devname);
	        return -1;
	}
	newt = oldt;
	newt.c_iflag |= (PARMRK|INPCK);
	newt.c_iflag &= ~(BRKINT|IGNBRK|IGNPAR|ISTRIP|INLCR|IGNCR|ICRNL|IXON|IXOFF);
	newt.c_oflag &= ~(OPOST);
	newt.c_cflag |= (CLOCAL|CREAD|CS8|PARENB);
	newt.c_cflag &= ~(CSTOPB|HUPCL|PARODD);
	newt.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ICANON|ISIG|NOFLSH|TOSTOP);
	newt.c_cc[VMIN] = 0;
	newt.c_cc[VTIME] = 1;
	cfsetispeed(&newt, B9600);
	cfsetispeed(&newt, B9600);

	if (tcsetattr(devfd, TCSANOW, &newt) < 0) {
	        fprintf(stderr,"Fujilib:tcgetattr error %s\n",devname);
		return -1;
	}

       return(attention());
}

void set_max_speed (void)
{
	int speed;

	if (fuji_debug)
	        fprintf(stderr,"Setting max speed\n");

	/* This mess could be cleaned up... */
#ifdef B115200
	speed = B115200;
	cmd1(1, 7, 8);
	if (answer[4] == 0)
		goto change_speed;
#endif
#ifdef B57600
	speed = B57600;
	cmd1(1, 7, 7);
	if (answer[4] == 0)
		goto change_speed;
#endif
	speed = B38400;
	cmd1(1, 7, 6);
	if (answer[4] == 0)
		goto change_speed;

	/* Ugh -- we're stuck at 9600 bps */
	return;

change_speed:
	close_connection();
	cfsetispeed(&newt, speed);
	cfsetospeed(&newt, speed);
	tcsetattr(devfd, TCSANOW, &newt);
	attention();
	if (fuji_debug)
	        fprintf(stderr,"Speed Upgraded\n");

}

int download_picture(int n,int thumb,struct Image *im)
{
	char name[100];
	struct stat st;
	clock_t t1, t2;

	/* Predict picture size */
	if (!thumb) {
	        fuji_size=get_picture_info(n,name);
		if (fuji_debug) fprintf(stderr,"%3d   %12s  \n", n, name);
	}
	else fuji_size=10500;  /* Probly not same for all cams, better way ? */
	
	t1 = times(0);
	if (cmd2(0, (thumb?0:0x02), n)==-1) goto die_gracefully;

	if (fuji_debug) fprintf(stderr,
		"Download %s:%4d actual bytes vs expected %4d bytes\n", 
		       (thumb?"thumbnail":"picture"),fuji_count ,fuji_size);

	im->image_size=fuji_count+(thumb?128:0);/*leave room for thumb-decode*/
	im->image=malloc(im->image_size);
	if (im->image==NULL) goto die_gracefully;
	memcpy(im->image,fuji_buffer,im->image_size);

	t2 = times(0);

	if (fuji_debug){
	        fprintf(stderr,"%3d seconds, ", (int)(t2-t1) / CLK_TCK);
		fprintf(stderr,"%4d bytes/s\n", 
			fuji_count * CLK_TCK / (int)(t2-t1));
	};

	if (has_cmd[17]&&!thumb){
	if (!thumb&&(fuji_count != fuji_size)){
	    update_status("Short picture file--disk full or quota exceeded\n");
	    return(-1);
	  };
	};
	return(0);

 die_gracefully:	  /* when life gets rough */
	if (im->image!=NULL) free(im->image);
	im->image_size=0;
	im->image=NULL;
	return(-1);

}

int dc_free_memory (void)
{
	cmd0 (0, 0x1B);
	return answer[5] + (answer[6]<<8) + (answer[7]<<16) + (answer[8]<<24);
}


int delete_pic (const char *picname)
{
	int i, ret;

	for (i = 1; i <= pictures; i++)
	        if (!strcmp(pinfo[i].name, picname)) {
	                 if ((ret = del_frame(i)) == 0)
			          get_picture_list();
			 return ret;
		}
	return -1;
}


char* auto_rename (void)
{
	static char buffer[13];
	
	if (maxnum < 99999)
		maxnum++;

	sprintf(buffer, "DSC%05d.JPG", maxnum);
	return buffer;
}


int upload_pic (const char *picname)
{
	unsigned char buffer[516];
	const char *p;
	struct stat st;
	FILE *fd;
	int c, last, len, free_space;

	fd = fopen(picname, "r");
	if (fd == NULL) {
	  update_status("Cannot open file for upload");
		return 0;
	}
	if (fstat(fileno(fd), &st) < 0) {
		perror("fstat");
		return 0;
	}
	free_space = dc_free_memory();
	fprintf(stderr, "Uploading %s (size %d, available %d bytes)\n",
		picname, (int) st.st_size, free_space);
	if (st.st_size > free_space) {
		fprintf(stderr, "  not enough space\n");
		return 0;
	}
	if ((p = strrchr(picname, '/')) != NULL)
		picname = p+1;
	if (strlen(picname) != 12 || memcmp(picname,"DSC",3) || memcmp(picname+8,".JPG",4)) {
		picname = auto_rename();
		fprintf(stderr, "  file renamed %s\n", picname);
	}
	buffer[0] = 0;
	buffer[1] = 0x0F;
	buffer[2] = 12;
	buffer[3] = 0;
	memcpy(buffer+4, picname, 12);
	cmd(16, buffer);
	if (answer[4] != 0) {
		fprintf(stderr, "  rejected by the camera\n");
		return 0;
	}
	buffer[1] = 0x0E;
	while(1) {
		len = fread(buffer+4, 1, 512, fd);
		if (!len) break;
		buffer[2] = len;
		buffer[3] = (len>>8);
		last = 1;
		if ((c = getc(fd)) != EOF) {
			last = 0;
			ungetc(c, fd);
		}
		if (!last && interrupted) {
			fprintf(stderr, "Interrupted!\n");
			return 0;
		}
again:
		send_packet(4+len, buffer, last);
		wait_for_input(1);
		if (get_byte() == NAK)
			goto again;
	}
	fclose(fd);
	fprintf(stderr, "  looks ok\n");
	return 1;
}


int fuji_init(){
	
        if (! fuji_initialized)  /* this should never happen...*/
	        if (fuji_initialize()<1) return -1;

	if (fuji_debug) 
	        fprintf(stderr,"Fujilib:Initializing %s\n",serial_port);

	if (init_serial(serial_port)==-1) return(-1);

	set_max_speed();

	return(0);
};


/***********************************************************************
     gphoto lib calls
***********************************************************************/


int fuji_configure(){
  /* 
     DS7 can't be configured, looks like other Fuji cams can...
     expansion needed here.
  */
 open_fuji_config_dialog();

  return(1);
};

struct Image *fuji_get_picture (int picture_number,int thumbnail){
  GdkImlibImage *imlibimage;
  char tmpfname[1024];
  unsigned char *buffer;
  struct Image *newimage;
  FILE *fd;
  int i;
  exifparser exifdat;


  if (fuji_debug) fprintf(stderr,"fuji_get_picture called for #%d %s\n",
			  picture_number, thumbnail?"thumb":"photo");

  if (fuji_init()) return(0);

  /* setup gdk_image buffer */
  newimage=malloc(sizeof(struct Image));
  newimage->image=NULL;

  if (download_picture(picture_number,thumbnail,newimage)) {
    /* No Luck */
    free(newimage);
    return(0);
  };

  /* Point to the actual downloaded data */
  buffer=newimage->image;

  /* Work on the tags...*/
  exifdat.header=buffer;
  exifdat.data=buffer+12;

  if (exif_header_parse(&exifdat)>0){
    stat_exif(&exifdat);
  
    newimage->image_info=
      malloc(sizeof(char*)*(exifdat.ifdtags[thumbnail?1:0]*2+(thumbnail?6:0)));
    if (newimage->image_info==NULL) {
      fprintf(stderr,"BIG TROUBLE!!! Bad image memory allocation\n");
      return(0);
    };
    newimage->image_info_size=exifdat.ifdtags[thumbnail?1:0]*2;

    if (thumbnail) {/* SHOULD be done by tag id rather than assuming order*/
      togphotostr(&exifdat,0,0,newimage->image_info,newimage->image_info+1);
      togphotostr(&exifdat,0,1,newimage->image_info+2,newimage->image_info+3);
      togphotostr(&exifdat,0,2,newimage->image_info+4,newimage->image_info+5);
      newimage->image_info_size+=6;
    };
    
    /* Grab all of the tags */
    exif_add_all(&exifdat,thumbnail?1:0,newimage->image_info+(thumbnail?6:0));

    /* Spew a list of the tags, more than you will ever need */
    if (fuji_debug) {
            printf("====================EXIF TAGS================\n");
	    for(i=0;i<newimage->image_info_size/2;i++)
	            printf("%12s = %12s \n",newimage->image_info[i*2],
			   newimage->image_info[i*2+1]);
	    printf("=============================================\n");
    };

    /* Tiff info in thumbnail must be converted to be viewable */
    if (thumbnail) {
      newimage->image=fuji_exif_convert(&exifdat);
      
      if (newimage->image==NULL) {
	FILE *fp;
	
	fprintf(stderr,"Thumbnail conversion error, saving data to \
fuji-death-dump.dat\n\
\tPlease mail this file to gphoto-devel@gphoto.org along with a description \
of setup, camera model and any text output.\n");
	if ((fp=fopen("fuji-death-dump.dat","w"))!=NULL){
	  fwrite(buffer,1,newimage->image_size,fp);
	  fclose(fp);
	};
	free(newimage);
	return(0);
      };
      strcpy(newimage->image_type,"tif");
    }
    else  {
      newimage->image=buffer;
      strcpy(newimage->image_type,"jpg");
    };
  } else    { /* Wasn't recognizable exif data */
	FILE *fp;
	fprintf(stderr,"Saving problem data to fuji-death-dump.dat\n");
	if ((fp=fopen("fuji-death-dump.dat","w"))!=NULL){
	  fwrite(newimage->image,1,newimage->image_size,fp);
	  fclose(fp);
	};
  };

  reset_serial(); /* Wish this wasn't necessary...*/
  return(newimage);

};

int fuji_delete_image (int picture_number){
  int err_status;

  if (fuji_init())
    return (-1);

  if (!has_cmd[19]) {
    return(0);
  };

  err_status = del_frame(picture_number);
  reset_serial();

  /* Warning, does the index need to be re-counted after each delete?*/
  return(!err_status);
};

int fuji_number_of_pictures (){
  int numpix;

  if (fuji_debug) fprintf(stderr, "Fujilib:Number of Pictures Called\n");

  if (fuji_init()) return (-1);
  numpix=dc_nb_pictures();

  reset_serial(); /* Wish this wasn't necessary...*/
  return(numpix);

};

/* 
   One-time initialization: Read config, allocate buffers, etc
 */
int fuji_initialize () {
        char idstring[256];

	fuji_maxbuf=FUJI_MAXBUF_DEFAULT; /*Should be camera dependent*/
	/*	read_fuji_config(); */

	if (fuji_debug) fprintf(stderr,"Fujilib: Doing First Init\n");

	/* Allocate photo buffer */
	if (fuji_buffer) free(fuji_buffer);
	fuji_buffer=malloc(fuji_maxbuf);
	if (fuji_buffer==NULL) {
	        fprintf(stderr,"Fujilib:Memory allocation error\n");
		return(-1);
	} else if (fuji_debug) {
	        fprintf(stderr,"Fujilib:Allocated %ld bytes to main buffer\n",\
			fuji_maxbuf);
	};
	fuji_initialized=1;  
	atexit(reset_serial);

	/* Setup the serial port*/
	if (fuji_init()) {
	        fuji_initialized=0;
		return(-1);
	};
	
	/* could ID camera here and set the relevent variables... */
	get_command_list();
	/* Assume the user won't use 2 diff fuji cams in one session */
	strcpy(idstring,"Identified ");
	strncat(idstring,dc_version_info(),100);
	update_status(idstring);
	if (fuji_debug) fprintf (stderr,"Fujilib:%s\n",idstring);

	reset_serial();
	return 1;
}
 

/* Convert Fuji previews to 24-bit PPM format.
 * Based on yycc2ppm.c, part of the fujiplay package by Thierry Bousch
 * <bousch@club-internet.fr>.
 *
 * > One question - what format is the output from "fujiplay preview"?
 *
 * I don't think it's a "standard" format, rather a custom one. See the
 * yycc2ppm(1) utility for more information: there is a four-byte header
 * containing the image dimensions, followed by chunks of 4 bytes. Each
 * chunk encodes two adjacent pixels, in the format Y1 Y2 Cb Cr, where Y1
 * and Y2 are the luminance values of the two pixels, and Cb and Cr are the
 * averaged blue and red chrominance values for the pair.
 *
 * Usage: Pass the data, the length, and a pointer to an integer to receive
 *	  the size of the PPM buffer.
 * Returns the new PPM buffer.
 */
static char *
yycc2ppm(unsigned char *c, int len, int *ppmsize)
{
	int width, height, Y1, Y2, Cb, Cr, Roff, Goff, Boff;
	const char *headformat = "P6\n%d %d\n255\n";
	char header[24];
	unsigned char *ppm;
	register int i, j;

	if (len < 8) {
		fprintf(stderr, "yycc2ppm: preview is %d bytes, need >= 8.\n",
			len);
		return NULL;
	}
	width  = 256 * c[1] + c[0];	/* little-endian */
	height = 256 * c[3] + c[2];

	if (fuji_debug)
		fprintf(stderr, "yycc2ppm: preview %d x %d\n", width, height);

	snprintf(header, sizeof(header), headformat, width, height);
	header[sizeof(header)-1] = '\0';

	/* For every four bytes of YYCC after the header, generate 6 of RGB */
	*ppmsize = strlen(header) + 6 * (len - 4) / 4;

	ppm = malloc(*ppmsize);
	if (!ppm) {
		fprintf(stderr,"yycc2ppm: couldn't alloc %d bytes\n",*ppmsize);
		return NULL;
	}
	strcpy(ppm, header);

	for (i=4, j=strlen(header); i<len; i+=4) {
		Y1 = c[i];
		Y2 = c[i+1];
		Cb = c[i+2] - 128;
		Cr = c[i+3] - 128;

		Roff = (359*Cr + 128) >> 8;
		Goff = (-88*Cb - 183*Cr + 128) >> 8;
		Boff = (454*Cb + 128) >> 8;

		#define CLIP(x) (((x)<0) ? 0 : (((x)>255) ? 255 : (x)))
		ppm[j++] = CLIP(Y1+Roff);
		ppm[j++] = CLIP(Y1+Goff);
		ppm[j++] = CLIP(Y1+Boff);
		ppm[j++] = CLIP(Y2+Roff);
		ppm[j++] = CLIP(Y2+Goff);
		ppm[j++] = CLIP(Y2+Boff);
	}

	if (i != len)
		fprintf(stderr, "yycc2ppm: warn: preview had %d extra bytes.\n",
			i - len);
	return ppm;
}


struct Image *fuji_get_preview (){
  FILE *fd;

  struct Image *newimage;

  if (fuji_init()) return NULL;

  newimage=malloc(sizeof( struct Image));
  if (!newimage) {
    fprintf(stderr, "fuji_get_preview: failed to malloc newimage.\n");
    return NULL;
  }
  memset(newimage, 0, sizeof(struct Image));

  if (!has_cmd[0x62] || !has_cmd[0x64]) {
    update_status("Cannot preview (unsupported command)");
    return ( 0 );
  }

  cmd0(0, 0x64);
  cmd0(0, 0x62);
  newimage->image = yycc2ppm(fuji_buffer, fuji_count, &(newimage->image_size));

  reset_serial();

  if (!newimage->image) {
    free(newimage);
    return NULL;
  }

  strcpy(newimage->image_type, "ppm");
  return(newimage);
};

int fuji_take_picture (){
   int pic;

   if (fuji_init()) return (-1);

   if (!has_cmd[0x27]) {
    update_status("Cannot take picture (unsupported command)\n");
    return(0);
  }

  pic=take_picture();

  reset_serial();

  return pic;
};

char *fuji_summary() {
	return ("Not Yet Supported.");
}

char *fuji_description() {
	return(
"Generic Fuji
Matthew G. Martin
Based on fujiplay by
Thierry Bousch<bousch@topo.math.u-psud.fr>

Known to work with Fuji DS-7 and DX-5,7,10 and MX-500,600,700,2700
, Apple QuickTake 200,Samsung Kenox SSC-350N cameras, 
but may support other Fuji cams as well.");
}

struct _Camera fuji = {fuji_initialize,
		       fuji_get_picture,
		       fuji_get_preview,
		       fuji_delete_image,
		       fuji_take_picture,
		       fuji_number_of_pictures,
		       fuji_configure,
		       fuji_summary,
		       fuji_description,
		       NULL
};
