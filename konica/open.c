#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * getKeyword - get value for a keyword/variable, from                 *
 *              the first successful test:                             *
 *                                                                     *
 *            1.  Look for an environment variable of the              *
 *                form "QM100_varname"                                 *
 *            2.  Look in ~/.gphoto/konicarc for a line                *
 *                beginning with varname                               *
 *            3.  default value provided by caller.                    *
 *                                                                     *
 *---------------------------------------------------------------------*/
char *qm100_getKeyword(char *key, char *dflt)
{
   char        *sp=NULL;
   static char  buf[256];
   QM100_CONFIGDATA *cp = & qm100_configData;
   sprintf(buf, "QM100_%s", key);
   sp = getenv(buf);
   if (!sp)
      {
      if (strcasecmp(key, "Speed") == 0)
         sp = cp->speed;
      else if (strcasecmp(key, "Pacing") == 0)
         sp = cp->pacing;
      else if (strcasecmp(key, "Camera") == 0)
         sp = cp->device;
      else if (strcasecmp(key, "Trace") == 0)
         sp = cp->tracefile;
      else if (strcasecmp(key, "Trace_Bytes") == 0)
         sp = cp->tracebytes;
      else if (strcasecmp(key, "Quality") == 0)
         sp = cp->quality;
      else if (strcasecmp(key, "Focus") == 0)
         sp = cp->focus;
      else if (strcasecmp(key, "Flash") == 0)
         sp = cp->flash;
      else if (strcasecmp(key, "AutoOff") == 0)
         sp = cp->autooff;
      else if (strcasecmp(key, "Timer") == 0)
         sp = cp->timer;
      else if (strcasecmp(key, "RedEye") == 0)
         sp = cp->redeye;
      else
         sp = dflt;
      }
   return sp;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * setTrace - set tracing options from environment and/or              *
 *            ~/.gphoto/konicarc:                                      *
 *                                                                     *
 *            Trace - specifies name of file, or "Off".                *
 *                    "On" is synonym for ~/.gphoto/konica.trace       *
 *            Trace_Bytes - 'On' indicates that low-level byte         *
 *                    trace should be included.                        *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_setTrace(void)
{
   char *fname;
   char tname[128];
   fname = qm100_getKeyword("TRACE", "off");
   if (!qm100_trace && fname &&
       strcasecmp(fname, "off") != 0  &&
       strcasecmp(fname, "none") != 0)
      {
      if (strcasecmp(fname, "on") == 0)
         fname = "konica.trace";
      if (*fname != '/' && *fname != '.')
         sprintf(tname, "%s/.gphoto/%s", getenv("HOME"),  fname);
      else
         strcpy(tname, fname);
      qm100_trace = fopen(tname, "w");
      if (!qm100_trace)
         {
         sprintf(tname, "./%s", fname);
         qm100_trace = fopen(tname, "w");
         }
      }
   fname = qm100_getKeyword("TRACE_BYTES", "off");
   if (qm100_trace && fname && strcasecmp(fname, "off") != 0)
      qm100_showBytes = 1;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * open  - prepare serial port for use, and send initialization        *
 *         packets to camera.                                          *
 *                                                                     *
 *---------------------------------------------------------------------*/
int qm100_open(const char *devname)
{
  int serialdev;
  qm100_packet_block packet;
  char cmd[]=QM100_INIT;
  serialdev = open(devname, O_RDWR | O_NOCTTY);
  if (serialdev <= 0)
     {
     char tmsg[100];
     sprintf(tmsg, "Unable to open serial device %s", devname);
     qm100_error(serialdev, tmsg, errno);
     }
  if (tcgetattr(serialdev, &oldt) < 0)
     qm100_error(serialdev, "Unable to get serial device attributes", errno);
  memcpy((char *)&newt,(char *)&oldt, sizeof(struct termios));
  newt.c_cflag |= CS8 | HUPCL;
  newt.c_iflag &= ~(IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR);
  newt.c_iflag &= ~(IGNCR|ICRNL|IXON|IXOFF|IXANY|IMAXBEL);
  newt.c_oflag &= ~(OPOST);
  newt.c_lflag &= ~(ISIG|ICANON);
  newt.c_cc[VMIN] = 1;
  newt.c_cc[VTIME] = 0;
  cfsetospeed(&newt, B9600);
  cfsetispeed(&newt, B9600);
  if (tcsetattr(serialdev, TCSANOW, &newt) < 0)
     qm100_error(serialdev, "Unable to set serial device attributes", errno);
  qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "Open");
  qm100_setSpeed(serialdev, qm100_transmitSpeed);
  return serialdev;
}
