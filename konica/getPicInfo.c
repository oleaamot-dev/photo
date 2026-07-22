#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * getPicInfo - send command to retrieve information packet            *
 *              for a single picture.                                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
int qm100_getPicInfo(int serialdev, int pic, qm100_packet_block *packet)
{
  unsigned char cmd[]=QM100_PICINFO;
  cmd[5]=(char)(pic>>8 & 0xff);
  cmd[6]=(char)(pic & 0xff);
  qm100_transmit(serialdev, cmd, sizeof(cmd), packet, "GetPicInfo");
  qm100_getCommandTermination(serialdev);
  return(1);
}
