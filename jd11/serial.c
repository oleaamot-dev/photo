#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "../src/gphoto.h"
#include "serial.h"

static float	f1,f2,f3;

static int _send_cmd(int fd,unsigned short cmd) {
	unsigned char buf[2];
	buf[0] = cmd>>8;
	buf[1] = cmd&0xff;
	if (2!=write(fd,buf,2))
	    return 0;
	return 1;
}

static void _read_cmd(int fd,unsigned short *xcmd) {
	unsigned char buf[2];
	int	i=0;
	*xcmd = 0x4242;
	do {
		if (2==read(fd,buf,2)) {
			if (buf[0]==0xff)
				break;
			continue;
		}
		/*usleep(100);*/ i=10;
	} while (i++<10);
	*xcmd = (buf[0]<<8)|buf[1];
}

static void _dump_buf(unsigned char *buf,int size) {
	int i;

	return;

	fprintf(stderr,"[");
	for (i=0;i<size;i++)
		fprintf(stderr,"%02x ",buf[i]);
	fprintf(stderr,"]\n");
}

int ping(int fd) {
	unsigned short xcmd;
	int i=0;

	if (!_send_cmd(fd,0xff08))
	    return 0;
	while (i<20) {
	    xcmd = 0x4242;
	    _read_cmd(fd,&xcmd);
	    if (xcmd==0xfff1)
		break;
	    /*fprintf(stderr,"ping returned %x\n",xcmd);*/
	    i++;
	}
	return (xcmd==0xfff1);
}

int float_query(int fd) {
	char	buf[20];
	int	curread=0,ret;

	if (!_send_cmd(fd,0xffa7))
	    return 0;
	usleep(500);
	while(curread<10) {
	    ret=read(fd,buf+curread,20-curread);
	    if (ret==-1)
		return 0;
	    curread+=ret;
	}
	f1 = buf[1]+buf[2]*0.1+buf[3]*0.01;
	f2 = buf[4]+buf[5]*0.1+buf[6]*0.01;
	f3 = buf[7]+buf[8]*0.1+buf[9]*0.01;
	if(curread<10) {
	    /*fprintf(stderr,"%d returned bytes on float query.\n",curread);*/
	    assert(curread>=10);
	}
	/*fprintf(stderr,"FLOAT_QUERY: %f %f %f.\n",f1,f2,f3);*/
	return 1;
}

int cmd_a4(int fd) {	/* select index */
	unsigned short xcmd;

	if (!_send_cmd(fd,0xffa4))
	    return 0;
	xcmd = 0x4242;
	_read_cmd(fd,&xcmd);
	return (xcmd==0xff01); /* this seems to be the OK value or Go Ahead */
}

int cmd_a1(int fd,int nr) {	/* select image <nr> */
	unsigned short xcmd;
	int t=0;

	if (!_send_cmd(fd,0xffa1))
	    return 0;
	if (!_send_cmd(fd,0xff00|nr))
	    return 0;
	while (t<10) {
	    xcmd = 0x4242;
	    _read_cmd(fd,&xcmd);
	    if ((xcmd&0xff00) != 0xff00)
		continue;
	    break;
	}
	if (t==10)
	    return 0;
	/*fprintf(stderr,"A1 - initiated, xcmd=%x\n",xcmd);*/
	return 1;
}
int cmd_75(int fd) {
	unsigned short xcmd;

	if (!_send_cmd(fd,0xff75))
	    return 0;
	xcmd = 0x4242;
	_read_cmd(fd,&xcmd);
	/*fprintf(stderr,"75: done, xcmd=%x\n",xcmd);*/
	return 1;
}

int cmd_72(int fd) {
	unsigned short xcmd;

	if (!_send_cmd(fd,0xff72))
	    return 0;
	xcmd = 0x4242;
	_read_cmd(fd,&xcmd);
	assert(xcmd==0xff01); /* this seems to be the OK value or Go Ahead */
	fprintf(stderr,"72: done.\n");
	return 1;
}

int cmd_73(int fd) {
	unsigned short xcmd;

	if (!_send_cmd(fd,0xff73))
	    return 0;
	xcmd = 0x4242;
	_read_cmd(fd,&xcmd);
	fprintf(stderr,"73: xcmd = %x.\n",xcmd);
	return 1;
}

/* flattert den Abzug ... selftest? */
int cmd_79(int fd) {
	unsigned short xcmd;

	if (!_send_cmd(fd,0xff79))
	    return 0;
	xcmd = 0x4242;
	_read_cmd(fd,&xcmd);
	fprintf(stderr,"79: done, xcmd =%x\n",xcmd);
	return 1;
}

int imgsize(int fd) {
	char	*s,buf[20];
	int	ret;
	int	i=0,curread=0;

	if (!_send_cmd(fd,0xfff0))
	    return 0;
	do {
		ret=read(fd,&buf[curread],10-curread);
		if (ret>0)
			curread+=ret;
		if (ret<0)
			break;
		usleep(100);
	} while ((i++<20) && (curread<10));
	_dump_buf(buf,ret);
	s=buf;
	for (i=0;i<curread-2;i++) 
	    	if (*s==0x66) break;
	ret=strtol(&buf[i+2],NULL,16);
	/*fprintf(stderr,"IMGSIZE: %d\n",ret);*/
	return ret;
}

int getpacket(int fd,unsigned char *buf,int expsize) {
	if (expsize==200) expsize++;
	while (1) {
		int i=0,ret,curread=0,csum=0;

		do {
			ret=read(fd,buf+curread,expsize-curread);
			if (ret>0) {
				curread+=ret;
				i=0;
				continue;
			}
			if (ret==-1)
			    break;
			usleep(100);
		} while ((i++<2) && (curread<expsize));
		if (!curread)
			return 0;
		/*printf("curread is %d\n",curread);*/
		/*printf("PACKET:");_dump_buf(buf,curread);*/
		for (i=0;i<curread-1;i++)
			csum+=buf[i];
		if (buf[curread-1]==(csum&0xff) && (curread==201))
			return curread-1;
		if (curread!=201)
			return curread;
		fprintf(stderr,"BAD CHECKSUM %x vs %x, trying resend...\n",buf[curread-1],csum&0xff);
		/*_send_cmd(fd,0xfff3);*/
		fprintf(stderr,"\n");
		return curread-1;
	}
	/* not reached */
}

int
serial_index_reader(int fd,unsigned char **indexbuf) {
	int	xsize,packets=0,curread=0,ret=0,expsize;

	cmd_a4(fd);
	xsize = imgsize(fd);xsize = (xsize/(64*48)) * (64*48);
	*indexbuf = malloc(xsize+400);
	_send_cmd(fd,0xfff1);
	while (1) {
	    	expsize=xsize-curread;
	    	if (expsize>200) expsize=200;
		ret=getpacket(fd,(*indexbuf)+curread,expsize);
		if (ret<0)
			break;
		curread+=ret;
		packets++;
		if (ret<200)
			break;
		_send_cmd(fd,0xfff1);
	}
	return curread/64/48;
}

int
serial_image_reader(int fd,int nr,unsigned char ***imagebufs,int *sizes) {
	int	picnum,packets,curread,ret=0;

	/*fprintf(stderr,"serial_image_reader(nr=%d)\n",nr);*/

	cmd_a1(fd,nr);
	*imagebufs = (unsigned char**)malloc(3*sizeof(char**));
	for (picnum=0;picnum<3;picnum++) {
		packets=0;
		curread=0;
		sizes[picnum] = imgsize(fd);
		(*imagebufs)[picnum]=(unsigned char*)malloc(sizes[picnum]+400);
		_send_cmd(fd,0xfff1);
		while (1) {
		    	int expsize = sizes[picnum]-curread;
			if (expsize>200) expsize=200;
			ret=getpacket(fd,(*imagebufs)[picnum]+curread,expsize);
			if (ret==0)
				break;
			curread+=ret;
			packets++;
			if (ret<200)
				break;
			_send_cmd(fd,0xfff1);
		}
	}
	return 1;
}

int
serial_open(char *dev) {
	int fd;
	struct termios tios;

	fd = open(dev,O_RDWR);
	if (fd==-1) {
	    perror("open serial");
	    return -1;
	}
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 10;

	cfmakeraw(&tios);
	tios.c_iflag &= ~(INPCK|IXON|IXOFF|ISTRIP|BRKINT|IGNCR|ICRNL|INLCR|IMAXBEL);
	tios.c_iflag |=   IGNBRK;
	tios.c_oflag &= ~(OPOST);
	tios.c_cflag &= ~(CBAUD|HUPCL|CRTSCTS|CSTOPB);
	tios.c_cflag |= CLOCAL|CREAD|B115200|CS8;
	tios.c_lflag &= ~(ICANON|ECHO|ISIG);
        /*tios.c_lflag |= NOFLSH;*/

	cfsetospeed(&tios,115200);
	cfsetispeed(&tios,115200);
	if (-1==tcsetattr(fd,TCSANOW,&tios)) {
		perror("tcsetattr");
		return -1;
	}
	if (!ping(fd)) {
	    fprintf(stderr,"PING JD11 failed!\n");
	    close(fd);
	    return -1;
	}
	if (!float_query(fd)) {
	    fprintf(stderr,"FLOAT query on jd11 failed.\n");
	    close(fd);
	    return -1;
	}
	return fd;
}

void
serial_close(int fd) {
	close(fd);
}

