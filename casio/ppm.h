#ifndef _PPM_H
#define _PPM_H

#include "../src/gphoto.h"

extern int
record_ppm(u_char buf[], int PPM_WIDTH, int PPM_HEIGHT,
	   int rateW, int rateH, int withheader, int order,
	   struct Image *cameraImage);

#endif /* _PPM_H */
