#ifndef _TRANSMISSION_H
#define _TRANSMISSION_H
#include <stdio.h>
int   qm100_transmit(int serialdev, unsigned char *cmd, int cmd_len, qm100_packet_block *packet, char *id);
void  qm100_attention(int serialdev);
void  qm100_sendPacket(int serialdev, unsigned char *cmd, int cmd_len);
void  qm100_getAck(int serialdev);
int   qm100_getPacket(int serialdev, qm100_packet_block *packet);
void  qm100_endTransmit(int serialdev, char *title);
void  qm100_continueTransmission(int serialdev, char *title);
int   qm100_getRealPicNum(int serialdev, int picNum);
void  qm100_getCommandTermination(int serialdev);
void  qm100_setTransmitSpeed(void);
#endif
