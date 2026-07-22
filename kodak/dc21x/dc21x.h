#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <stdarg.h>

#define CAMERA_EPOC              852094800

//#define DEBUG 1
#define TRUE 1
#define FALSE 0

#define CAM_INFO 1
#define PIC_INFO 2

#define TAKE_PIC 1
#define DELE_PIC 2

#define SPD_VHI 1
#define SPD_LOW 0

#define PKT_CTRL_RECV            0x01
#define PKT_CTRL_SEND            0x00
#define PKT_CTRL_EOF             0x80
#define PKT_CTRL_CANCEL          0xFF

#define DC_COMMAND_COMPLETE      0x00
#define DC_COMMAND_ACK           0xD1
#define DC_CORRECT_PACKET        0xD2
#define DC_COMMAND_NAK           0xE1
#define DC_ILLEGAL_PACKET        0xE3
#define DC_BUSY                  0xF0
  
#define DC_SET_RESOLUTION        0x36
#define DC_SET_FILEFORMAT	 0x37 // Write: [8]0x37 0x00 0x03 0x00 0x00 0x00 0x00 0x1a : 3 or 4
#define DC_SET_SPEED             0x41
#define DC_PICTURE_DOWNLOAD      0x64
#define DC_PICTURE_INFO          0x65
#define DC_PICTURE_THUMBNAIL     0x66
#define DC_SET_QUALITY		 0x71 // Write: [8]0x71 0x00 0x03 0x00 0x00 0x00 0x00 0x1a : 1-3
#define DC_SET_FLASH		 0x72 // Write: [8]0x72 0x00 0x## 0x00 0x00 0x00 0x00 0x1a : 1-5
#define DC_SET_SOMETHING_WEIRD   0x75
#define DC_SET_ZOOM		 0x78 // Write: [8]0x78 0x00 0x## 0x00 0x00 0x00 0x00 0x1a : 1-4, 5=macro 
#define DC_TAKE_PICTURE          0x7C
#define DC_ERASE                 0x7A
#define DC_ERASE_IMAGE_IN_CARD   0x7B
#define DC_INITIALIZE            0x7E
#define DC_STATUS                0x7F
#define DC_SET_EXPCOMP		 0x80 // Write: [8]0x80 0x00 0x04 0x00 0x00 0x00 0x00 0x1a
#define DC_SET_CAMERA_ID         0x9E // 60 bytes + checksum.
 // 0x0b 0x4e 0xffffffed 0x06 0x00 0x1a

struct kodak_dc210_status {
   char    camera_type_id;              /* 1 */
   char    firmware_major;              /* 2 */
   char    firmware_minor;              /* 3 */
   char    battery;     	/* 8 battery status */
   char    acstatus;                  /* 9 */
   long int time;	        	/* 11-14 */
   char    zoomMode;     	     	/* 15 */
   char    flash_charged;		/* 17 */
   char    compression_mode_id;        	/* 18 */
   char    flash_mode;	        	/* 19 */
   char    exposure_compensation;	/* 20 */
   char    picture_size;		/* 21 */
   char    filetype;		        /* 22 */
   int totalPicturesTaken;          /* 25-26 */
   int totalFlashesFired;   	/* 27-28 */
   int num_pictures;         	/* 56-57 */
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

// from lowlevel.c
int my_read (unsigned char *buf, int nbytes );
void my_write(char byte);
int send_command (char command, int arg1, int arg2, int arg3, int arg4);
int command_complete(void);
void camera_init(void);
unsigned char checksum(char *packet,int length);
int read_packet ( char *packet, int length);
void change_speed (int speed);
int get_camera_status(void);
int open_camera (const char *devname);
void get_picture_info (int picture);
int pic_ops ( int type , int arg );
char get_thumbnail ( int picture );
char get_picture (int picture);

// from main.c
void eprintf(char *fmt, ...);
void print_info( int type );

int serialdev;
int quiet;
char *fname;

struct kodak_dc210_picture_info picinfo;
struct kodak_dc210_status status;
