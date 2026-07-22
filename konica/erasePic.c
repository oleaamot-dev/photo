#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * erasePic - send command to erase a single picture.                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_erasePic(int serialdev, int pic)
{
  unsigned char cmd[] = QM100_ERASE;
  qm100_packet_block packet;
  cmd[5] = (pic >> 8) & 0xff;
  cmd[6] = pic & 0xff;
  qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "Erase");
}
