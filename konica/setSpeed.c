#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * setSpeed - set baud rate for transmissions.                         *
 *                                                                     *
 *          1. Send a packet to the camera telling it about            *
 *             the new speed.                                          *
 *          2. Use IOCTL to change the baud rate.                      *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_setSpeed(int serialdev, int speed)
{
  qm100_packet_block packet;
  int reg;
  unsigned char cmd[]=QM100_SETSPEED;
  newt.c_cflag |= CS8;
  newt.c_iflag &= ~(IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR);
  #if defined(__FreeBSD__) || defined(__NetBSD__) /* by fujisawa@konica.co.jp */
  newt.c_iflag &= ~(IGNCR|ICRNL|IXON|IXOFF|      IXANY|IMAXBEL);
  #else
  newt.c_iflag &= ~(IGNCR|ICRNL|IXON|IXOFF|IUCLC|IXANY|IMAXBEL);
  #endif
  newt.c_oflag &= ~(OPOST);
  newt.c_lflag &= ~(ISIG|ICANON);
  #if defined(__FreeBSD__) || defined(__NetBSD__)
  /**/
  #else
  newt.c_cflag &= ~(XCASE);
  #endif
  newt.c_cc[VMIN] = 255;
  newt.c_cc[VTIME] = 5;
  switch (speed)
    {
    case B9600:
      reg = 0x020;
      break;
    case B19200:
      reg = 0x040;
      break;
    case B38400:
      reg = 0x080;
      break;
    case B57600:
      reg = 0x100;
      break;
    case B115200:
      reg = 0x200;
      break;
    }
  cmd[4]=(reg & 0xff);
  cmd[5]=((reg>>8) & 0xff);
  qm100_transmit(serialdev, cmd, sizeof(cmd), &packet, "Set Speed");
  if (packet.packet_len != 8)
     qm100_error(serialdev, "SetSpeed incorrect response length", 0);
  else
     {
     cfsetispeed(&newt, speed);
     cfsetospeed(&newt, speed);
     if (tcsetattr(serialdev, TCSANOW, &newt) < 0)
        qm100_error(serialdev, "Unable to set serial device attributes", errno);
     }
}
