/*


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "serio.h"


/***************************************************************
**
**
*/
int ConfigDSCF55Speed(char *spd_buf, int verbose)
{
  int speed = B9600;

/* printf("%s\n", spd_buf); */

  if (*spd_buf == 'B') spd_buf++;
  if      (!strcmp(spd_buf, "115200")) speed = B115200;
#if defined(B76800)
  else if (!strcmp(spd_buf,  "76800")) speed = B76800;
#endif
  else if (!strcmp(spd_buf,  "57600")) speed = B57600;
  else if (!strcmp(spd_buf,  "38400")) speed = B38400;
  else if (!strcmp(spd_buf,  "19200")) speed = B19200;
  else if (!strcmp(spd_buf,   "9600")) speed = B9600;

  if (verbose > 1) printf("Speed set to %u (%s bps)\n", speed, spd_buf);

  return speed;
}




