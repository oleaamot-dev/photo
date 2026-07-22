#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * qm100.c - command line interface for Konica QM100 and               *
 *           HP C20/C30 cameras.  Useful mostly for testing            *
 *           the camera protocol and provides marginally               *
 *           better CLI support than the gphoto CLI for automating     *
 *           camera downloads.                                         *
 *                                                                     *
 *---------------------------------------------------------------------*/
void progress(void);
void usage(void);
static char cycle[] = "|/-\\";
static char  cameraName[128];
static int   qm100_killPic;
static int   qm100_format;
static int   qm100_getPic;
static int   qm100_getThumb;
void progress(void)
{
   static int dots = 0;
   fprintf(stderr, "\b%c", cycle[dots]);
   fflush(stderr);
   if (++dots >= sizeof(cycle)-1)
      dots=0;
}
void usage(void)
{
   printf("Konica QM-100 Digital Camera Utility "
          "Version %s Mod %s\n", QM100_VER,  QM100_MOD);
   printf("Usage:\tqm100 MODE [OPTION]... [FILENAME]\n");
   printf("\tMODES\n");
   printf("\t  -s\t\tdisplay the camera status\n");
   printf("\t  -a\t\tget all pictures\n");
   printf("\t  -t\t\tget all thumbnails\n");
   printf("\t  -g##\t\tget picture ## (needs filename)\n");
   printf("\t  -e##\t\terase picture ##\n");
   printf("\t  -i##\t\tdump information block for picture ##\n");
   printf("\t  -f\t\tformat the compact flash card\n");
   printf("\t  -d\t\tget date and time from camera\n");
   printf("\t  -D\t\tSet camera date and time from system clock\n");
   printf("\t  -T\t\tTake a picture\n");
   printf("\t  -I\t\tDump camera identification packet\n");
   printf("\n\tOPTIONS\n");
   printf("\t  -p\t\tSet the camera device/port (default %s)\n", cameraName);
   printf("\t  -b\t\tSet the speed/baud rate\n"
          "\t    \t\tChoose from 9600, 19200, 38400, 57600, or 115200\n"
          "\t    \t\t(Default %s)\n", DEFAULT_SPEED);
   printf("\t  -h\t\tdisplay this help\n");
   printf("\n\tplease supply ONE major mode (-s, -g, -t, -f, -e, -a)\n");
   printf("\n");
   printf("\tEnvironment variables:\n");
   printf("\t  QM100_CAMERA=dev  - set default device for camera\n");
   printf("\t  QM100_SPEED=nn    - set transmission speed\n");
   printf("\t  QM100_PACING=nn   - set I/O pacing delay, in millesconds\n");
   printf("\t                      (Default %s)\n", DEFAULT_PACING);
   printf("\t  QM100_TRACE=fname - trace camera activity to fname\n");
   printf("\t  QM100_TRACE_BYTES - trace low-level camera I/O\n");
}
int main(int argc, char *argv[])
{
   int   c;
   int   serialdev;
   int   infoPic=0;
   int   qm100_date=0;
   int   operation=0;
   qm100_packet_block packet;
   QM100_CAMERA_INFO  cinfo;
   /*------------------------------------------------------------------*
    *                                                                  *
    * Extract parameters from ~/.gphoto/konicarc,  and from the        *
    * command line.                                                    *
    *                                                                  *
    *------------------------------------------------------------------*/
   qm100_main = 1;
   qm100_readConfigData(&qm100_configData);
   strcpy(cameraName,qm100_getKeyword("CAMERA", DEFAULT_PORT));
   qm100_setTrace();
   while ((c=getopt(argc, argv, "e:fg:hstaIi:p:b:dDTvV?C")) != EOF)
      {
      switch (c)
         {
         case 'C':
            qm100_configureDialog();
            return 0;
         case 'b':
            {
            char tval[64];
            sprintf(tval, "QM100_SPEED=%s", optarg);
            putenv(tval);
            break;
            }
         case 'p':
            strcpy(cameraName, optarg);
            break;
         case 'i':
            operation=c;
            sscanf(optarg,"%d",&infoPic);
            break;
         case 'a':
            operation=c;
            qm100_getPic = -1;
            break;
         case 'T':
            operation=c;
            break;
         case 'I':
            operation=c;
            break;
         case 's':
            qm100_showStatus=1;
            operation=c;
            break;
         case 'e':
            sscanf(optarg,"%d",&qm100_killPic);
            operation=c;
            break;
         case 'f':
            qm100_format=1;
            operation=c;
            break;
         case 'd':
            qm100_date=1;
            operation=c;
            break;
         case 'D':
            qm100_date=2;
            operation=c;
            break;
         case 'g':
            sscanf(optarg,"%d",&qm100_getPic);
            operation=c;
            break;
         case 't':
            qm100_getThumb = 1;
            operation=c;
            break;
         default:
            usage();
            exit(0);
         }
      }
   /*------------------------------------------------------------------*
    *                                                                  *
    * Perform requested camera operations.                             *
    *                                                                  *
    *------------------------------------------------------------------*/
   if (operation)
      {
      qm100_setTransmitSpeed();
      serialdev = qm100_open(cameraName);
      qm100_getStatus(serialdev, &cinfo);
      switch (operation)
         {
         case 'T':
            qm100_takePic(serialdev);
            break;
         case 'f':
            qm100_formatCF(serialdev);
            break;
         case 'd':
            printf("Camera reports date as: %s\n", qm100_getDate(serialdev));
            break;
         case 'D':
            printf("Camera date set to: %s\n", qm100_setDate(serialdev));
            break;
         case 'g':
            qm100_getPic = qm100_getRealPicNum(serialdev, qm100_getPic);
            printf("Retrieving %s  ", argv[optind]);
            qm100_savePic(serialdev, argv[optind], qm100_getPic, progress);
            printf("\b - Done.\n");
            break;
         case 'i':
            qm100_getPicInfo(serialdev, infoPic, &packet);
            dump(stdout, "PicInfo", packet.packet, packet.packet_len);
            break;
         case 'a':
            {
            int i;
            char filename[40];
            for (i=1; i<=qm100_pictureCount; i++)
               {
               qm100_getPic = qm100_getRealPicNum(serialdev, i);
               if (qm100_getPic)
                  {
                  sprintf(filename, "pic%02d.jpg", qm100_getPic);
                  printf("Retrieving %s  ", filename);
                  fflush(stdout);
                  qm100_savePic(serialdev, filename, qm100_getPic, progress);
                  printf("\b - Done.\n");
                  }
               }
            }
            break;
         case 'e':
            qm100_killPic = qm100_getRealPicNum(serialdev, qm100_killPic);
            qm100_erasePic(serialdev, qm100_killPic);
            break;
         case 'I':
            {
            char cmd[]=QM100_GETID;
            qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "GetID");
            dump(stdout, "Camera ID", packet.packet, packet.packet_len);
            break;
            }
         case 's':
            printf("Camera:             %s\n", cinfo.name);
            printf("Product Id:         %-4.4s\n", cinfo.product);
            printf("Serial #:           %-10.10s\n", cinfo.serial);
            printf("Hardware Version:   %d.%d\n", cinfo.hwver, cinfo.hwmod);
            printf("Software Version:   %d.%d\n", cinfo.swver, cinfo.swmod);
            printf("Pictures in Memory: %d\n", cinfo.pictureCount);
            printf("Total Pictures:     %d\n", cinfo.totalCount);
            printf("Total Flashes:      %d\n", cinfo.strobeCount);
            printf("Date:               %d/%02d/%02d\n", cinfo.year+1900, cinfo.month, cinfo.day);
            printf("Time:               %02d:%02d:%02d\n", cinfo.hour, cinfo.min, cinfo.sec);
            break;
         case 't':
            {
            int i;
            char filename[40];
            for (i=1; i<=qm100_pictureCount; i++)
               {
               qm100_getThumb = qm100_getRealPicNum(serialdev, i);
               if (qm100_getThumb)
                  {
                  sprintf(filename, "thumb%02d.tif", qm100_getThumb);
                  printf("Retrieving %s  ", filename);
                  fflush(stdout);
                  qm100_saveThumb(serialdev, filename, qm100_getThumb, progress);
                  printf("\b - Done.\n");
                  }
               }
            break;
            }
         default:
            printf("qm100:  no MODE specified - try \"qm100 -h\"\n");
         }
      qm100_close(serialdev);
      }
   else
      usage();
   return 0;
}
