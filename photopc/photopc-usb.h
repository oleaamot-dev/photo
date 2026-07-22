#ifndef _PHOTOPC_USB_H_
#define _PHOTOPC_USB_H

#include "olympus.h"

int olympus_usb_read(struct olympus_device *dev, void *buffer, int len);
int olympus_usb_send(struct olympus_device *dev, void *buffer, int len);
struct olympus_device *olympus_usb_open(void);
void olympus_usb_close (struct olympus_device *dev);
void olympus_usb_reset (struct olympus_device *dev);

#endif
