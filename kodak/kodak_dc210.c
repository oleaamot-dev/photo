#include <config.h>
#include "../src/gphoto.h"
#include "../src/util.h"


#include "dc210.h"
#include "kodak_dc210.h"

/* glib is used for byte swapping and endian checking */
#include <glib.h>
#include <gdk_imlib.h>
#include <gdk/gdk.h>

#include <errno.h>

#ifdef HAVE_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SELECT_H */

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#    ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#    else HAVE_SYS_TIME_H
#    include <time.h>
#    endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

int kodak_dc210_send_command ( char serialdev, char command, char arg1, char arg2, char arg3, char arg4)
{
  unsigned char ack;
  int success = TRUE;

  /* try to write 8 bytes in sequence to the camera */
  success = success && kodak_dc210_write_byte ( serialdev, command );
  success = success && kodak_dc210_write_byte ( serialdev, 0x00 );
  success = success && kodak_dc210_write_byte ( serialdev, arg1 );
  success = success && kodak_dc210_write_byte ( serialdev, arg2 );
  success = success && kodak_dc210_write_byte ( serialdev, arg3 );
  success = success && kodak_dc210_write_byte ( serialdev, arg4 );
  success = success && kodak_dc210_write_byte ( serialdev, 0x00 );
  success = success && kodak_dc210_write_byte ( serialdev, 0x1A );

  /* if the command was sent successfully to the camera, continue */
  if (success)
  {
    /* read ack from camera */
    success = kodak_dc210_read( serialdev, &ack, 1 );

    if (success)
    {
      /* make sure the ack is okay */
      if (ack != DC_COMMAND_ACK)
      {
	fprintf(stderr,"kodak_dc210_send_command - bad ack from camera\n");
	success = FALSE;
      }
    }
    else
    {
      fprintf(stderr,"kodak_dc210_send_command - failed to read ack from camera\n");
      success = FALSE;
    }
  }
  else
  {
    fprintf(stderr,"kodak_dc210_send_command - failed to send command to camera\n");
    success = FALSE;
  }

  return(success);
}

int kodak_dc210_set_port_speed(int serialdev,int speed)
{
  int success = 1;
  int arg1, arg2;
  struct termios newt, oldt;

  /* get the termios structure */
  if (tcgetattr(serialdev, &oldt) < 0) 
  {
    success = 0;
    dialog_error("tcgetattr");
  }
  else
  {
    /* make a copy of the old termios structure */
    memcpy((char *)&newt,(char *)&oldt, sizeof(struct termios));

    if (speed == 9600)
    {
      arg1 = 0x96;
      arg2 = 0x00;
      cfsetospeed(&newt, B9600);
      cfsetispeed(&newt, B9600);
    }
    else if (speed == 19200)
    {
      arg1 = 0x19;
      arg2 = 0x20;
      cfsetospeed(&newt, B19200);
      cfsetispeed(&newt, B19200);
    }
    else if (speed == 38400)
    {
      arg1 = 0x38;
      arg2 = 0x40;
      cfsetospeed(&newt, B38400);
      cfsetispeed(&newt, B38400);
    }
    else if (speed == 57600)
    {
      arg1 = 0x57;
      arg2 = 0x60;
      cfsetospeed(&newt, B57600);
      cfsetispeed(&newt, B57600);
    }
    else if (speed == 115200)
    {
      arg1 = 0x11;
      arg2 = 0x52;
      cfsetospeed(&newt, B115200);
      cfsetispeed(&newt, B115200);
    }
    else
    {
      success = 0;
      fprintf(stderr,"speed not supported %d",speed);
    }

    if (success)
    {
      success = kodak_dc210_send_command(serialdev,DC_SET_SPEED,arg1,arg2,0x00,0x00);

      if (success)
      { 
	if (tcsetattr(serialdev, TCSANOW, &newt) < 0) 
        {
	  dialog_error("Serial speed change problem");
	  success = 0;
        }
      }
    }
  }

  return(success);
}

int kodak_dc210_command_complete (int serialdev)
{
  int status = DC_BUSY;
  int success = TRUE;

  /* Wait for the camera to be ready. */
  do
  {
    success = kodak_dc210_read(serialdev,(char *)&status,1);
  }
  while (success && (status == DC_BUSY));

  if (success)
  {
    if (status != DC_COMMAND_COMPLETE)
    {
      if (status == DC_ILLEGAL_PACKET)
      {
	fprintf(stderr,"kodak_dc210_command_complete - illegal packet received from host\n");
      }
      else
      {
	fprintf(stderr,"kodak_dc210_command_complete - command not completed\n");
      }

      /* status was not command complete - raise an error */
      success = FALSE;
    }
  }
  else
  {
    fprintf(stderr,"kodak_dc210_command_complete - failed to read status byte from camera\n");
    success = FALSE;
  }

  return(success);
}

int kodak_dc210_read ( int serialdev, unsigned char *buf, int nbytes )
{
  int ret;
  int numRead = 0;
  int n;
  int len;
  fd_set readfds;
  struct timeval timeout;

  while (numRead < nbytes)
  {
    /* set parameters for select call */
    n = serialdev + 1;
    FD_ZERO(&readfds);
    FD_SET(serialdev, &readfds);

    /* declare a timeout of 1.500sec */
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    ret = select(n, &readfds, NULL, NULL, &timeout);

    /* if the select failed or timed out, hanled it */
    if(ret <= 0)
    {
      if (ret<0)
      {
	/* if the system call was interupted, retry */
	if (errno == EINTR) 
	  continue;
        else
	  perror("select");
      }
      else
	fprintf(stderr, "kodak_dc210_read: read timed out\n");

      return FALSE;
    }
    else
    {
      if (FD_ISSET(serialdev, &readfds))
      {
	len = read(serialdev, buf + numRead,
		   nbytes - numRead);

        if (len < 0)
        {
	  if (errno == EINTR) 
	    continue;
          else
          { 
	    fprintf(stderr, "kodak_dc210_read: read failed on a ready file handle\n");
	    perror("read");
	    return FALSE; 
          }
        }
        else
        {
	  numRead += len;
        }
      }
      else
	return FALSE;
    }
  }

  return TRUE;
}

int kodak_dc210_write_byte ( int serialdev, char b )
{
/*
This function tries to write the supplied byte to the serial device.

Args
  int serieldev - a file handle
  char b - the byte to write
Returns
  success - 1 if the command completed, 0 otherwise
*/
  int numwrite;

  /* write the bite to the serial device */
  numwrite = write(serialdev, &b, 1);

  /* check to make sure the byte was written */
  if (numwrite != 1)
  {
    fprintf(stderr,"failed to write byte\n");
    return(0);
  }
  else
  {
    return(1);
  }
}

int kodak_dc210_read_packet ( int serialdev, char *packet, int length)
{
  int numread = 0;
  int success = TRUE;
  int r = 0;
  unsigned char sent_checksum;
  unsigned char computed_checksum;
  unsigned char control_byte;

  /* Read the control byte from the camera - and packet coming from
     the camera must ALWAYS be 0x01 according to Kodak Host Protocol. */
  if (kodak_dc210_read(serialdev,&control_byte,1))
  {
    if (control_byte == PKT_CTRL_RECV)
    {
      if (kodak_dc210_read(serialdev,packet,length))
      {
	/* read the checksum from the camera */
	if (kodak_dc210_read(serialdev, &sent_checksum, 1))
	{
	  computed_checksum = kodak_dc210_checksum(packet,length);

	  if (sent_checksum != computed_checksum)
	  {
	    fprintf(stderr,"kodak_dc210_read_packet: bad checksum %d != %d\n",computed_checksum,sent_checksum);
	    kodak_dc210_write_byte(serialdev,DC_ILLEGAL_PACKET);
	    success = FALSE;
	  }
	  else
	  {
	    kodak_dc210_write_byte(serialdev,DC_CORRECT_PACKET);
	    success = TRUE;
          }
	}
	else
	{
	  fprintf(stderr,"kodak_dc210_read_packet: failed to read checksum byte from camera\n");
	  success = FALSE;
	}
      }
      else
      {
	fprintf(stderr,"kodak_dc210_read_packet: failed to read paket from camera\n");
	success = FALSE;
      }
    }
    else
    {
      fprintf(stderr,"kodak_dc210_read_packet - packet control byte invalid %x\n",control_byte);
      success = FALSE;
    }
  }
  else
  {
    fprintf(stderr,"kodak_dc210_read_packet - failed to read control byte from camera\n");
    success = FALSE;
  }

  return success;
}

unsigned char kodak_dc210_checksum(char *packet,int length)
{
/* Computes the *primitave* kodak checksum using XOR - yuck. */
  unsigned char chksum = 0;
  int i;

  for (i = 0 ; i < length ; i++)
  {
    chksum ^= packet[i];
  }

  return chksum;
}

int kodak_dc210_initialize() 
{
    return(1);
}

int kodak_dc210_open_camera (const char *devname)
{
  int serialdev;

  struct termios newt, oldt;

  serialdev = open(devname, O_RDWR|O_NOCTTY|O_NONBLOCK);

  if (serialdev < 0) 
    dialog_error("Cannot open device");

  if (tcgetattr(serialdev, &oldt) < 0) 
    dialog_error("tcgetattr");

  memcpy((char *)&newt,(char *)&oldt, sizeof(struct termios));

  /* need the device to be raw. 8 bits no parity on 9600 baud to start.  */
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
  if (tcsetattr(serialdev, TCSANOW, &newt) < 0) 
    dialog_error("Serial speed change problem");

  kodak_dc210_send_command(serialdev,DC_INITIALIZE,0x00,0x00,0x00,0x00);
  kodak_dc210_command_complete(serialdev);
  kodak_dc210_set_port_speed(serialdev,115200);

  return serialdev;
}

int kodak_dc210_close_camera (int serialdev) 
{

  kodak_dc210_set_port_speed(serialdev,9600);
  close(serialdev);
  return(1);
}

int kodak_dc210_number_of_pictures () 
{
  int serialdev;
  int numPics = 0;
  struct kodak_dc210_status status;

  if (serialdev = kodak_dc210_open_camera (serial_port))
  {
    kodak_dc210_get_camera_status (serialdev, &status);
    kodak_dc210_close_camera ( serialdev );

    numPics = status.num_pictures;
  }

  return(numPics);
}

int kodak_dc210_take_picture() 
{
  int serialdev;

  if (serialdev = kodak_dc210_open_camera (serial_port))
  {
    kodak_dc210_send_command( serialdev,DC_TAKE_PICTURE,0x00,0x00,0x00,0x00);
    kodak_dc210_command_complete( serialdev );
    kodak_dc210_close_camera ( serialdev );
  }

  return( kodak_dc210_number_of_pictures() );
}

int kodak_dc210_get_picture_info (int serialdev,
                                  int picNum,
                                  struct kodak_dc210_picture_info *picInfo)
{
  unsigned char packet[256];

  picNum--;

  /* send picture info command, receive data, send ack */
  kodak_dc210_send_command( serialdev,DC_PICTURE_INFO,0x00,(char)picNum,0x00,0x00);
  kodak_dc210_read_packet( serialdev,packet,256);
  kodak_dc210_command_complete( serialdev );

  picInfo->resolution    = packet[3];
  picInfo->compression   = packet[4];
  picInfo->pictureNumber = packet[6] * 0x100 + packet[7];

  picInfo->fileSize      = (guint32)packet[8]  * 0x1000000 + 
			   (guint32)packet[9]  * 0x10000 + 
                           (guint32)packet[10] * 0x100 + 
                           (guint32)packet[11];

  picInfo->elapsedTime   = (guint32)packet[12] * 0x1000000 + 
			   (guint32)packet[13] * 0x10000 + 
		           (guint32)packet[14] * 0x100 + 
			   (guint32)packet[15];

  strncpy(picInfo->fileName,packet + 32,12);
}

int kodak_dc210_get_camera_status (int serialdev,
                                   struct kodak_dc210_status *status)
{
  unsigned char packet[256];
  int success = TRUE;

  if (kodak_dc210_send_command( serialdev,DC_STATUS,0x00,0x00,0x00,0x00))
  {
    if (kodak_dc210_read_packet( serialdev,packet,256))
    {
      kodak_dc210_command_complete( serialdev );

      memset((char*)status,0,sizeof(struct kodak_dc210_status));

      status->camera_type_id        = packet[1];
      status->firmware_major        = packet[2];
      status->firmware_minor        = packet[3];
      status->batteryStatusId       = packet[8];
      status->acStatusId            = packet[9];

      /* seconds since unix epoc */
      status->time = CAMERA_EPOC + (
				packet[12] * 0x1000000 + 
				packet[13] * 0x10000 + 
				packet[14] * 0x100 + 
				packet[15]) / 2;

      status->zoomMode              = packet[15];
      status->flash_charged         = packet[17];
      status->compression_mode_id   = packet[18];
      status->flash_mode            = packet[19];
      status->exposure_compensation = packet[20];
      status->picture_size          = packet[21];
      status->file_Type             = packet[22];
      status->totalPicturesTaken    = packet[25] * 0x100 + packet[26];
      status->totalFlashesFired     = packet[27] * 0x100 + packet[28];
      status->num_pictures          = packet[56] * 0x100 + packet[57];
      strncpy(status->camera_ident,packet + 90,32); 
    }
    else
    {
      fprintf(stderr,"kodak_dc210_get_camera_status: send command failed\n");
      success = FALSE;
    }
  }
  else
  {
    fprintf(stderr,"kodak_dc210_get_camera_status: send command failed\n");
    success = FALSE;
  }

  return success;
}


struct Image *kodak_dc210_get_thumbnail (int serialdev, int picNum) 
{
  struct Image *im = NULL;
  int i,j;
  int numRead = 0;
  int success = TRUE;
  int fileSize = 20736;
  int blockSize = 1024;
  char *picData;
  char *imData;
  char bmpHeader[] = {
        0x42, 0x4d, 0x36, 0x24, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x48, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  /* the protocol expects 0..n-1, but this function is supplied 1..n */
  picNum--;

  /* allocate space for the thumbnail data */
  picData = (char *)malloc(fileSize + blockSize);

  /* allocate space for the image data */
  imData = (char *)malloc(fileSize + 54);

  update_progress(0);

  if (kodak_dc210_send_command(serialdev,DC_PICTURE_THUMBNAIL,0x00,(char)picNum,0x01,0x00))
  {
    while (success && (numRead < fileSize))
    {
      if (kodak_dc210_read_packet( serialdev,picData+numRead,blockSize))
      {
	numRead += blockSize;

	/* on the last block numRead will be > fileSize, so don't report it
	   in these situations */
	if ((numRead <= fileSize)&&(fileSize))
	  update_progress( 100 * numRead / fileSize);
      }
      else
      {
	fprintf(stderr,"kodak_dc210_get_thumbnail - bad packet read from camera\n");
	success = FALSE;
      }
    }

    /* get to see if the thumbnail was retreived */
    if (success)
    {
      kodak_dc210_command_complete(serialdev);
      update_progress(100);

      /* allocate memory for image structure */
      if ( (im = (struct Image *)malloc ( sizeof(struct Image) )) == NULL ) 
      {
	dialog_error("Could not allocate memory for image structure.");
	return ( NULL );
      }
    
      memcpy(imData,bmpHeader, sizeof(bmpHeader));

      /* reverse the thumbnail data */
      /* not only is the data reversed but the image is flipped
       * left to right
       */
      for (i = 0; i < 72; i++) {
         for (j = 0; j < 96; j++) {
            imData[i*96*3+j*3+54] = picData[(71-i)*96*3+j*3+2];
            imData[i*96*3+j*3+54+1] = picData[(71-i)*96*3+j*3+1];
            imData[i*96*3+j*3+54+2] = picData[(71-i)*96*3+j*3];
            }
         }

      strcpy ( im->image_type, "bmp" );
      im->image_info = NULL;
      im->image_info_size = 0;
      im->image_size = fileSize+54;
      im->image = imData;
    }
  }
  else
  {
    fprintf(stderr,"kodak_dc210_get_thumbnail: failed to get thumbnail command to camera\n");
    success = FALSE;
  }

  free(picData);

  return im;
}



struct Image *kodak_dc210_get_picture (int picNum, int thumbnail) 
{
  struct Image *im = NULL;
  int blockSize;
  int fileSize;
  int numRead;
  int serialdev;
  struct kodak_dc210_picture_info picInfo;
  char *picData;

  if (serialdev = kodak_dc210_open_camera (serial_port))
  {
    if (thumbnail)
    {
      im = kodak_dc210_get_thumbnail(serialdev,picNum);
    }
    else
    {
      /* get information (size, name, etc) about the picture */
      kodak_dc210_get_picture_info (serialdev, picNum, &picInfo);

      /* DC210 addresses pictures 0..n-1 instead of 1..n */
      picNum--;

      /* send the command to start transfering the picture */
      kodak_dc210_send_command( serialdev, DC_PICTURE_DOWNLOAD, 0x00, (char)picNum, 0x00, 0x00);

      fileSize = picInfo.fileSize;
      blockSize = 1024;
      picData = (char *)malloc(fileSize + blockSize);
      numRead = 0;

      update_progress(0);
      while (numRead < fileSize)
      {
	kodak_dc210_read_packet( serialdev,picData+numRead,blockSize);
	numRead += blockSize;

	if ((numRead <= fileSize)&&(fileSize))
	  update_progress( 100 * numRead / fileSize);
      }
      fprintf(stderr,"%d/%d\n",numRead,fileSize);
      kodak_dc210_command_complete( serialdev );
      update_progress(100);

      /* allocate memory for image structure */
      if ( (im = (struct Image *)malloc ( sizeof(struct Image) )) == NULL ) 
      {
	dialog_error("Could not allocate memory for image structure.");
	return ( NULL );
      }
      
      strcpy ( im->image_type, "jpg" );
      im->image_info = NULL;
      im->image_info_size = 0;
      im->image_size = fileSize;
      im->image = picData;

    }

    /* close the camera */
    kodak_dc210_close_camera ( serialdev );
  }

  return(im);
}

int kodak_dc210_delete_picture (int picNum) 
{
  int serialdev;

  if (serialdev = kodak_dc210_open_camera (serial_port))
  {
    picNum--;

    kodak_dc210_send_command( serialdev,DC_ERASE_IMAGE_IN_CARD,0x00,(char)picNum,0x00,0x00);
    kodak_dc210_command_complete( serialdev );
    kodak_dc210_close_camera ( serialdev );
  }

  return (1);
}


struct Image *kodak_dc210_get_preview () 
{
  int numPicBefore;
  int numPicAfter;
  struct Image *im = NULL;

  /* find out how many pictures are in the camera so we can
     make sure a picture was taken later */
  numPicBefore = kodak_dc210_number_of_pictures();

  /* take a picture -- it returns the picture number taken */
  numPicAfter = kodak_dc210_take_picture();

  /* if a picture was taken then get the picture from the camera and
     then delete it */
  if (numPicBefore + 1 == numPicAfter)
  {
    im = kodak_dc210_get_picture (numPicAfter,0);
    kodak_dc210_delete_picture (numPicAfter);
  }

  return(im);
}


int kodak_dc210_configure () 
{

  dialog_error("Configuration is not supported through gPhoto, use the menus on the camera.");
  return(0);

}

char *kodak_dc210_summary() 
{
  static char summary_string[2048] = "";
  char buff[1024];
  int serialdev = kodak_dc210_open_camera (serial_port);
  struct kodak_dc210_status status;

  if (serialdev)
  {
    if (kodak_dc210_get_camera_status (serialdev, &status))
    {
      strcpy(summary_string,"Kodak DC210\n");

      snprintf(buff,1024,"Camera Identification: %s\n",status.camera_ident);
      strcat(summary_string,buff);

      snprintf(buff,1024,"Camera Type: %d\n",status.camera_type_id);
      strcat(summary_string,buff);

      snprintf(buff,1024,"Firmware: %d.%d\n",status.firmware_major,status.firmware_minor);
      strcat(summary_string,buff);

      snprintf(buff,1024,"Battery Status: %d\n",status.batteryStatusId);
      strcat(summary_string,buff);

      snprintf(buff,1024,"AC Status: %d\n",status.acStatusId);
      strcat(summary_string,buff);

      strftime(buff,1024,"Time: %a, %d %b %y %T\n",localtime((time_t *)&status.time));
      strcat(summary_string,buff);

      fprintf(stderr,"step 4\n");
      snprintf(buff,1024,"Total Pictures Taken: %d\n",status.totalPicturesTaken);
      strcat(summary_string,buff);

      snprintf(buff,1024,"Total Flashes Fired: %d\n",status.totalFlashesFired);
      strcat(summary_string,buff);

      snprintf(buff,1024,"Pictures in Camera: %d\n",status.num_pictures);
      strcat(summary_string,buff);

    }
    kodak_dc210_close_camera ( serialdev );
  }

  return (summary_string);
}

char *kodak_dc210_description() 
{

  return(
"Kodak DC210 support
by Brian Hirt <bhirt@mobygames.com> 
http://www.mobygames.com");

}


/* define function pointers for the dc210 camera */
struct _Camera kodak_dc210 = {
		kodak_dc210_initialize,
		kodak_dc210_get_picture,
		kodak_dc210_get_preview,
		kodak_dc210_delete_picture,
		kodak_dc210_take_picture,
		kodak_dc210_number_of_pictures,
		kodak_dc210_configure,
		kodak_dc210_summary,
		kodak_dc210_description,
		NULL};
