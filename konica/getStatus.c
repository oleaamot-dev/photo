#include "qm100.h"
#include <time.h>
/*---------------------------------------------------------------------*
 *                                                                     *
 * getDate -  retrieve date and time from camera                       *
 *                                                                     *
 *---------------------------------------------------------------------*/
char *qm100_getDate(int serialdev)
{
   static char fmtdate[64];
   qm100_packet_block packet;
   char cmd[] = QM100_GETTIME;
   QM100_PKT_TIME *pt = (QM100_PKT_TIME *) &(packet.packet);
   qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "GetTime");
   if (pt->year <60)
      pt->year += 100;
   sprintf(fmtdate, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
           pt->year+1900, pt->month, pt->day, pt->hour,
           pt->minute, pt->second);
   return fmtdate;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * setDate - set camera date and time from system clock.               *
 *                                                                     *
 *---------------------------------------------------------------------*/
char *qm100_setDate(int serialdev)
{
   qm100_packet_block packet;
   struct tm *tp;
   time_t    mytime;
   char cmd[] = QM100_SETTIME;
   mytime = time(NULL);
   tp = localtime(&mytime);
   if (tp->tm_year > 99)
      cmd[4] = tp->tm_year - 100;
   else
      cmd[4] = tp->tm_year;
   cmd[5] = 1 + tp->tm_mon;
   cmd[6] = tp->tm_mday;
   cmd[7] = tp->tm_hour;
   cmd[8] = tp->tm_min;
   cmd[9] = tp->tm_sec;
   qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "SetTime");
   return qm100_getDate(serialdev);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * getStatus - retrieve status packet from camera.  This               *
 *             contains date, time, # pictures in camera,              *
 *             total picture count, etc.  Optionally,                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
int qm100_getStatus(int serialdev, QM100_CAMERA_INFO *cip)
{
   qm100_packet_block packet;
   char cmd_getstatus[]=QM100_GETSTATUS;
   char cmd_getid[]=QM100_GETID;
   QM100_PKT_STATUS *pp = (QM100_PKT_STATUS *) packet.packet;
   QM100_PKT_ID *ip = (QM100_PKT_ID *) packet.packet;
   if (cip)
      {
      qm100_transmit(serialdev, cmd_getid, sizeof(cmd_getid), &packet, "GetId");
      strcpy(cip->name, ip->name);
      memcpy(cip->product, ip->product, sizeof(cip->product));
      memcpy(cip->serial, ip->serial, sizeof(cip->serial));
      cip->hwmod   = ip->hwmod;
      cip->hwver   = ip->hwver;
      cip->swmod   = ip->swmod;
      cip->swver   = ip->swver;
      }
   qm100_transmit(serialdev, cmd_getstatus, sizeof(cmd_getstatus), &packet, "GetStatus");
   qm100_pictureCount = pp->currentCount ;
   if (cip)
      {
      cip->pictureCount = pp->currentCount;
      cip->totalCount   = pp->totalCount;
      cip->strobeCount  = pp->strobeCount;
      cip->year         = pp->year;
      cip->month        = pp->month;
      cip->day          = pp->day;
      cip->hour         = pp->hour;
      cip->min          = pp->min;
      cip->sec          = pp->sec;
      }
   return qm100_pictureCount;
}
