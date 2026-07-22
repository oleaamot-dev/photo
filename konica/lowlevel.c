#include "qm100.h"
#include <sys/ioctl.h>
#include <termios.h>
/*---------------------------------------------------------------------*
 *                                                                     *
 * readByte - read a single byte from the serial port                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
char qm100_readByte(int serialdev)
{
  char byte;
  if ((read(serialdev, &byte, 1)) < -1)
     qm100_error(serialdev, "Cannot read from device", errno);
  if (qm100_showBytes)
     qm100_iostat("recv :", &byte, 1);
  return byte;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * readTimedByte - wait for a byte to arrive at the serial port, but   *
 *                 timeout after 1ms.                                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
char qm100_readTimedByte(int serialdev)
{
  fd_set rfds;
  struct timeval tv;
  FD_ZERO(&rfds);
  FD_SET(serialdev, &rfds);
  tv.tv_sec=0;
  tv.tv_usec=1000;
  return (select(1+serialdev, &rfds, NULL, NULL, &tv));
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * readCodedByte - return a single data byte from the serial port.     *
 *                 For  control characters which are 'escaped', we     *
 *                 must actually read 2 bytes, returning the           *
 *                 decoded value of the second one.                    *
 *                                                                     *
 *---------------------------------------------------------------------*/
char qm100_readCodedByte(int serialdev)
{
  char byte;
  byte=qm100_readByte(serialdev);
  if (byte==0x1b)
    {
      byte=qm100_readByte(serialdev);
      byte=(~byte & 0xff);
      qm100_escapeCode = 1;
    }
  else
      qm100_escapeCode = 0;
  return byte;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * writeByte - send a single byte to the serial port                   *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_writeByte(int serialdev, char data)
{
  usleep(qm100_sendPacing * 1000);
  if ((write(serialdev, &data, 1)) < -1)
     qm100_error(serialdev, "Cannot write to device", errno);
  if (qm100_showBytes)
     qm100_iostat("sent :", &data, 1);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * iostat - write a sequence of bytes to the trace file                *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_iostat(unsigned char *str, unsigned char *buf, int len)
{
  fprintf(qm100_trace, "%s ", str);
  if(len>0)
    {
      int p = 1;
      fprintf(qm100_trace, "0x%02x", buf[0]);
      while(p<len)
          fprintf(qm100_trace, ", 0x%x", (unsigned char)buf[p++]);
    }
  fprintf(qm100_trace,"\n");
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * resetUART - attempt to reset the serial port and camera             *
 *             by dropping  DTR and RTS for three seconds.  This       *
 *             sometimes clears the camera from a hung state.          *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_resetUart(int serialdev)
{
   int out;
   sleep(1);
   out = TIOCM_DTR | TIOCM_RTS;
   ioctl(serialdev, TIOCMBIC, &out);
   sleep(3);
   ioctl(serialdev, TIOCMBIS, &out);
}
