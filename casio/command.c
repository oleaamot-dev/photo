#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <termios.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gdk_imlib.h>
#include <gdk/gdk.h>

#include "sdComm.h"
#include "../src/gphoto.h"
#include "../src/util.h"
#include "getuint.h"
#include "ppm.h"
#include "jpeg.h"
#include "printMsg.h"
#include "messages.h"
#include "qv_io.h"
#include "casio_qv_defines.h"

extern int qvverbose;
extern int qv7xxprotocol;

int
QVok(sdcInfo info) {
    unsigned char c;
    int attempts = 0;

    while (attempts < RETRY_COUNT) {
	casio_qv_send_byte(info, ENQ);
	if (casio_qv_read(info, &c, 1) == SDC_FAIL) {
	    attempts++;
	    continue;
	}
	if (c == ACK) {
	    if (attempts > 0) {
		sdcFlush(info);
	    }
	    RESET_CHECKSUM();
	    return(GPHOTO_SUCCESS);
	}
	attempts++;
    }
    return(GPHOTO_FAIL);
}

int
QVreset(sdcInfo info, int flag) {
    u_char	c;

    if (!QVok(info))
	return -1;			/*ng*/

    if(flag)
	casio_qv_write(info, "QR", 2);
    else
	casio_qv_write(info, "QE", 2);

    /*supposed to be 0x5c('Z') or 0x69*/
    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);

#if !defined(BSD)  /* Why ? */
    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVreset");
	return(-1);
    }
#endif
    casio_qv_send_byte(info, ACK);

    return((int)c);		/*ok*/
}

int
QVbattery(sdcInfo info)
{
    u_char        c;

    if (!QVok(info))
	return -1;                  /*ng*/


    casio_qv_write(info, "RB", 2);
    casio_qv_send_byte(info, ENQ);
    casio_qv_send_byte(info, 0xff);
    casio_qv_send_byte(info, 0xfe);
    casio_qv_send_byte(info, 0xe6);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVbattery");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);

#ifdef AAAAAAA
    if(qv7xxprotocol == 0) {
	casio_qv_write(info, "RB", 2);
	casio_qv_send_byte(info, ENQ);
	casio_qv_send_byte(info, 0xff);
	casio_qv_send_byte(info, 0xfe);
	casio_qv_send_byte(info, 0xe6);
	if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
	if (!casio_qv_confirm_checksum(info, c)) {
	    print_error(CHECKSUM_FAILED, "QVbattery");
	    return(-1);
	}
	casio_qv_send_byte(info, ACK);
	if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
	if (!casio_qv_confirm_checksum(info, c)) {
	    print_error(CHECKSUM_FAILED, "QVbattery");
	    return(-1);
	}
	casio_qv_send_byte(info, ACK);
	if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
    } else {
	casio_qv_write(info, "BC", 2);
	if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
	casio_qv_send_byte(info, ACK);
	if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
    }
#endif

    return (int) c;
}

long
QVrevision(sdcInfo info) {
    u_char	c;
    int         i;
    long r;

    if (!QVok(info))
	return -1;			/*ng*/

    casio_qv_write(info, "SU", 2);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVrevision");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);
    for (i = 0, r = 0; i < 4; i++) {
        if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
	r = (r << 8) | c;
    }
    return(r);
}


int 
QVsectorsize(sdcInfo info, int newsize) {
    u_char s;
    u_char t;

    s = (u_char) (newsize >> 8) & 0xff;
    t = (u_char) newsize &  0xff;
    if (!QVok(info))
	return -1;                  /*ng*/

    casio_qv_write(info, "PP", 2);
    casio_qv_send_byte(info, s);
    casio_qv_send_byte(info, t);
    if (casio_qv_read(info, &s, 1) == SDC_FAIL) return(-1);
    if (!casio_qv_confirm_checksum(info, s)) {
	print_error(CHECKSUM_FAILED, "QVsectorsize");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);
    return 1;
}

int
QVpicattr(sdcInfo info, int picture_number) {
    unsigned char	c;

    if (QVok(info) == GPHOTO_FAIL)
	return(-1);

    casio_qv_write(info, "DY", 2);
    casio_qv_send_byte(info, 0x02);
    casio_qv_send_byte(info, picture_number);

    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);
    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVpicattr");
	return(-1);
    }
    
    casio_qv_send_byte(info, ACK);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(-1);

    return c;  /* picture attribute */
    /* 00 protect off semi qvga  */
    /* 01 protect on  semi qvga */
    /* 02 protect off vga */
    /* 03 protect on  vga */
}

int
QVshowpicture(sdcInfo info, int picture_number) {
    unsigned char c;

    if (!QVok(info))
	return -1;			/*ng*/

    casio_qv_write(info, "DA", 2);
    casio_qv_send_byte(info, picture_number);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1); /*supposed to be 0x7a - n*/

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVshowpicture");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);
    return 1;			/*ok*/
}

int
QVgetsize(sdcInfo info, int picture_number) {
    unsigned char c;
    int len;
    
    if (!QVok(info))
        return -1;			/*ng*/

    if (picture_number)
	casio_qv_write(info, "Mq", 2);

    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1);

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVgetsize");
	return(-1);
    }

    casio_qv_send_byte(info, ACK);
    if (picture_number) {
	if (casio_qv_read(info, &c, 1) == SDC_FAIL)
	    return(-1);
	len = ((int)c) << 8;

	if (casio_qv_read(info, &c, 1) == SDC_FAIL)
	    return(-1);
	len += ((int)c);
    }
    return len;
}

int
QVgetsize2(sdcInfo info, int picture_number)
{
    /* QV700/770 only */
    unsigned char c;
    long filesize = 0;

    if (QVshowpicture(info, picture_number) < 0)
	return -1;			/*ng*/

    if (!QVok(info))
	return -1;			/*ng*/

    casio_qv_write(info, "DL", 2);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1); /*supposed to be 0x6f */

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVgetsize2");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);

    if (!QVok(info))
	return -1;			/*ng*/

    casio_qv_write(info, "EM", 2);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1);
    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVgetsize2");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);
    
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1);
    filesize = c;

    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1);
    filesize = filesize << 8;
    filesize = filesize | c;

    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1);
    filesize = filesize << 8;
    filesize = filesize | c;
    
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1);
    filesize = filesize << 8;
    filesize = filesize | c;
    return(filesize);
}

static int
calcsum(unsigned char *buf, int len) {
    unsigned char *c;
    int i;
    int total = 0;

    for (i = 0, c = buf; i < len; i++, c++) {
	total += (int)*c;
    }
    return(total);
}

int
QVblockrecv(sdcInfo info, unsigned char *buf, int filesize, int show_percent_read) {
    unsigned char	s;
    unsigned char	t;
    unsigned char	*p;
    unsigned int	sectorsize;
    int sum;
    int retrycount = RETRY_COUNT;
    int errorOccurred = 0;
    int totalRead = 0;
    int expectedAmount = 0;

    casio_qv_send_byte(info, DC2);

    p = buf;
    while (1) {
#if 0
	if (qvverbose) {
	    if(filesize == 0)
		fprintf(stderr, "%6d\b\b\b\b\b\b", p - buf);
	    else
		fprintf(stderr,
			"%6d/%6d\b\b\b\b\b\b\b\b\b\b\b\b\b", p - buf, filesize);
	}
#endif

    nak:;
	/* x: fault handlers */
	if (errorOccurred) {
	    casio_qv_send_byte(info, NAK);
	    if (qvverbose)
	        fprintf(stderr, "*********retry*********\n");
	}

    retry:;
        errorOccurred = 0;
	retrycount --;

	/* 1: obtain sector size */
	if (casio_qv_read(info, &s, 1) == SDC_FAIL) {
	    if (retrycount) {
		errorOccurred = 1;
		goto nak;
	    }
	    return(-1);
	}
	if (s  != STX) {
	    if (qvverbose)
		fprintf(stderr,"NG sector size(%02x)\n",s );
	    sdcFlush(info);
	    casio_qv_send_byte(info, NAK);
	    if (retrycount) {
		errorOccurred = 1;
		goto retry;
	    }
	    return -1;		/*ng*/
	}

	if (casio_qv_read(info, &s, 1) == SDC_FAIL) {
	    if (retrycount) {
		errorOccurred = 1;
		goto nak;
	    }
	    return(-1);
	}
	sum = s;
	
	if (casio_qv_read(info, &t, 1) == SDC_FAIL)
	    return(-1);
	sum = sum + t;
	sectorsize = ((u_int)s << 8) | t;

	/* 2: drain it */
	if (casio_qv_read(info, p, sectorsize) == SDC_FAIL) {
	    if (retrycount) {
		errorOccurred = 1;
		goto nak;
	    }
	    return(-1);
	}
	sum = sum + calcsum(p, sectorsize);
	p += sectorsize;
	totalRead += sectorsize;

	/* 3: finalize sector */
	/*sector type?*/
	if (casio_qv_read(info, &s, 1) == SDC_FAIL) {
	    if (retrycount) {
		errorOccurred = 1;
		goto nak;
	    }
	    return(-1);
	}

	if (show_percent_read) {
	    float percentRead;

	    if (expectedAmount == 0) {
		expectedAmount = 136 + get_u_short(buf + 2) +
				       get_u_short(buf + 4) +
				       get_u_short(buf + 6);
	    }
	    percentRead = ((float)totalRead) / expectedAmount;
	    if (expectedAmount)
		    update_progress(100 * totalRead / expectedAmount);
	}

	if (casio_qv_read(info, &t, 1) == SDC_FAIL) {
	    if (retrycount) {
		errorOccurred = 1;
		goto nak;
	    }
	    return(-1);
	}
	t = 0xff & ~t;	/*checksum?*/
	sum = 0xff & (sum + s);
	if (sum != t) {
	    sdcFlush(info);
	    casio_qv_send_byte(info, NAK);
	    goto retry;
	}
		
	if (s == ETX) {
	    /* final sector... terminate transfer */
	    casio_qv_send_byte(info, ACK);
	    break;
	} else if (s == ETB) {
	    /* block cleanup */	
	    casio_qv_send_byte(info, ACK);
	} else {
	    /* strange condition... retry this sector */
	    sdcFlush(info);
	    casio_qv_send_byte(info, NAK);
	    goto retry;
	}
    }

    if (qvverbose)
	fprintf(stderr, "\n");

    return p - buf;
}

int
QVgetpicture(sdcInfo info, int picture_number, unsigned char *buf,
	     int format, int vga, int show_percent_read) {
    u_char c;
    int	len;
    long filesize = 0;

    if(vga == 2){
	if((format == JPEG) || (format == CAM)) {
	    filesize = QVgetsize2(info, picture_number);
	    if (filesize < 0) return -1;
	}
    }

    if (QVshowpicture(info, picture_number) < 0)
	return -1;			/*ng*/

    if (!QVok(info))
	return -1;			/*ng*/

    casio_qv_write(info, "DL", 2);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1); /*supposed to be 0x6f */

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVgetpicture");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);

    if (!QVok(info))
	return -1;			/*ng*/

    switch(format){
	case PPM_T:
	case RGB_T:
	case BMP_T:
	    casio_qv_write(info, "MK", 2);
	    break;

	case JPG_T:
	    casio_qv_write(info, "MF", 2);
	    break;

	case PPM_P:
	case RGB_P:
	case BMP_P:
	    if(vga)
		casio_qv_write(info, "Ml", 2);
	    else
		casio_qv_write(info, "ML", 2);
	    break;

	case JPEG:
	default:
	    if(vga == 1) {
		casio_qv_write(info, "Mg", 2);
	    } else if (vga == 2) {
		casio_qv_write(info, "EG", 2);
	    }
	    else {
		casio_qv_write(info, "MG", 2);
	    }
	    break;
    }

    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(-1); /*supposed to be 0x6f */

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVgetpicture");
	return(-1);
    }
    casio_qv_send_byte(info, ACK);

    if (qvverbose){
	switch(format){
	    case PPM_T:
	    case RGB_T:
	    case BMP_T:
		fprintf(stderr, "Thumbnail %3d: ", picture_number);
		break;
	    case CAM:
	    case JPEG:
	    case PPM_P:
	    case RGB_P:
	    case BMP_P:
	    default:
		fprintf(stderr, "Picture   %3d: ", picture_number);
		break;
	}
    }

    len = QVblockrecv(info, buf, filesize, show_percent_read);

    if (!QVok(info))
	return -1;			/*ng*/

    return len;
}

struct Image *
casio_qv_download_thumbnail(sdcInfo info, int picture_number) {
    int picType;
    int vga = 0;
    unsigned char buf[THUMBNAIL_MAXSIZE];
    struct Image *cameraImage = NULL;
    int len;

    picType = QVpicattr(info, picture_number);
    if (picType == -1) {
	return(NULL);
    } else {
	if (picType & 0x02) {
	    vga = 1;
	    if(qv7xxprotocol != 0) {
	        vga = 2;
	    }
	} 
    }

    cameraImage = (struct Image *)malloc(sizeof(struct Image));
    if (cameraImage == NULL) {
	return(NULL);
    }

    len = QVgetpicture(info, picture_number, buf, PPM_T, vga, 0);
    if (len < 0) {
	return(NULL);
    }

    record_ppm(buf, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, 2, 2, 1, 0, cameraImage);
    cameraImage->image_info_size = 0;
    cameraImage->image_info = NULL;
    strcpy(cameraImage->image_type, "ppm");
    return(cameraImage);
}

struct Image *
casio_qv_download_picture(sdcInfo info, int picture_number,
			  int lowResPictureSize) {
    int picType;
    int vga = 0;
    unsigned char buf[PICTURE_MAXSIZE];
    char picFileName[1024];
    struct Image *cameraImage = NULL;
    GdkImlibImage *imlibImage = NULL;
    GdkImlibImage *rescaledImlibImage = NULL;
    FILE *picFP;
    struct stat picFileInfo;
    int len;

    cameraImage = (struct Image *)malloc(sizeof(struct Image));
    if (cameraImage == NULL) {
	return(NULL);
    }

    picType = QVpicattr(info, picture_number);
    if (picType == -1) {
	return(NULL);
    } else {
	if (picType & 0x02) {
	    vga = 1;
	    if(qv7xxprotocol != 0) {
	        vga = 2;
	    }
	} 
    }

    sprintf(picFileName, "%s/pic_%d.jpg", gphotoDir, picture_number);
    picFP = fopen(picFileName, WRITE_MODE);
    if (picFP == NULL) {
	print_error(CANT_OPEN_FILE, "picture", picFileName);
        return(NULL);
    }

    if (qvverbose)
        fprintf(stderr, "Opened picture file %s\n", picFileName);

    len = QVgetpicture(info, picture_number, buf, JPEG, vga, 1);
    if (len < 0) {
	fclose(picFP);
	return(NULL);
    }

    if (vga == 1) {
	if (write_jpeg_fine(buf, picFP) == -1) {
	    fclose(picFP);
	    return(NULL);
	}
    } else if (vga == 2) {
	if (casio_write_file(buf, len, picFP) == -1) {
	    fclose(picFP);
	    return(NULL);
	}
    } else {
	if (write_jpeg(buf, picFP) == -1) {
	    fclose(picFP);
	    return(NULL);
	}
    }

    fclose(picFP);
    imlibImage = gdk_imlib_load_image(picFileName);
    unlink(picFileName);

    if (lowResPictureSize == SIZE_640_x_480)
	rescaledImlibImage = gdk_imlib_clone_scaled_image(imlibImage, 640, 480);
    else rescaledImlibImage = gdk_imlib_clone_scaled_image(imlibImage, 320, 240);
    /* Free original, unscaled image */
    gdk_imlib_kill_image(imlibImage);

    gdk_imlib_save_image(rescaledImlibImage, picFileName, NULL);
    gdk_imlib_kill_image(rescaledImlibImage);

    picFP = fopen(picFileName, "rb");
    if (picFP == NULL) {
	print_error(CANT_OPEN_FILE, "picture", picFileName);
        return(NULL);
    }
    stat(picFileName, &picFileInfo);
    cameraImage->image_size = picFileInfo.st_size;
    cameraImage->image = (void *)malloc(cameraImage->image_size);
    if (cameraImage->image == NULL) {
	cameraImage->image_size = 0;
	return(NULL);;
    }
    fread(cameraImage->image, cameraImage->image_size, 1, picFP);
    fclose(picFP);
    unlink(picFileName);

    /*record_jpeg(buf, cameraImage);*/
    cameraImage->image_info_size = 0;
    cameraImage->image_info = NULL;
    strcpy(cameraImage->image_type, "jpeg");

    return(cameraImage);
}

int
QVdeletepicture(sdcInfo info, int picture_number) {
    u_char	c;

    if (!QVok(info))
	return(GPHOTO_FAIL);
    casio_qv_write(info, "DF", 2);
    casio_qv_send_byte(info, picture_number);
    casio_qv_send_byte(info, 0xff);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(GPHOTO_FAIL);		 /*supposed to be 0x76 - n*/

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "QVgetpicture");
	return(GPHOTO_FAIL);
    }
    casio_qv_send_byte(info, ACK);
    return(GPHOTO_SUCCESS);
}

int
casio_qv_record(sdcInfo info) {
    u_char	c;

    if (!QVok(info))
	return(GPHOTO_FAIL);
    casio_qv_write(info, "DR", 2);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(GPHOTO_FAIL);

    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "casio_qv_record");
	return(GPHOTO_FAIL);
    }

    casio_qv_send_byte(info, ACK);
    if (casio_qv_read(info, &c, 1) == SDC_FAIL)
        return(GPHOTO_FAIL);

    if (c != ACK) {
	print_error(NOT_IN_RECORD_MODE);
	return(GPHOTO_FAIL);
    }

    return(GPHOTO_SUCCESS);
}

int
casioSetPortSpeed(sdcInfo info, int speed) {
    int n;
    u_char	c;
    int baud;
    extern int currentBaudRate;

    if(speed == sdcGetBaudRate(info))
	return(GPHOTO_SUCCESS);

    if (QVok(info) == GPHOTO_FAIL)
	return(GPHOTO_FAIL);			/*ng*/

    switch(speed) {
	case LIGHT:			/* 115200 baud */
	    n = 3;
#if defined(WIN32) || defined (OS2) || defined(BSD) || defined(DOS) || defined(__linux__)
	    baud = B115200;
#else
	    baud = B38400;
#endif
	    break;

	case TOP:			/* 57600 baud */
	    n = 7;
#if defined(WIN32) || defined(OS2) || defined(BSD) || defined(DOS) || defined(__linux__)

	    baud = B57600;
#else
	    baud = B38400;
#endif
	    break;
	    
	case HIGH:			/* 38400 baud */
#ifdef X68
	    if(qvhasvgamode)
		n = 11;  /* QV-100/300  */
	    else
		n = 10;                     /* 39062.5 baud */
#else
	    n = 11;
#endif
	    baud = B38400;
	    break;
	    
	case MID:			/* 19200 baud */
#ifdef X68
	    n = 23;
#else
	    n = 22;
#endif
	    baud = B19200;
	    break;
	    
	case DEFAULT:
	default:
	    n = 46;
	    baud = B9600;
	    break;
	}

    if (casio_qv_write(info, "CB", 2) == SDC_FAIL) return(GPHOTO_FAIL);
    if (casio_qv_send_byte(info, n) == SDC_FAIL) return(GPHOTO_FAIL);

    if (casio_qv_read(info, &c, 1) == SDC_FAIL) return(GPHOTO_FAIL);
    if (!casio_qv_confirm_checksum(info, c)) {
	print_error(CHECKSUM_FAILED, "casioSetPortSpeed");
	return(GPHOTO_FAIL);
    }

    casio_qv_send_byte(info, ACK);

    sleep(1);			/* ??? */
    sdcSetBaudRate(info, baud);
    currentBaudRate = baud;
    if (QVok(info) == GPHOTO_FAIL)
	return(GPHOTO_FAIL);			/*ng*/
    return(GPHOTO_SUCCESS);
}
