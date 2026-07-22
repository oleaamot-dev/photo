//#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include <jpeglib.h>
//#include <setjmp.h>
#include <time.h>
#include "../src/gphoto.h"
#include "../src/util.h"

#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

char *summary="Not Available";
char *description="Polaroid PDC-700";

unsigned int utimeout=1000000; // microseconds
//unsigned int tsleep=10; // microseconds
//unsigned int tsleep=10000; // microseconds
unsigned int tsleep=10000; // microseconds

int sendseq(int fd, int len, unsigned char *list)
{
 unsigned int loop;
 unsigned int num;

 for (loop=0;loop<len;loop++)
 {
//printf("sendseq 0x%02x\n",*(list+loop));
  num=write(fd,list+loop,1);
//  if (num==1)
//   printf(">>0x%02x\n",list[loop]);
//  else
  if (num!=1)
   return 0;
  usleep(tsleep);
 }
 return 1;
}

/*
int getseq(int fd, int len, unsigned char *list)
{
 unsigned int num;
 unsigned char ch;
 unsigned int loop;

 for (loop=0;loop<len;loop++)
 {
  num=read(fd,&ch,1);
//  if ((num==1)&&(list[loop]==ch))
  if (num==1)
   printf("<-0x%02x\n",ch);
  else
   return 0;
 }
 return 1;
}
*/

int set_checksum(unsigned char *cmdseq, unsigned int cmdlen)
// sets last byte to truncated sum of all previous bytes except first three
{
 int loop;
 unsigned char checksum=0;

 for (loop=3;loop<cmdlen-1;loop++)
  checksum+=cmdseq[loop];
 cmdseq[cmdlen-1]=checksum;
}

int check_checksum(unsigned char *cmd, unsigned int cmdlen)
// returns true if last byte is the sum of previous bytes except first three
{
 int loop;
 unsigned char checksum=0;

 for (loop=3;loop<cmdlen-1;loop++)
  checksum+=cmd[loop];
 return checksum==cmd[cmdlen-1];
}

unsigned char *get_seq(int fd, unsigned int *len)
// downloads a packet across the serial line
{
 unsigned int num;
 unsigned char ch;
 unsigned int loop;
 unsigned char *seq=0;
 unsigned char header[3];
 int maxfd;
 fd_set readset;
 struct timeval timeout;

 maxfd=fd+1;
 *len=0;
 for (loop=0;loop<3;loop++)
 {
  FD_ZERO(&readset);
  FD_SET(fd,&readset);
  timeout.tv_sec=0;
  timeout.tv_usec=utimeout;
  select(maxfd,&readset,0,0,&timeout);
  if (!FD_ISSET(fd,&readset))
  {
printf("get_seq timeout expired!\n");
   return 0;
  }
  num=read(fd,&header[loop],1);
  if (num==0)
   return 0;
 }
 if (header[0]!=0x40)
  return 0;
 *len=header[2];
 *len=(*len<<8)|header[1];
 *len+=3;
 seq=(unsigned char *)malloc(*len);
 if (!seq)
 {
  *len=0;
  return 0;
 }
 strncpy((char *)seq,(char *)header,3);

// for (loop=3;loop<*len;loop++)
 for (loop=3;loop<*len;)
 {
  FD_ZERO(&readset);
  FD_SET(fd,&readset);
  timeout.tv_sec=0;
  timeout.tv_usec=utimeout;
  select(maxfd,&readset,0,0,&timeout);
  if (!FD_ISSET(fd,&readset))
  {
   *len=0;
   free(seq);
   return 0;
  }
//  num=read(fd,&seq[loop],1);
  num=read(fd,&seq[loop],*len-loop);
  if (!num)
  {
   *len=0;
   free(seq);
   return 0;
  }
  else
   loop+=num;
 }
 if (!check_checksum(seq,*len))
 {
  *len=0;
  free(seq);
  return 0;
 }
 return seq;
}

int change_baud_cmd(int fd, int baud)
// sends message to camera to change baud rate
{
 unsigned char *cmdseq=0;
 unsigned int cmdlen=0;
 unsigned char baudval=255;
 unsigned char *response;

 if (baud==57600)
  baudval=3;
 if (baud==38400)
  baudval=2;
 if (baud==19200)
  baudval=1;
 if (baud==9600)
  baudval=0;
 if (baudval==255)
  return 0;
 cmdlen=6;
 cmdseq=(unsigned char *)malloc(cmdlen);
 if (!cmdseq)
  return 0;
 cmdseq[0]=0x40;
 cmdseq[1]=(cmdlen-3)>>8;
 cmdseq[2]=(cmdlen-3)&0xff;
 cmdseq[3]=0x04; // change_baud_func
 cmdseq[4]=baudval;
 set_checksum(cmdseq,cmdlen);
 sendseq(fd,cmdlen,cmdseq);
 free(cmdseq);
//printf("expecting 0x84 0x01\n");
 response=get_seq(fd,&cmdlen);
 if (!response)
  return 0;
 if (cmdlen!=6)
 {
  free(response);
  return 0;
 }
 if (response[3]!=0x84)
 {
  free(response);
  return 0;
 }
 if (response[4]!=01)
 {
  free(response);
  return 0;
 }
// wait for a seqence with timeout
// if timeout or incorrect checksum or incorrect response, fail
 free(response);
 return 1;
}

int init_seq(int fd)
// send initializing sequence
{
 unsigned char *cmdseq=0;
 unsigned int cmdlen=0;
 unsigned char *response;
 
 cmdlen=5;
 cmdseq=(unsigned char *)malloc(cmdlen);
 if (!cmdseq)
  return 0;
 cmdseq[0]=0x40;
 cmdseq[1]=(cmdlen-3)>>8;
 cmdseq[2]=(cmdlen-3)&0xff;
 cmdseq[3]=0x01; // init
 set_checksum(cmdseq,cmdlen);
//printf("sending 0x01\n");
//printf("sendseq returned %d\n",
 sendseq(fd,cmdlen,cmdseq);
 free(cmdseq);
//printf("expecting 0x81 0x01\n");
 response=get_seq(fd,&cmdlen);
//printf("cmdlen: %d\n",cmdlen);
 if (!response)
 {
printf("init_seq No response\n");
  return 0;
 }
 if (cmdlen!=6) 
 {
  free(response);
  return 0;
 }
 if (response[3]!=0x81)
 {
  free(response);
  return 0;
 }
 if (response[4]!=0x01)
 {
  free(response);
  return 0;
 }
 free(response);
 return 1; 
} 

int init_line(int fd, int speed)
// configure serial line to a specific baud rate
{
 int num;
 int i;
 struct termios tios;
 speed_t tspeed=B57600;
  
//printf("init_line %d\n",speed);
 if (speed==9600)
  tspeed=B9600;
 if (speed==19200)
  tspeed=B19200;
 if (speed==38400)
  tspeed=B38400;
 if (speed==57600)
  tspeed=B57600;
 num=tcgetattr(fd,&tios);
 if (num<0)
  return -1;
 tios.c_iflag=0;  
 tios.c_oflag=0;
 tios.c_lflag=0;  
 cfsetospeed(&tios,tspeed);
 cfsetispeed(&tios,tspeed);
 tios.c_cflag|=(CS8|CLOCAL|CREAD);
// CLOCAL & CREAD let kernel handle serial IO
 tios.c_cc[VMIN]=1;
 tios.c_cc[VTIME]=5;
 tios.c_cflag&=~(PARENB|PARODD);
 tios.c_iflag&=~INPCK;
 tios.c_iflag|=IGNBRK;
 num=tcsetattr(fd,TCSANOW,&tios);
 if (num<0)
  return -1;
 i=ioctl(fd,TIOCMGET,&num);
 if (i<0)
  return -1;
 num&=~(TIOCM_DTR|TIOCM_RTS);
 num=ioctl(fd,TIOCMSET,&num);
 if (num<0)
  return -1;
 num=0;
 num=ioctl(fd,TIOCMBIC,&num);
 if (num<0)
  return -1;
 num=0;
 num=ioctl(fd,TIOCMBIS,&num);
 if (num<0)
  return -1;
 return 0;
}

int init_camera()
// Initializes camera for further communication
{
 int fd;
 int num;

 fd=open(serial_port,O_RDWR); // no O_NDELAY appears to wait for DCD
 if (fd<0)
  return fd;
// On powerup, the camera starts up running 9600 baud
 init_line(fd,57600);
 num=init_seq(fd);
 if (!num)
 {
  init_line(fd,38400);
  num=init_seq(fd);
  if (!num)
  {
   init_line(fd,19200);
   num=init_seq(fd);
   if (!num)
   {
    init_line(fd,9600);
    num=init_seq(fd);
    if (!num)
    {
     printf("init_seq failed\n");
     close(fd);
     fd=-1;
     return fd;
    }
   }
  }
  num=change_baud_cmd(fd,57600);
  if (!num)
  {
   printf("change_baud_cmd failed\n");
   close(fd);
   fd=-1;
   return fd;
  }
  init_line(fd,57600);
// try other baud rates, if all fail, exit
 }
/*
// A more generic initialization would try all possible speeds
 init_line(fd,9600);
 num=init_seq(fd);
 if (!num)
 {
  printf("init_seq failed\n");
  close(fd);
  fd=-1;
  return fd;
// try other baud rates, if all fail, exit
 }

 num=change_baud_cmd(fd,57600);
 if (!num)
 {
  printf("change_baud_cmd failed\n");
  close(fd);
  fd=-1;
  return fd;
 }
 init_line(fd,57600);
*/

 num=init_seq(fd);
 if (!num)
 {
  printf("init_seq failed\n");
  close(fd);
  fd=-1;
  return fd;
 }
 return fd;
}

int close_camera(int fd)
// return camera to base state, close files
{
 int num;

/*
 num=change_baud_cmd(fd,9600);
 if (!num)
 {
  printf("change_baud_cmd failed\n");
  close(fd);
  return 0;
 }

 usleep(tsleep);
 init_line(fd,9600);
 num=init_seq(fd);
 if (!num)
 {
  printf("init_seq failed\n");
  close(fd);
  return 0;
 }
*/
 close(fd);
}

int polaroid_pdc700_initialize()
// 1 means successfull
{
// printf("init\n");
// printf("init: serial port %s\n",serial_port);
 return 1;
}

//GdkImlibImage *polaroid_pdc700_get_picture(int pic_num, int thumbnail)
struct Image *polaroid_pdc700_get_picture(int pic_num, int thumbnail)
{
 struct Image *img=0;
 int fd;
 unsigned int file_size=0;
 unsigned int thumbnail_size=0;
 unsigned char *cmdseq=0;
 unsigned int cmdlen=0;
 unsigned char *response;
 unsigned int not_done=1;
 unsigned int sequence_num;
 unsigned char *ptr;
 unsigned char cmd;

 if (thumbnail)
  cmd=0x06; // request thumbnail
 else
  cmd=0x07; // request picture
//  return 0;
 fd=init_camera();
 if (fd<0)
  return 0;

// Get Picture Info, find file_size
 cmdlen=7;
 cmdseq=(unsigned char *)malloc(cmdlen);
 if (!cmdseq)
  return 0;
 cmdseq[0]=0x40;
 cmdseq[1]=(cmdlen-3)>>8;
 cmdseq[2]=(cmdlen-3)&0xff;
 cmdseq[3]=0x05; // get picture info
 cmdseq[4]=pic_num&0xff;  
 cmdseq[5]=pic_num>>8;
 set_checksum(cmdseq,cmdlen);
 sendseq(fd,cmdlen,cmdseq);
 free(cmdseq);
 response=get_seq(fd,&cmdlen);
 if (!response) 
  return 0;
 if (response[3]!=0x85)
 {
  free(response);
  return 0;
 }
 file_size=response[9];
 file_size|=response[10]<<8;
 file_size|=response[11]<<16;
 file_size|=response[12]<<24;
 thumbnail_size=response[23];
 thumbnail_size|=response[24]<<8;
 thumbnail_size|=response[25]<<16;
 thumbnail_size|=response[26]<<24;
 if (thumbnail)
  file_size=thumbnail_size;
 free(response);


// download picture from camera to memory
 img=(struct Image *)malloc(sizeof(struct Image));
 if (img)
 {
  img->image_info_size=0;
  img->image_info=0;
  strcpy(img->image_type,"jpeg");
  img->image_size=file_size;
  img->image=(void *)malloc(img->image_size);
  if (!img->image)
  {
   free(img);
   img=0;
  }
  else
  {
// Download picture
   cmdlen=8;
   cmdseq=(unsigned char *)malloc(cmdlen);
   if (!cmdseq)
   {
    free(img->image);
    free(img);
    return 0;
   }
   ptr=img->image;
   cmdseq[0]=0x40;
   cmdseq[1]=(cmdlen-3)>>8;
   cmdseq[2]=(cmdlen-3)&0xff;
//   cmdseq[3]=0x07; // get picture
   cmdseq[3]=cmd; // get thumbnail/picture
   cmdseq[4]=0x00; // initiate transfer
   cmdseq[5]=pic_num&0xff;
   cmdseq[6]=pic_num>>8;
   set_checksum(cmdseq,cmdlen);
   sendseq(fd,cmdlen,cmdseq);
   free(cmdseq);
   while (not_done)
   {
    response=get_seq(fd,&cmdlen); 
    if (!response)
    {
     free(img->image);
     free(img);
     return 0;
    }
//    if (response[3]!=0x87)
    if (response[3]!=(0x80|cmd))
    {
     free(img->image);
     free(img);
     free(response);
     return 0;
    }
    if (response[4]==0x02)
    {
     not_done=0;
    }
    sequence_num=response[5];
    memcpy(ptr,response+6,cmdlen-7);
    ptr=ptr+(cmdlen-7);

    free(response);
    cmdlen=7;
    cmdseq=(unsigned char *)malloc(cmdlen);
    if (!cmdseq)
    {
     free(img->image);
     free(img);
     return 0;
    }
    cmdseq[0]=0x40;
    cmdseq[1]=(cmdlen-3)>>8; 
    cmdseq[2]=(cmdlen-3)&0xff;
//    cmdseq[3]=0x07; // get picture
    cmdseq[3]=cmd; // get thumbnail/picture
    if (not_done) 
     cmdseq[4]=0x01; // continue transfer
    else
     cmdseq[4]=0x02; // terminate transfer
    cmdseq[5]=sequence_num;
    set_checksum(cmdseq,cmdlen);
    sendseq(fd,cmdlen,cmdseq);
    free(cmdseq);
   }


  }
 }
 close_camera(fd);

 return img;
}

//GdkImlibImage *polaroid_pdc700_get_preview()
struct Image *polaroid_pdc700_get_preview()
// 'preview' of what is currently visible to the camera
{
//printf("get_preview\n");
//fflush(stdout);
 return 0;
}

int polaroid_pdc700_delete_image(int pic_num)
// 1 is successfull
{
//printf("delete_image\n");
//fflush(stdout);
 return 0;
}

int polaroid_pdc700_take_picture()
{
// 'takes picture' and returns picture number
//printf("take_picture\n");
 return 0;
}

int polaroid_pdc700_number_of_pictures()
// queries camera for number of pictures
{
 int fd;
 int num;
 unsigned char *cmdseq=0;
 unsigned int cmdlen=0;
 unsigned char *response;
 int loop;
 unsigned int num_pics_free;
 unsigned int num_pics=0;

//printf("number_of_pictures\n");

 fd=init_camera();
 if (fd<0)
  return 0;

// retrieve information from camera
 cmdlen=5;
 cmdseq=(unsigned char *)malloc(cmdlen);
 if (!cmdseq)
  return 0;
 cmdseq[0]=0x40;
 cmdseq[1]=(cmdlen-3)>>8;
 cmdseq[2]=(cmdlen-3)&0xff;
 cmdseq[3]=0x02; // get camera info
 set_checksum(cmdseq,cmdlen);
 sendseq(fd,cmdlen,cmdseq);
 free(cmdseq);
 response=get_seq(fd,&cmdlen);
 if (!response)
  return 0;
 if (response[3]!=0x82)
 {
  free(response);
  return 0;
 }
 if (response[4]!=0x01)
 {
  free(response);
  return 0;
 }

 num_pics=response[21];
 num_pics|=response[22]<<8;
 num_pics_free=response[23];
 num_pics_free|=response[24]<<8;
// printf("%d pictures are currently available\n",num_pics);
// printf("About %d pictures are left to take\n",num_pics_free);
// printf("\n");

 close_camera(fd);

 return num_pics;
}

int polaroid_pdc700_configure()
// configure camera, produce dialogs, etc
// 1 successfull
{
//printf("configure\n");
//fflush(stdout);
 return 0;
}

char *polaroid_pdc700_summary()
// returns current camera status
{
//printf("summary\n");
 return summary;
}

char *polaroid_pdc700_description()
// library information
{
//printf("description\n");
//fflush(stdout);
 return description;
}

struct _Camera polaroid_pdc700=
{
 polaroid_pdc700_initialize,
 polaroid_pdc700_get_picture,
 polaroid_pdc700_get_preview,
 polaroid_pdc700_delete_image,
 polaroid_pdc700_take_picture,
 polaroid_pdc700_number_of_pictures,
 polaroid_pdc700_configure,
 polaroid_pdc700_summary,
 polaroid_pdc700_description,
 NULL
};
