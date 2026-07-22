#ifndef OLYMPUS_H
#define OLYMPUS_H

#include <gpio.h>

struct olympus_device {
	gpio_device *gpdev;

	/* These parameters are only significant for serial support */
	int portspeed;
	int deviceframesize;
};
#endif
