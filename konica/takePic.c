#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * takePic - send command to take a picture.                           *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_takePic(int serialdev)
{
  unsigned char cmd_takepic[]=QM100_TAKEPIC;
  qm100_packet_block packet;
/*---------------------------------------------------------------------*
 *                                                                     *
 * The following messages to the camera are intended to set various    *
 * camera controls which affect the actual process of taking a         *
 * picture.  Even if they work, which is not at all clear, it seems    *
 * more appropriate to do these things in the 'Configure Camera'       *
 * dialog, and indeed, to save them in a configuration file.  If you   *
 * want to try them out, simply add -DSET_CAMERA_CONTROLS to CFLAGS in *
 * the Makefile, and re-build.                                         *
 *                                                                     *
 *---------------------------------------------------------------------*/
#ifdef SET_CAMERA_CONTROLS
  /* Set Quality */
  unsigned char cmd_setpic[]=QM100_SETPIC;
  cmd_setpic[4]=0x00;
  cmd_setpic[5]=0x80;
  cmd_setpic[6]=QUALITY_FINE;
  qm100_transmit(serialdev, cmd_setpic, sizeof(cmd_setpic), &packet, "Set Quality");
  /* Set Flash */
  cmd_setpic[4]=0x00;
  cmd_setpic[5]=0x90;
  cmd_setpic[6]=FLASH_AUTO;
  qm100_transmit(serialdev, cmd_setpic, sizeof(cmd_setpic), &packet, "Set Flash");
  /* Set Exposure */
  cmd_setpic[4]=0x02;
  cmd_setpic[5]=0x80;
  cmd_setpic[6]=0x00;
  qm100_transmit(serialdev, cmd_setpic, sizeof(cmd_setpic), &packet, "Set Exposure");
  /* Set Focus */
  cmd_setpic[4]=0x02;
  cmd_setpic[5]=0x90;
  cmd_setpic[6]=FOCUS_AUTO;
  qm100_transmit(serialdev, cmd_setpic, sizeof(cmd_setpic), &packet, "Set Focus");
#endif /* SET_CAMERA_CONTROLS */
  /* Take Picture */
  qm100_transmit(serialdev, cmd_takepic, sizeof(cmd_takepic), &packet, "Take Picture");
  qm100_getCommandTermination(serialdev);
}
