#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * savePic - retrieve and save a single image                          *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_savePic(int serialdev, char *filename, int pic,
                   void (*progress)(void))
{
  int jpgfile;
  char success=1;
  char cmd[]=QM100_GETPIC;
  qm100_packet_block packet;
  cmd[5] = (pic >> 8) & 0xff;
  cmd[6] = (pic & 0xff);
  /*-------------------------------------------------------------------*
   *                                                                   *
   * Send the command, and get the first response packet.              *
   *                                                                   *
   *-------------------------------------------------------------------*/
  qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "GetPicture");
  if (packet.packet_len == 4)
      success = 0; /* No pic there! */
  else
    {
      jpgfile = open(filename, O_RDWR|O_CREAT|O_EXCL, 0700);
      write(jpgfile, packet.packet, packet.packet_len);
      /*---------------------------------------------------------------*
       *                                                               *
       * Continue to receive and write packets as long as the packet   *
       * indicates data is continued.                                  *
       *                                                               *
       *---------------------------------------------------------------*/
      while (packet.transmission_continues)
         {
         if (progress)
            progress();
         qm100_continueTransmission(serialdev, "GetPicture");
         qm100_getPacket(serialdev, &packet);
         write(jpgfile, packet.packet, packet.packet_len);
         }
      close(jpgfile);
    }
  /*-------------------------------------------------------------------*
   *                                                                   *
   * Get final acknowledgement for command completion                  *
   *                                                                   *
   *-------------------------------------------------------------------*/
  qm100_endTransmit(serialdev, "GetPicture");
  qm100_getCommandTermination(serialdev);
}
