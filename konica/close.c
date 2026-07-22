#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * close - set camera back to default speed, and send the              *
 *         'close' packet.                                             *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_close(int serialdev)
{
  unsigned char cmd[] = QM100_SUSPEND;
  qm100_packet_block packet;
  qm100_setSpeed(serialdev, B9600);
  qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "Close");
  close(serialdev);
}
