/*
 * photopc-usb.c 
 *
 *  USB Olympus support
 *
 * Copyright 2000, Fabrice Bellet <fabrice@gnu.org>
 *
 * This module is covered by the GPL.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "olympus.h"
#include "photopc-usb.h"

#include "../src/gphoto.h"

#include <gpio.h>

int
olympus_usb_read(struct olympus_device *dev, void *buffer, int len)
{
#ifdef GPIO_USB
	if (dev)
		return gpio_read(dev->gpdev, buffer, len);
	else
		return GPIO_ERROR;
#else
	return GPIO_ERROR;
#endif
}

int
olympus_usb_send(struct olympus_device *dev, void *buffer, int len)
{
#ifdef GPIO_USB
	if (dev)
		return gpio_write(dev->gpdev, buffer, len);
	else
		return GPIO_ERROR;
#else
	return GPIO_ERROR;
#endif
}

#ifdef GPIO_USB
extern struct Model *Camera;

int 
olympus_usb_probe(struct olympus_device *dev)
{
	if (gpio_usb_find_device(dev->gpdev, Camera->idVendor,
				 Camera->idProduct) == GPIO_OK) {
                printf("found '%s' @ %s/%s\n", Camera->name,
 		       dev->gpdev->usb_device->bus->dirname,
		       dev->gpdev->usb_device->filename);
		return 1;
	}

	fprintf(stderr, "unable to find any compatible USB cameras\n");

	return 0;
}
#endif

struct olympus_device *
olympus_usb_open(void)
{
#ifdef GPIO_USB
	struct olympus_device *dev;
	gpio_device_settings settings;
	struct usb_device *udev;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;

	dev->gpdev = gpio_new(GPIO_DEVICE_USB);
	if (!dev->gpdev)
		return NULL;

	if (!olympus_usb_probe(dev))
		return NULL;

	gpio_set_timeout (dev->gpdev, 2000);
	settings.usb.udev = udev;
	settings.usb.inep = 0x83;
	settings.usb.outep = 0x04;
	settings.usb.config = 1;
	settings.usb.interface = 0;
	settings.usb.altsetting = 0;

	gpio_set_settings(dev->gpdev, settings);
	if (gpio_open(dev->gpdev) < 0) {
		fprintf(stderr, "error opening device\n");
		return NULL;
	}

	return dev;
#else
	return NULL;
#endif
}

void 
olympus_usb_close (struct olympus_device *dev)
{
#ifdef GPIO_USB
	if (dev) {
		gpio_close (dev->gpdev);
		gpio_free (dev->gpdev);
		free (dev);
	}
#endif
}

void
olympus_usb_reset (struct olympus_device *dev)
{
#ifdef GPIO_USB
	if (dev) {
		gpio_usb_clear_halt (dev->gpdev, GPIO_USB_IN_ENDPOINT);
	}
#endif
}
