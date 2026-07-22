#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include "dc21x.h"

/* Open serial port attached to the camera */
int open_camera (const char *devname)
{
  struct termios newt, oldt;

  serialdev = open(devname, O_RDWR|O_NOCTTY|O_NONBLOCK);
  if (serialdev < 0) {
     eprintf("Cannot open %s\n",devname);
     exit(1);
  }
  if (tcgetattr(serialdev, &oldt) < 0) { 
     eprintf("Cannot get serial parameters for %s\n",devname);
     exit(1);
  }
  memcpy((char *)&newt,(char *)&oldt, sizeof(struct termios));
  cfmakeraw(&newt);
  newt.c_cflag &= ~CSTOPB;
  newt.c_cflag &= ~PARENB;
  newt.c_cflag |= CS8;
  newt.c_cflag &= ~PARODD;
  newt.c_cflag |= CRTSCTS;
  newt.c_cc[VMIN] = 0;
  newt.c_cc[VTIME] = 10;
  cfsetospeed(&newt, B9600);
  cfsetispeed(&newt, B9600);
  if (tcsetattr(serialdev, TCSANOW, &newt) < 0) {
    eprintf("Cannot set serial parameters for %s\n",devname);
    exit(1);
  }
  return serialdev;
}

/* Camera read routine */
int my_read (unsigned char *buf, int nbytes )
{
  int ret;
  int numRead = 0;
  int n;
  int len;
  fd_set readfds;
  struct timeval timeout;

  while (numRead < nbytes) {
    n = serialdev + 1;
    FD_ZERO(&readfds);
    FD_SET(serialdev, &readfds);
    timeout.tv_sec = 6;
    timeout.tv_usec = 500000;
    ret = select(n, &readfds, NULL, NULL, &timeout);

    if (ret <= 0) {
      
      if (ret<0)
	if (errno == EINTR) continue; // while loop
        else perror("select");
      else
	eprintf("camera_read: read timed out\n");
      return 0;

    } else {
      if (FD_ISSET(serialdev, &readfds)) {
        len = read(serialdev, buf + numRead, nbytes - numRead);
        if (len < 0) 
	  if (errno == EINTR) continue;
	else { 
	  eprintf("camera_read: read failed on a ready file handle\n");
	  perror("read");
	  return 0;
        }
        else numRead += len;
      }
      else return 1;
    }
  } // end while
  return 0;
}

/* Camera write routine */
void my_write(char byte)
{
  write(serialdev, &byte, 1);
}

/* Read packet size length from camera */
int read_packet ( char *packet, int length)
{
  int success = TRUE;
  unsigned char sent_checksum;
  unsigned char computed_checksum;
  unsigned char control_byte;

  my_read(&control_byte,1);
  my_read(packet,length);

  my_read(&sent_checksum, 1);

  computed_checksum = checksum(packet,length);
  
  if (sent_checksum != computed_checksum)
  {
    eprintf("Bad checksum %d != %d\n",computed_checksum,sent_checksum);
    my_write(DC_ILLEGAL_PACKET);
    success = FALSE;
  } else  {
    my_write(DC_CORRECT_PACKET);
    success = TRUE;
  }
  return success;
}

/* Calculate simple Kodak checksum */
unsigned char checksum(char *packet,int length)
{
  unsigned char chksum = 0;
  int i;
  for (i = 0 ; i < length ; i++)
    chksum ^= packet[i];
  return chksum;
}

/* Send command, read ack from cam */
int send_command ( char command, int arg1, int arg2, int arg3, int arg4)
{
  unsigned char ack;
  char packet[10];

#ifdef DEBUG
  eprintf("DEBUG: Sending command: 0x%02x\n",command);
#endif
  sprintf(packet,"%c%c%c%c%c%c%c%c",command,0x00,arg1,arg2,arg3,arg4,0x00,0x1A);
  write(serialdev, packet, 8);
  my_read( &ack, 1 );

  if (ack!=DC_COMMAND_ACK) {
    eprintf("Packet send error\n");
    return FALSE;
  }
  return TRUE;
}

int command_complete(void) 
{
  int status = DC_BUSY;
  int success=TRUE;

  do {
    my_read( (char *)&status,1);
  } while (status == DC_BUSY);
  
  if (status == DC_COMMAND_COMPLETE ) {
#ifdef DEBUG
      eprintf("DEBUG: Command complete\n");
#endif
      success=TRUE;
  } else success=FALSE;
  return success;
}

void camera_init(void)
{
 int status=TRUE;
  status=send_command(DC_INITIALIZE,0x00,0x00,0x00,0x00);
  usleep(100000);
  command_complete();
 
#ifdef DEBUG
  eprintf("DEBUG: Initialize sent\n");
#endif
}

/* Change port speed to transfer images quickly */
void change_speed ( int speed ) {
  struct termios newt, oldt;
  int arg1,arg2;
    
  if (tcgetattr(serialdev, &oldt) < 0) {
    eprintf("Cannot get serial parameters.\n");
    exit(1);
  }
  memcpy((char *)&newt,(char *)&oldt, sizeof(struct termios));
  arg1 = speed ? 0x11 : 0x96;
  arg2 = speed ? 0x52 : 0x00;
  send_command(DC_SET_SPEED,arg1,arg2,0x00,0x00);
  usleep(500000); /* 500ms sleep after speed change */
  cfsetospeed(&newt, speed ? B115200 : B9600);
  cfsetispeed(&newt, speed ? B115200 : B9600);
  if (tcsetattr(serialdev, TCSANOW, &newt) < 0) {
    eprintf("Cannot set serial parameters.\n");
    exit(1);
  }
}

int get_camera_status(void)
{
  unsigned char packet[256];
  int success = TRUE;

  success=send_command(DC_STATUS,0x00,0x00,0x00,0x00);
  read_packet(packet,256); 
  
  status.camera_type_id        = packet[1];
  status.firmware_major        = packet[2];
  status.firmware_minor        = packet[3];
  status.battery               = packet[8];
  status.acstatus              = packet[9];

  status.time = CAMERA_EPOC + (
				packet[12] * 0x1000000 + 
				packet[13] * 0x10000 + 
				packet[14] * 0x100 + 
				packet[15]) / 2;

  status.zoomMode              = packet[15];
  status.flash_charged         = packet[17];
  status.compression_mode_id   = packet[18];
  status.flash_mode            = packet[19];
  status.exposure_compensation = packet[20];
  status.picture_size          = packet[21];
  status.filetype              = packet[22];
  status.totalPicturesTaken    = packet[25] * 0x100 + packet[26];
  status.totalFlashesFired     = packet[27] * 0x100 + packet[28];
  status.num_pictures          = packet[56] * 0x100 + packet[57];
  strncpy(status.camera_ident,packet + 90,32); 
  
  success=command_complete();
  return success;
}

void get_picture_info (int picture)
{
  unsigned char packet[256];

  send_command(DC_PICTURE_INFO,0x00,picture-1,0x00,0x00);
  read_packet(packet,256);
  picinfo.resolution    = packet[3];
  picinfo.compression   = packet[4];
  picinfo.pictureNumber = packet[6] * 0x100 + packet[7];
  picinfo.fileSize      = (int)packet[8]  * 0x1000000 + 
			   (int)packet[9]  * 0x10000 + 
                           (int)packet[10] * 0x100 + 
                           (int)packet[11];
  picinfo.elapsedTime   = (int)packet[12] * 0x1000000 + 
			   (int)packet[13] * 0x10000 + 
		           (int)packet[14] * 0x100 + 
			   (int)packet[15];
  strncpy(picinfo.fileName,packet + 32,12);
  command_complete();
}

int pic_ops ( int type , int arg )
{
 int ret=0;
 
 switch (type) {
  case TAKE_PIC:
   send_command(DC_TAKE_PICTURE,0x00,0x00,0x00,0x00);
   command_complete();
   get_camera_status();
   ret=status.num_pictures;
  break;
  case DELE_PIC:
   send_command(DC_ERASE_IMAGE_IN_CARD,0x00,arg-1,0x00,0x00);
   command_complete();
   ret=0;
  break;
 }
 return ret;
}

char get_thumbnail ( int picture )
{
  int i,j;
  int numRead = 0;
  int success = TRUE;
  int fileSize = 20736;
  int blockSize = 1024;
  char *picData;
  char *imData;
  FILE *fd;
  char bmpHeader[] = {
        0x42, 0x4d, 0x36, 0x24, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x48, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  picData = (char *)malloc(fileSize + blockSize);
  imData = (char *)malloc(fileSize + 54);


  change_speed(SPD_VHI);
  send_command(DC_PICTURE_THUMBNAIL,0x00,picture-1,0x01,0x00);

  while (success && (numRead < fileSize))
  {
    if (!quiet) printf("Complete: %03d%%\n\033[A",(numRead*100)/fileSize);
    success=read_packet(picData+numRead,blockSize);
    numRead += blockSize;
  }
  if (!quiet) printf("\n");
  command_complete();
  change_speed(SPD_LOW);

  memcpy(imData,bmpHeader, sizeof(bmpHeader));

  for (i = 0; i < 72; i++) {
    for (j = 0; j < 96; j++) {
      imData[i*96*3+j*3+54] = picData[(71-i)*96*3+j*3+2];
      imData[i*96*3+j*3+54+1] = picData[(71-i)*96*3+j*3+1];
      imData[i*96*3+j*3+54+2] = picData[(71-i)*96*3+j*3];
    }
  } 

  free(picData);

  fd=fopen("/tmp/pic.bmp","w");
  fwrite(imData,fileSize+54,1,fd);
  fclose(fd);
  return fileSize+54;
}

char get_picture (int picture) 
{
  int blockSize;
  int fileSize;
  int numRead;
  char *picData;
  FILE *fd;

  get_picture_info(picture);
  change_speed(SPD_VHI);
  send_command(DC_PICTURE_DOWNLOAD, 0x00, picture-1, 0x00, 0x00);
  
  fileSize = picinfo.fileSize;
  blockSize = 1024;
  
  picData = (char *)malloc(fileSize + blockSize);
  numRead = 0;

  while (numRead < fileSize)
  {
    read_packet(picData+numRead,blockSize);
    numRead += blockSize;
    if (!quiet) printf("Complete: %03d%%\n\033[A",(numRead*100)/fileSize);
  }
  if (!quiet) printf("\n");
  command_complete();
  change_speed(SPD_LOW);

  fd=fopen(fname,"w");
  fwrite(picData,fileSize,1,fd);
  fclose(fd);

  return 0;
} 

