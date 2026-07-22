#define DEFINE_GLOBALS
#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * transmit - send a command packet, and read the response             *
 *            packet.                                                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
int qm100_transmit(int serialdev, unsigned char *cmd, int cmd_len,
                   qm100_packet_block *packet, char *title)
{
   if (qm100_trace)
      fprintf(qm100_trace, "Beginning operation: %s\n", title);
   qm100_attention(serialdev);
   qm100_sendPacket(serialdev, cmd, cmd_len);
   qm100_getAck(serialdev);
   qm100_getPacket(serialdev, packet);
   if (!packet->transmission_continues)
      qm100_endTransmit(serialdev, title);
   return (1);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * attention - get camera's permission to send a command packet.       *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_attention(int serialdev)
{
   char c;
   qm100_packet_block packet;
   int  limit;
  restart:
   /*------------------------------------------------------------------*
    *                                                                  *
    * Send SIO_ENQ to the camera, and try to read a response byte.     *
    * Retry this up to 100 time, at 10ms intervals.                    *
    *                                                                  *
    *------------------------------------------------------------------*/
   limit = 100;
   do
      {
      qm100_writeByte(serialdev, SIO_ENQ);
      c=qm100_readTimedByte(serialdev);
      } while (c == 0 && --limit >0);
   if (c)
      c=qm100_readByte(serialdev);
   switch (c)
      {
      case  SIO_ENQ:
         /*---------------------------------------------------------------*
          *                                                               *
          * Camera wants to send data - we're not expecting any, but we   *
          * need to keep him happy, so read it and discard it, then try   *
          * again to get his attention.                                   *
          *                                                               *
          *---------------------------------------------------------------*/
         packet.transmission_continues=1;
         while (packet.transmission_continues)
            {
            qm100_getPacket(serialdev, &packet);  // get his data
            qm100_writeByte(serialdev, SIO_ACK);  // and acknowledge
            qm100_readByte(serialdev);            // should be eot
            }
         goto restart;
      case  SIO_EOT:
         /*------------------------------------------------------------*
          *                                                            *
          * Must be left over from previous command - just ignore it.  *
          *                                                            *
          *------------------------------------------------------------*/
         goto restart;
      default:
         /*------------------------------------------------------------*
          *                                                            *
          * Unexpected response, or no response at all.  Nothing we    *
          * can do except report the error                             *
          *                                                            *
          *------------------------------------------------------------*/
         if (qm100_trace && c)
            fprintf(qm100_trace, "Attention response invalid - %02x\n", c);
         qm100_error(serialdev, "Camera is not online", 0);
         break;
      case SIO_ACK:
         /*------------------------------------------------------------*
          *                                                            *
          * Camera acknowledges our request.  Away we go!!!            *
          *                                                            *
          *------------------------------------------------------------*/
         break;
      }
   if (qm100_trace)
      fprintf(qm100_trace, "Attention acknowledged by camera\n");
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * sendPacket - send command packet to the camera, escaping/encoding   *
 *              control code bytes as needed.                          *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_sendPacket(int serialdev, unsigned char *cmd, int cmd_len)
{
   unsigned char esc_list[255];
   unsigned char packet[255];
   unsigned char packet_sum=0;
   unsigned packet_pos=0, cmd_pos=0, esc_lookup=0;
   int data;
   /*------------------------------------------------------------------*
    *                                                                  *
    * Initialize translate table for escape/encoding                   *
    *                                                                  *
    *------------------------------------------------------------------*/
   memset(&esc_list, 0, sizeof(esc_list));
   esc_list[0x02] = 0xfd;
   esc_list[0x03] = 0xfc;
   esc_list[0x05] = 0xfa;
   esc_list[0x06] = 0xf9;
   esc_list[0x11] = 0xee;
   esc_list[0x1b] = 0xe4;
   /*------------------------------------------------------------------*
    *                                                                  *
    * Build packet header, consisting of SIO_STX, followed by 2-byte   *
    * packet length.                                                   *
    *                                                                  *
    *------------------------------------------------------------------*/
   memset(&packet, 0, sizeof(packet));
   packet[packet_pos]=SIO_STX;
   packet_pos++;
   esc_lookup = esc_list[ (cmd_len & 0xff) ];
   packet_sum+=(cmd_len & 0xff);
   if (esc_lookup)
      {
      packet[packet_pos]=0x1b;
      packet_pos++;
      packet[packet_pos]=esc_lookup;
      packet_pos++;
      }
   else
      {
      packet[packet_pos]=(cmd_len & 0xff);
      packet_pos++;
      }
   esc_lookup = esc_list[((cmd_len>>8) & 0xff)];
   packet_sum+=((cmd_len>>8) & 0xff);
   if (esc_lookup)
      {
      packet[packet_pos]=0x1b;
      packet_pos++;
      packet[packet_pos]=esc_lookup;
      packet_pos++;
      }
   else
      {
      packet[packet_pos]=((cmd_len>>8) & 0xff);
      packet_pos++;
      }
   /*------------------------------------------------------------------*
    *                                                                  *
    * Encode the data bytes into the packet.                           *
    *                                                                  *
    *------------------------------------------------------------------*/
   while (cmd_pos < cmd_len)
      {
      data=cmd[cmd_pos];
      cmd_pos++;
      packet_sum+=data;
      esc_lookup = esc_list[data];
      if (esc_lookup)
         {
         packet[packet_pos]=0x1b;
         packet_pos++;
         packet[packet_pos]=esc_lookup;
         packet_pos++;
         }
      else
         {
         packet[packet_pos]=data;
         packet_pos++;
         }
      }
   /*------------------------------------------------------------------*
    *                                                                  *
    * Append the packet trailer, containing SIO_ETX and the calculated *
    * checksum.                                                        *
    *                                                                  *
    *------------------------------------------------------------------*/
   packet[packet_pos]=SIO_ETX;
   packet_pos++;
   packet_sum += SIO_ETX;
   packet_sum = packet_sum & 0xff;
   esc_lookup = esc_list[packet_sum];
   if (esc_lookup)
      {
      packet[packet_pos]=0x1b;
      packet_pos++;
      packet[packet_pos]=esc_lookup;
      packet_pos++;
      }
   else
      {
      packet[packet_pos]=packet_sum;
      packet_pos++;
      }
   /*------------------------------------------------------------------*
    *                                                                  *
    * Send the encoded packet to the camera.                           *
    *                                                                  *
    *------------------------------------------------------------------*/
   if (qm100_trace)
      dump(qm100_trace, "Send Packet", packet, packet_pos);
   if ((write(serialdev, packet, packet_pos)) < packet_pos)
      qm100_error(serialdev, "Cannot write to device", errno);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * getAck - Read and verify acknowledgement from camera, discarding    *
 *          extraneous bytes.  We then expect to find SIO_ENQ from     *
 *          the camera, showing his attempt to send a response.  If    *
 *          we get it, we send SIO_ACK to let him proceed.             *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_getAck(int serialdev)
{
   char c;
   unsigned  retries=0;
   c=qm100_readByte(serialdev);
   if (c != SIO_ACK)
      qm100_error(serialdev, "Acknowledgement Failed", 0);
   qm100_writeByte(serialdev, (char) SIO_EOT);
   while(c != SIO_ENQ)
      {
      retries++;
      c=qm100_readByte(serialdev);
      }
   if (retries > 2)
      printf("%u unexpected bytes discarded\n", retries-1);
   qm100_writeByte(serialdev, SIO_ACK);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * packetError - send packet error message to stdout,                  *
 *               and to the trace if active.                           *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void packetError(char *msg, int retry, int pktcnt)
{
   printf("\n%s - restarting packet #%d retry #%d\n",
          msg, pktcnt, retry);
   fflush(stdout);
   if (qm100_trace)
      {
      fprintf(qm100_trace,"\n%s - restarting packet #%d retry #%d\n",
             msg, pktcnt, retry);
      fflush(qm100_trace);
      }
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * getPacket - receive a packet from the camera.                       *
 *    Operation:                                                       *
 *         1. Receive bytes until we get 'start transmission'          *
 *            SIO_STX)                                                 *
 *         2. Read the length of the packet, in the next 2 bytes.      *
 *         3. Read the packet bytes, calculating a checksum as         *
 *            we go.  If we get an un-escaped control character        *
 *            during this phase, we've experienced a data error,       *
 *            so restart the process.                                  *
 *         4. Read the end transmission byte,  either SIO_ETB or       *
 *            SIO_ETX.  If we read anything else, we've got a          *
 *            data error, so restart the process.                      *
 *         5. Read the checksum byte.  If it does not match our        *
 *            calculated value, restart the process.                   *
 *         6. Send positive acknowledgement to the camera              *
 *            (SIO_ACK).                                               *
 *                                                                     *
 *     When the camera finishes sending a packet, it expects us        *
 *     to send SIO_ACK; if we do not in a reasonable amount of         *
 *     time (???),  it will attempt to send it again, an unknown       *
 *     number of times.  Unless our system is totally incapable        *
 *     of handling the current baud-rate, we should eventually         *
 *     recover from the error.                                         *
 *                                                                     *
 *---------------------------------------------------------------------*/
int qm100_getPacket(int serialdev, qm100_packet_block *packet)
{
   unsigned char c, qm100_sum=0, sum=0;
   short len, pos=0;
   int  retries=0;
   static int pktcnt = 0;
   ++pktcnt;
  restart:
   ++retries;
   /*------------------------------------------------------------------*
    *                                                                  *
    * Read the packet header, consisting of SIO_STX and length.        *
    *                                                                  *
    *------------------------------------------------------------------*/
   while (c != SIO_STX)
      c = qm100_readByte(serialdev);
   qm100_sum = 0;
   sum = 0;
   pos = 0;
   c=qm100_readCodedByte(serialdev);
   len=c;
   sum=c;
   c=qm100_readCodedByte(serialdev);
   len += (c<<8);
   sum += c;
   packet->packet_len=len;
   /*------------------------------------------------------------------*
    *                                                                  *
    * Read the data bytes for the packet                               *
    *                                                                  *
    *------------------------------------------------------------------*/
   while (len--)
      {
      c=qm100_readCodedByte(serialdev);
      if (c == SIO_STX && !qm100_escapeCode)
         {
         packetError("Transmission data error", retries, pktcnt);
         goto restart;
         }
      packet->packet[pos]=c;
      pos++;
      sum+=c;
      }
   if (qm100_trace)
      dump(qm100_trace, "Receive Packet", packet->packet, packet->packet_len);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Read the packet trailer, consisting of SIO_ETB or SIO_ETX,       *
    * followed by the checksum                                         *
    *                                                                  *
    *------------------------------------------------------------------*/
   c=qm100_readByte(serialdev);
   if (c==SIO_ETX)
      packet->transmission_continues=0;
   else if (c==SIO_ETB)
      packet->transmission_continues=1;
   else
      {
      packetError("Transmission trailer error", retries, pktcnt);
      goto restart;
      }
   sum+=c;
   sum=(sum & 0xff);
   qm100_sum=qm100_readCodedByte(serialdev);
   if (qm100_sum != sum)
      {
      packetError("Transmission checksum error", retries, pktcnt);
      goto restart;
      }
   /*------------------------------------------------------------------*
    *                                                                  *
    * Everything is AOK, so send the SIO_ACK                           *
    *                                                                  *
    *------------------------------------------------------------------*/
   qm100_writeByte(serialdev, SIO_ACK);
   return 0;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * endTransmission - read and verify end of transmission from the      *
 *                   camera.                                           *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_endTransmit(int serialdev, char *title)
{
   char c;
   c=qm100_readByte(serialdev);
   if (c != SIO_EOT)
      qm100_error(serialdev, "End of Transmission Failed", 0);
   if (qm100_trace)
      fprintf(qm100_trace, "End transmission for %s\n", title);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * continueTransmission - tell camera to resume transmission.          *
 *                                                                     *
 *         We expect to find SIO_EOT from the previous packet,         *
 *         then SIO_ENQ for the continuation.  If everything is        *
 *         ok, we send SIO_ACK to allow the camer to proceed.          *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_continueTransmission(int serialdev, char *title)
{
   char c;
   if (qm100_trace)
      fprintf(qm100_trace, "Continue transmission for %s\n", title);
   c=qm100_readByte(serialdev);
   if (c != SIO_EOT)
      qm100_error(serialdev, "Unexpected response to continue Transmission", 0);
   c=qm100_readByte(serialdev);
   if (c != SIO_ENQ)
      qm100_error(serialdev, "Unexpected response to continue Transmission", 0);
   qm100_writeByte(serialdev, SIO_ACK);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * getRealPicNum - return camera's internal number for a picture.      *
 *       Because of deletions, internal numbering for pictures         *
 *       may not be consecutive.  We retrieve the info using           *
 *       external relative picture number, and extract the internal    *
 *       number from the info block.                                   *
 *                                                                     *
 *---------------------------------------------------------------------*/
int qm100_getRealPicNum(int serialdev, int picNum)
{
   qm100_packet_block packet;
   int realPicNum;
   qm100_getPicInfo(serialdev, picNum, &packet);
   if (packet.packet_len == 0x3ff)
      sscanf(PICNUM, "%d", &realPicNum);       // hpc20
   else if (packet.packet_len == 0x37a)
      sscanf(PICNUM-0x42, "%d", &realPicNum);  // hpc30
   else
      qm100_error(serialdev,"Unexpected packet length in response to getPicInfo",0);
   return (realPicNum);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * qm100_getCommandTermination - get command acknowledgement           *
 *          packet from the camera.                                    *
 *                                                                     *
 *     Some of the simpler commands, such as getStatus, consist of a   *
 *     single response packet, which contains the requested data, as   *
 *     well as an acknowledgent of command completion.                 *
 *     'qm100_transmit' always retrieves the first response packet, so *
 *     for these simple commands, the transaction is complete when     *
 *     qm100_transmit returns.                                         *
 *                                                                     *
 *     Other commands, such as getPicture or getThumb, consist of      *
 *     multiple data packets, followed by a short command              *
 *     acknowledgement packet.  After the last data packet has been    *
 *     received, the functions which process these request must also   *
 *     request the acknowledgement packet.                             *
 *                                                                     *
 *     All we need actually do is read and verify the SIO_ENQ request  *
 *     from the camera, then call getPacket to read the actual         *
 *     response bytes.                                                 *
 *                                                                     *
 *---------------------------------------------------------------------*/
void  qm100_getCommandTermination(int serialdev)
{
   qm100_packet_block packet;
   char c;
   c = qm100_readByte(serialdev);
   if (c != SIO_ENQ)
      qm100_error(serialdev,
                  "Unexpected data while expecting command termination packet", 0);
   qm100_writeByte(serialdev, SIO_ACK);
   qm100_getPacket(serialdev, &packet);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * setTransmitSpeed - determine baudrate and pacing values from        *
 *                    the environment and/or ~/.gphoto/konicarc.       *
 *                                                                     *
 *---------------------------------------------------------------------*/
void  qm100_setTransmitSpeed(void)
{
   char *sp;
   sp = qm100_getKeyword("SPEED", DEFAULT_SPEED);
   while (qm100_transmitSpeed == 0)
      {
      unsigned l;
      if (!sp)
         sp = DEFAULT_SPEED;
      l = strlen(sp);
      if (strncmp(sp, "115200", l) == 0)
         qm100_transmitSpeed = B115200;
      else if (strncmp(sp, "57600", l) == 0)
         qm100_transmitSpeed = B57600;
      else if (strncmp(sp, "38400", l) == 0)
         qm100_transmitSpeed = B38400;
      else if (strncmp(sp, "19200", l) == 0)
         qm100_transmitSpeed = B19200;
      else if (strncmp(sp, "9600", l) == 0)
         qm100_transmitSpeed = B9600;
      else
         {
         printf("qm100:  Invalid speed %s - using default (%s)\n",
                sp, DEFAULT_SPEED);
         sp = NULL;
         }
      }
   sp = qm100_getKeyword("PACING", DEFAULT_PACING);
   while (qm100_sendPacing == 0)
      {
      qm100_sendPacing = atoi(sp);
      if (qm100_sendPacing < 1)
         {
         printf("qm100:  Invalid pacing value %s - using default (%s)\n",
                sp, DEFAULT_PACING);
         sp = DEFAULT_PACING;
         }
      }
}
