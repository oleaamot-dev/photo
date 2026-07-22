#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * formatCF - send camera command to format/erase the flash memory     *
 *            card.                                                    *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_formatCF(int serialdev)
{
  unsigned char cmd[] = QM100_FORMAT;
  qm100_packet_block packet;
  qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "Format CF");
}
