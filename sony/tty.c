#include "config.h"
#include <stdio.h>
#include <string.h>
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <stdlib.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#if HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#else
# if HAVE_TERMIO_H
#  include <termio.h>
# else
#  if HAVE_SYS_IOCTL_H
#   include <sys/ioctl.h>
#  endif
#  include <sgtty.h>
# endif
#endif
#if HAVE_TTOLD_H 
#   include <ttold.h>
#endif

#include "common.h"
#include "tty.h"

#define	TTYTIMEOUT	10

int changespeed(fd, baud)
     int fd;
     int baud;
{
#if HAVE_TERMIOS_H
  /* termios interface */
  struct termios tio;

  if (tcgetattr(fd, &tio) < 0) {
    fprintf(stderr, " Can't get tty attribute.\n");
    close(fd);
    return(-1);
  }
  tio.c_iflag = 0;
  tio.c_oflag = 0;
  tio.c_cflag = CS8 | CREAD | CLOCAL  ; /* 8bit non parity stop 1 */
  tio.c_lflag = 0;
  tio.c_cc[VMIN] = 1;
  tio.c_cc[VTIME] = 5 ;
  cfsetispeed(&tio, baud);
  cfsetospeed(&tio, baud);
  if (tcsetattr(fd, TCSANOW, &tio) < 0) {
    perror("Can't set tty attribute.\n");
    return(-1);
  }
#else
# if HAVE_TERMIO_H
  /* termio interface */
  /*  #  error not implemented yet. */
  fprintf(stderr,"sorry not inplemented yet.\n");
# else
  /* sgtty interface */
  struct sgttyb ttyb;
  if (ioctl(fd, TIOCGETP, &ttyb) < 0) {
    perror("Can't get tty attribute.\n");
    return(-1);
  }
  
  ttyb.sg_ispeed = baud;
  ttyb.sg_ospeed = baud;
  ttyb.sg_flags = 0;		/* 8bit non parity stop 1 */
  if (ioctl(fd, TIOCSETP, &ttyb) < 0) {
    perror("Can't set tty attribute.\n");
    return(-1);
  }

# endif
#endif
  flushtty(fd);
  return(1);
}

int opentty(path)
     char *path;
{
  int fd;
  struct termios tio;

  if ((fd = open(path, O_RDWR | O_NDELAY )) != -1) {
#if HAVE_TERMIOS_H
#else
    if(ioctl(fd, TIOCEXCL, 0) < 0){
      fprintf(stderr, "Can't set tty exclusive mode.\n");
      return(-1);
    }
    if(ioctl(fd, TIOCHPCL, 0) < 0){
      fprintf(stderr, "Can't set tty hold mode.\n");
      return(-1);
    }
#endif

    if(changespeed(fd, DEFAULTBAUD) < 0)
      return(-1); 
  }else{
    fprintf(stderr, "Can't open tty.\n");
    return(-1);
  }

  return(fd);
}

int readtty(fd, p, c)
     int fd;
     unsigned char *p;
     int c;
{
  fd_set readfds;
  int nfds;
  struct timeval tv;
  int i,j;
  unsigned char u;
	
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  tv.tv_sec = TTYTIMEOUT;
  tv.tv_usec = 0;

  for(j = 0 ; j < c ; j++){
    nfds = select( fd +1 , &readfds, NULL, NULL, &tv);
    if(nfds == 0){
      fprintf(stderr,"tty not respond. time up.\n");
      return(0);
    } else {
      if(FD_ISSET(fd, &readfds)){
	if(read(fd, &u, 1) < 0){
	  fprintf(stderr,"tty read fail.\n");
	  return(-1);
	}
	*p = u;
	p++;
      }
    }
  }
  return(j);
}

void flushtty(fd)
     int fd;
{
  u_char c;
  fd_set readfds;
  int nfds;
  struct timeval tv;
  int i,j;
  unsigned char u;
	
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  while (1) {
    nfds = select(fd +1 , &readfds, NULL, NULL, &tv);
    if(nfds == 0){
      return;
    } else {
      if(FD_ISSET(fd, &readfds)){
	if(read(fd, &c, 1) < 0){
	  fprintf(stderr,"tty read fail.\n");
	  return;
	}
      }
    }
  }
}

