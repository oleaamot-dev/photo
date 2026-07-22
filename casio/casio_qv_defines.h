#ifndef _CASIO_QV_DEFINES_H
#define _CASIO_QV_DEFINES_H

/* NEXTSTEP and (Old) FreeBSD cannot hold RTS to off
   so you short link cable's RTS and GND */
/* FreeBSD 2.2 Release can hold RTS to off. */
#if defined(NeXT)
#define NO_RTS
#endif 

#define GPHOTO_SUCCESS	1
#define GPHOTO_FAIL	0

#define	STX	0x02
#define	ETX	0x03
#define	ENQ	0x05
#define	ACK	0x06

#define	DC2	0x12
#define	NAK	0x15
#define	ETB	0x17


#define DEFAULTBAUD B9600
#define LIGHT 5
#define TOP   4
#define HIGH  3
#define MID   2
#define DEFAULT 1

#define JPEG 0
#define PPM_P 1
#define PPM_T 2
#define RGB_P 3
#define RGB_T 4
#define BMP_P 5
#define BMP_T 6
#define CAM   7
#define JPG_T 8

#define RETRY_COUNT   5
#define LOW_BATT 0x3b 
#define NEW_SECTOR_SIZE 0x0600

#define THUMBNAIL_WIDTH 52
#define THUMBNAIL_HEIGHT 36
#define PICTURE_WIDTH 480
#define PICTURE_HEIGHT 240
#define PICTURE_WIDTH_FINE 640
#define PICTURE_HEIGHT_FINE 480

#define SIZE_320_x_240		0
#define SIZE_640_x_480		1

#define THUMBNAIL_MAXSIZE	  8*1024
#define	JPEG_MAXSIZE		 32*1024
#define	YCC_MAXSIZE		154*1024
#define	JPEG_MAXSIZE_VGA 	 70*1024  /* not for QV770/QV700(may be QV5000SX) */
#define	YCC_MAXSIZE_VGA		460*1024
#define PICTURE_MAXSIZE		JPEG_MAXSIZE_VGA

#ifdef BINARYFILEMODE
#define WRITE_MODE "wb"
#define READ_MODE "rb"
#else
#define WRITE_MODE "w"
#define READ_MODE "r"
#endif

#define MAX_PICTURE_NUM_QV10 96

#endif /* _CASIO_QV_DEFINES_H */
