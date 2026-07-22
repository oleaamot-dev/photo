#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <glib.h>

#define CAMERA_EPOC              852094800

/* camera types */
#define CAMERA_TYPE_DC50_CAMERA_TYPE    0x01
#define CAMERA_TYPE_DC120_CAMERA_TYPE   0x02
#define CAMERA_TYPE_DC200_CAMERA_TYPE   0x03
#define CAMERA_TYPE_DC210_CAMERA_TYPE   0x04


/* Control bytes */
#define PKT_CTRL_RECV            0x01
#define PKT_CTRL_SEND            0x00
#define PKT_CTRL_EOF             0x80
#define PKT_CTRL_CANCEL          0xFF


/* Kodak System Codes */
#define DC_COMMAND_COMPLETE      0x00
#define DC_COMMAND_ACK           0xD1
#define DC_CORRECT_PACKET        0xD2
#define DC_COMMAND_NAK           0xE1
#define DC_ILLEGAL_PACKET        0xE3
#define DC_BUSY                  0xF0
  
/* Kodak System Commands */
#define DC_SET_RESOLUTION        0x36
#define DC_SET_SPEED             0x41
#define DC_PICTURE_DOWNLOAD      0x64
#define DC_PICTURE_INFO          0x65
#define DC_PICTURE_THUMBNAIL     0x66
#define DC_SET_SOMETHING         0x75
#define DC_TAKE_PICTURE          0x7C
#define DC_ERASE                 0x7A
#define DC_ERASE_IMAGE_IN_CARD   0x7B
#define DC_INITIALIZE            0x7E
#define DC_STATUS                0x7F
#define DC_SET_CAMERA_ID         0x9E

struct kodak_dc210_status {
   char    camera_type_id;              /* 1 */
   char    firmware_major;              /* 2 */
   char    firmware_minor;              /* 3 */
   char    batteryStatusId;     	/* 8 battery status */
   char    acStatusId;                  /* 9 */
   guint32 time;	        	/* 11-14 */
   char    zoomMode;     	     	/* 15 */
   char    flash_charged;		/* 17 */
   char    compression_mode_id;        	/* 18 */
   char    flash_mode;	        	/* 19 */
   char    exposure_compensation;	/* 20 */
   char    picture_size;		/* 21 */
   char    file_Type;		        /* 22 */
   guint16 totalPicturesTaken;          /* 25-26 */
   guint16 totalFlashesFired;   	/* 27-28 */
   guint16 num_pictures;         	/* 56-57 */
   char    camera_ident[32];      	/* 90-122 */
};

struct kodak_dc210_picture_info {
   char reserved_1[3];
   char resolution;
   char compression;
   char reserved_2;
   short  pictureNumber;
   int  fileSize;
   int  elapsedTime;
   char reserved_3[16];
   char fileName[12];
   char reserved_4[222];
};

