#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include "dc21x.h"

char *zoomstr[]={"58mm","51mm","41mm","34mm","29mm","Macro"};
char *flashstr[]={"Auto","On","Off","Auto Red Eye","On w/Red Eye"};
char *qualstr[]={"","Best","Better","Good"};

/* printf to stderr */
void eprintf(char *fmt, ...)
{
    char msg[132];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    fprintf(stderr,msg);
    va_end(args);
}

void print_info( int type )
{
 char filename[13];
 filename[13]='\0';

 switch (type) {
  case CAM_INFO:
   printf("Camera status:\n");
   printf("-----------------\n");
   printf("Camera ID: \t\t\t%d\n",status.camera_type_id);
   printf("Firmware version: \t\t%d.%d\n",status.firmware_major,status.firmware_minor);
   printf("Battery: \t\t\t%s\n", status.battery ? "Low" : "OK");
   printf("AC Power: \t\t\t%s\n",status.acstatus ? "present" : "absent");
   printf("Pictures in camera: \t\t%d\n",status.num_pictures);
   printf("Total pictures taken: \t\t%d\n",status.totalPicturesTaken);
   printf("Total flashes: \t\t\t%d\n",status.totalFlashesFired);
  break;
  case PIC_INFO:
   printf("Picture Information:\n");
   printf("--------------------\n");
   printf("Resolution: \t\t%s\n",picinfo.resolution ? "High" : "Standard");
   printf("Compression: \t\t%s\n",qualstr[(int)picinfo.compression]);
   printf("Size: \t\t\t%d\n",picinfo.fileSize);
   strncpy(filename,picinfo.fileName,12);
   printf("Filename: \t\t%12s\n",filename);
   // printf("Time: \t\t\t%s\n",picinfo.elapsedTime);
  break;
 }
}

void usage(void)
{
   printf ("Kodak DC21x Digital Camera utility by timecop [timecop@japan.co.jp]

	Usage:
	-s n		Use /dev/ttySn for camera port
	-q		Minimum messages to console [script use]
	
	-i [n]		Display Camera [ or picture n ] information
	-n		Display number of pictures in camera
	-d n		Delete image number n from camera
	-D		Delete all images from camera
	-t [filename]	Take a picture, save to filename
	-r [n] [fname]	Retrieve picture n, save as fname
	-R		Retrieve all pictures, save as imageNNN.jpg

	-z 1|0		Set resolution to [1:High,0:Low]
	-Q 1-3		Set JPEG quality. [1:Best,2:Better,3:Good]
	-Z 0..4 | 5	Set zoom. [0:58mm,1:51mm,2:41mm,3:34mm,4:29mm,5:Macro]
	-f 0-4		Flash setting. [0:Auto,1:Flash,2:NoFlash]
				       [3:RedEyeAuto,4:RedEyeFlash]
	
	Some options can be combined, for example:
	cam -s 2 -z 0 -q -f 1 -t webcam.jpg

");
}

int main(int argc, char *argv[])
{
 int i=0;
 char serdevice[]="/dev/ttyS0";

 fname="snap.jpg";
 quiet=FALSE;

 if (argc<=1) {
 usage();
 exit(0);
 }

 i = 1;
 while (i < argc ) {
  switch (argv[i][1]) {
   case 's':
    if (argv[i+1]) {
     serdevice[9]=argv[i+1][0];
     i++;
    } else { 
     eprintf("Port number not specified\n");
     exit(1);
    }
   break;
   case 'q':
    quiet=TRUE;
   break;
  }
  i++;
 }
 if (!quiet) printf("using %s\n",serdevice);
 serialdev=open_camera(serdevice);
 camera_init();

 i = 1;
 while (i < argc ) {
  switch (argv[i][1]) {
   int npic;
   int temp;

   case 'n':
    get_camera_status();
    printf("%d picture%sin camera.\n",status.num_pictures,status.num_pictures-1 ? "s " : " ");
   break;

   case 'z':
    if (argv[i+1]) {
      int tmp;
      tmp=atoi(argv[i+1]);
      tmp = tmp % 2;
      if (!quiet) printf("Setting resolution to %s\n",tmp ? "High" : "Low");
      send_command(DC_SET_RESOLUTION,tmp,0x00,0x00,0x00);
      command_complete();
      i++;
    } else { 
      eprintf("ERROR: Resolution not specified.\n");
      exit(1);
    }
   break;
   case 'Z':
    if (argv[i+1]) {
      int tmp;
      tmp=atoi(argv[i+1]);
      tmp = tmp % 6;
      if (!quiet) printf("Setting Zoom position to %s\n",zoomstr[tmp]);
      send_command(DC_SET_ZOOM,tmp,0x00,0x00,0x00);
      command_complete();
      i++;
    } else { 
      eprintf("ERROR: Zoom mode not specified.\n");
      exit(1);
    }
   break;
   case 'f':
    if (argv[i+1]) {
      int tmp;
      tmp=atoi(argv[i+1]);
      tmp = tmp % 5;
      if (!quiet) printf("Setting Flash mode to %s\n",flashstr[tmp]);
      send_command(DC_SET_FLASH,tmp,0x00,0x00,0x00);
      command_complete();
      i++;
    } else { 
      eprintf("ERROR: Flash mode not specified.\n");
      exit(1);
    }
   break;
   case 'Q':
    if (argv[i+1]) {
      int tmp;
      tmp=atoi(argv[i+1]);
      if (tmp<=0) tmp=1;
      tmp = tmp % 4;
      if (!quiet) printf("Setting Picture quality to %s\n",qualstr[tmp]);
      send_command(DC_SET_QUALITY,tmp,0x00,0x00,0x00);
      command_complete();
      i++;
    } else { 
      eprintf("ERROR: Picture quality not specified.\n");
      exit(1);
    }
   break;


   
   case 'd':
    if (argv[i+1]) {
     pic_ops(DELE_PIC,atoi(argv[i+1]));
     if (!quiet) printf("Deleted image %i from camera.\n",atoi(argv[i+1]));
     i++;
    } else {
      eprintf("ERROR: Image number not specified.\n");
      exit(1);
    }
   break;
   case 'D':
    get_camera_status();
    npic=status.num_pictures;
    for (temp=1;temp<=npic;temp++) {
     pic_ops(DELE_PIC,1);
     if (!quiet) printf("Deleted image %i from camera.\n", temp);
    }
   break;	    
   case 'i':
    get_camera_status();
    if (argv[i+1] && argv[i+1][0]!='-') {
      int tmp=atoi(argv[i+1]);
      if ( tmp <= status.num_pictures && tmp > 0 ) {
	get_picture_info( tmp );
	print_info(PIC_INFO);
      } else {
	eprintf("ERROR: Illegal picture number.\n");
	exit(1);
      }
    i++;
    } else {
      print_info(CAM_INFO);
    }
   break;

   case 't':
    if (argv[i+1]) {
     fname=argv[i+1];
     i++;
    } else fname="snap.jpg";
    if (!quiet) printf("Taking picture to %s.\n",fname);
    npic=pic_ops(TAKE_PIC,0);
    get_picture(npic);
    pic_ops(DELE_PIC,npic);
   break;
   case 'r':
    if (argv[i+1]) {
      npic=atoi(argv[i+1]);
      if (argv[i+2]) {
	i++;
        fname=argv[i+2];
	i++;
      } else {
	fname=malloc(256);
	sprintf(fname,"image%03d.jpg",atoi(argv[i+1]));
      }
    i++;
    } else {
      eprintf("Specify at least picture number.\n");
      exit(1);
    }
    if (!quiet) printf("Saving picture %d to %s.\n",npic,fname);
    get_picture(npic);
    free(fname);
   break;
 
   case 'R':
    get_camera_status();
    npic=status.num_pictures;
    fname=malloc(256);

    if (!quiet) printf("%d picture%sin camera.\n",status.num_pictures,status.num_pictures-1 ? "s " : " ");
    for (temp=1;temp<=npic;temp++) {
      sprintf(fname,"image%03d.jpg",temp);
      if (!quiet) printf("Saving picture %d to %s.\n",temp,fname);
      get_picture(temp);
    }
    free(fname);
   break;

   case 'q':
   break;
   case 's':
   i++;
   break;

   default:
    printf("Unknown option %s\n",argv[i]);
    usage();
    exit(1);
   break;
  
  }
  i++;
 }
 
 exit (0);
}

