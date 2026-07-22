#include "config.h"
#include <sys/types.h>

unsigned short
get_u_short(buf)
     unsigned char 	*buf;
{
  return ((unsigned short)buf[0] << 8) | buf[1];
}

unsigned int
get_u_int(buf)
        unsigned char  *buf;
{
        unsigned int   t;

        t = (((unsigned int)buf[0] << 8) | buf[1]) << 16;;
        t |= ((unsigned int)buf[2] << 8) | buf[3];;
        return t;
}

