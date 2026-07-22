#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdlib.h>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#    ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#    else HAVE_SYS_TIME_H
#    include <time.h>
#    endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else HAVE_TERMIOS_H
#    ifdef HAVE_TERMIO_H
#    include <termio.h>
#    else HAVE_TERMIO_H
#    include <sgtty.h>
#    endif /* HAVE_TERMIO_H */
#endif /* HAVE_TERMIOS_H */

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */

#ifdef HAVE_IOCTL_TYPES_H
#include <ioctl-types.h>
#endif

#ifdef HAVE_TTOLD_H 
#include <ttold.h>
#endif

#define	TTYTIMEOUT	15

#include "sdCommDefines.h"
#include "sdcMessages.h"
#include "printMsg.h"

typedef struct _sdcInfo {
    char *devName;
    sdcState state;
    int fd;
    int speed;
    int debug;
} *sdcInfo;

static sdcInfo
createInfoStruct() {
    sdcInfo info;

    info = (sdcInfo)malloc(sizeof(struct _sdcInfo));
    if (info == NULL) {
	print_error(MALLOC_ERROR, "createInfoStruct");
	exit(-1);
    }

    return(info);
}

sdcInfo
sdcInit(char *serialDeviceName) {
    sdcInfo info;

    info = createInfoStruct();
    if (info != NULL) {
	info->devName = malloc(sizeof(serialDeviceName) + 1);
	if (info->devName == NULL) {
	    print_error(MALLOC_ERROR, "sdcInit");
	    exit(-1);
	}
    }

    strcpy(info->devName, serialDeviceName);
    info->state = SDC_CLOSED;
    info->fd = -1;
    info->speed = -1;
    info->debug = 0;

    return(info);
}

void
sdcDebug(sdcInfo info, int onOff) {
    if (onOff != info->debug) {
	fprintf(stderr, "Setting debug state to %s for device %s\n",
		onOff ? "on" : "off",  info->devName);
	info->debug = onOff;
    }
}

sdcStatus
sdcOpen(sdcInfo info) {
    int flags;
  
    if (info == NULL) return(SDC_FAIL);

    if (info->debug) {
	fprintf(stderr, "Entering sdcOpen:\n");
    }

    if (info->state == SDC_OPEN) {
	fprintf(stderr, "Warning: Called sdcOpen on an alreaady open connection\n");
	return(SDC_SUCCESS); /* Not technically a failure */
    }

    #ifdef BSD /* by fujisawa */
    info->fd = open(info->devName, O_RDWR );
    #else
    /* first open with O_NONBLOCK so that the open systemcall won't block */
    info->fd = open(info->devName, O_RDWR | O_SYNC | O_NONBLOCK );
    /* then deactivate O_NONBLOCK again because we do not want it */
    flags = fcntl(info->fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(info->fd, F_SETFL, flags);
    /* if we wouldn't do it so complicated, the program last using the
       tty may leave it in a state so that open blocks.
       pppd does this when a modem is attached for example. */
    #endif
    if (info->fd < 0) {
	print_error(CANT_OPEN_DEVICE, info->devName);

	if (info->debug) {
	    fprintf(stderr, "Leaving sdcOpen\n");
	}
	return(SDC_FAIL);
    }

#ifndef HAVE_TERMIOS_H
    if (ioctl(info->fd, TIOCEXCL, 0) < 0) {
	print_warning(CANT_SET_EXCLUSIVE);

	if (info->debug) {
	    fprintf(stderr, "Leaving sdcOpen\n");
	}
	return(SDC_FAIL);
    }

    if(ioctl(info->fd, TIOCHPCL, 0) < 0){
	print_warning(CANT_SET_HOLD);

	if (info->debug) {
	    fprintf(stderr, "Leaving sdcOpen\n");
	}
	return(SDC_FAIL);
    }
#endif

    info->state = SDC_OPEN;

    if (info->debug) {
	fprintf(stderr, "Leaving sdcOpen\n");
    }
    return(SDC_SUCCESS);
}

int
sdcIsClosed(sdcInfo info) {
    return(info->state == SDC_CLOSED);
}

sdcStatus
sdcFlush(sdcInfo info) {
    u_char c;
    fd_set readfds;
    int nfds;
    struct timeval tv;
	
    if (info->debug) {
	fprintf(stderr, "Entering sdcFlush:\n");
    }

    if (info->state == SDC_CLOSED) {
	print_error(PORT_IS_CLOSED, "sdcFlush");
	return(SDC_FAIL);
    }

    FD_ZERO(&readfds);
    FD_SET(info->fd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while (1) {
	nfds = select(info->fd +1 , &readfds, NULL, NULL, &tv);
	if(nfds == 0){
	    return(SDC_SUCCESS);
	} else {
	    if(FD_ISSET(info->fd, &readfds)){
		if(read(info->fd, &c, 1) < 0){
		    print_error(CANT_READ_DEVICE, "sdcFlush");
		    return(SDC_FAIL);
		}
	    }
	}
    }
    return(SDC_SUCCESS);
}

sdcStatus
sdcClose(sdcInfo info) {
    if (info == NULL) return(SDC_FAIL);

    if (info->state == SDC_CLOSED) {
	print_error(PORT_IS_CLOSED, "sdcClose");
	return(SDC_FAIL);
    }

    /* sdcFlush(info); */
    close(info->fd);
    info->fd = -1;
    info->speed = -1;
    info->state = SDC_CLOSED;
    return(SDC_SUCCESS);
}

static char *
convertToAscii(char *str, int len, char *lineprefix) {
    static char result[8196];
    u_char *p;
    int i;
    char *linebegin;

    strcpy(result, lineprefix);

    linebegin = result;
    for (p = str, i = 0; i < len; i++, p++) {
	if (len <= 2 && isprint(*p)) {
	    strncat(result, p, 1);
	} else {
	    sprintf(result, "%s 0x%02x ", result, *p);
	}
	if (strlen(result) - (linebegin - result) > 60) {
	    strcat(result, "\n");
	    strcat(result, lineprefix);
	    linebegin = result + strlen(result);
	}
    }

    return(result);
}


static int
readPort(sdcInfo info, unsigned char *readBuf, int len_required) {
    fd_set readfds;
    int nfds;
    struct timeval tv;
    int read_so_far = 0;
    int amount_to_read;
    int count;
    unsigned char buf[SDC_MAX_BUFSIZE + 1];
    unsigned char *p = readBuf;
	
    FD_ZERO(&readfds);
    FD_SET(info->fd, &readfds);
    tv.tv_sec = TTYTIMEOUT;
    tv.tv_usec = 0;

    while(read_so_far < len_required || len_required < 0) {
	nfds = select(info->fd +1 , &readfds, NULL, NULL, &tv);
	if(nfds == 0){
	    if (len_required > 0) print_error(READ_TIMEOUT);
	    return(read_so_far);
	} else {
	    if(FD_ISSET(info->fd, &readfds)){
		if (len_required > 0) {
		    amount_to_read = len_required - read_so_far;
		} else {
		    amount_to_read = SDC_MAX_BUFSIZE;
		}
		if ((count = read(info->fd, buf, amount_to_read)) < 0) {
		    perror("Failure occurred while atempting to read port");
		    return(read_so_far);
		}
		memcpy(p, buf, count);
		read_so_far += count;
		p += count;
		if (len_required < 0) break;
	    }
	}
    }
    return(read_so_far);
}

sdcStatus
sdcReadAll(sdcInfo info, u_char *buf, int *len) {
    if (info->debug) {
	fprintf(stderr, "Entering sdcReadAll:\n");
    }

    if (info->state == SDC_CLOSED) {
	print_error(PORT_IS_CLOSED, "sdcReadAll");
	return(SDC_FAIL);
    }
	
    *len = readPort(info, buf, -1);

    if (info->debug) {
	fprintf(stderr, "    Read (len=%d)%s%s\n",
		*len, (*len < 10) ? " ": "\n    ",
		convertToAscii(buf, *len, "    "));
	fprintf(stderr, "Leaving sdcReadAll\n");
    }

    return((*len == 0) ? SDC_FAIL : SDC_SUCCESS);
}

sdcStatus
sdcRead(sdcInfo info, u_char *buf, int len_required) {
    int read_count;
    if (info->debug) {
	fprintf(stderr, "Entering sdcRead: %d bytes\n", len_required);
    }

    if (info->state == SDC_CLOSED) {
	print_error(PORT_IS_CLOSED, "sdcRead");
	return(SDC_FAIL);
    }
	
    read_count = readPort(info, buf, len_required);

    if (info->debug) {
	fprintf(stderr, "    Read (len=%d of %d)%s%s\n",
		read_count, len_required, (read_count < 10) ? " ": "\n    ",
		convertToAscii(buf, read_count, "    "));
	fprintf(stderr, "Leaving sdcRead\n");
    }

    return((read_count != len_required) ? SDC_FAIL : SDC_SUCCESS);
}

sdcStatus
sdcWrite(sdcInfo info, u_char *c, int len) {
    int amountWritten;
    
    if (info->debug) {
	fprintf(stderr, "Entering sdcWrite:\n");
        fprintf(stderr, "    Writing (len=%d)%s%s\n",
		    len, (len < 10) ? " ": "\n    ",
		    convertToAscii(c, len, "    "));
    }

    if (info->state == SDC_CLOSED) {
	print_error(PORT_IS_CLOSED, "sdcWrite");
	return(SDC_FAIL);
    }
	
    amountWritten = write(info->fd, c, len); 

    if (info->debug) {
	fprintf(stderr, "Leaving sdcWrite\n");
    }
	
    return(SDC_SUCCESS);
}

sdcStatus
sdcSendByte(sdcInfo info, unsigned char c) {
    int status;
    
    if (info->debug) {
	fprintf(stderr, "Entering sdcSendByte:\n");
    }

    if (info->state == SDC_CLOSED) {
	print_error(PORT_IS_CLOSED, "sdcSendByte");
	return(SDC_FAIL);
    }
    
    status = sdcWrite(info, &c, 1);

    if (info->debug) {
	fprintf(stderr, "Leaving sdcSendByte\n");
    }

    return(status);
}

int
sdcGetBaudRate(sdcInfo info) {
    return(info->speed);
}

static sdcStatus
setBaudRateFinish(sdcInfo info) {
#if HAVE_RTS_IOCTL
    int mode;
    mode = TIOCM_RTS;
    if(ioctl(info->fd, TIOCMBIC, &mode) < 0){ /* RTS OFF */
	print_error(CANT_SET_RTS, "OFF");
	sdcClose(info);
	return(SDC_FAIL);  
    }
    mode = TIOCM_CTS|TIOCM_DTR;
    if(ioctl(info->fd, TIOCMBIS, &mode) < 0){ /* CTS DTR ON */
	print_error(CANT_SET_DTR, "ON");
	sdcClose(info);
	return(SDC_FAIL);  
    }
#endif

    sdcFlush(info);
    return(SDC_SUCCESS);
}

#ifdef HAVE_TERMIOS_H
sdcStatus
sdcSetBaudRate(sdcInfo info, int baud_rate) {
    /* termios interface */
    struct termios tio;
	
    if (info->debug) {
	fprintf(stderr, "Entering sdcSetBaudRate:\n");
    }

    if (tcgetattr(info->fd, &tio) < 0) {
	print_error(CANT_GET_ATTRIBUTE, "sdcSetBaudRate");
	sdcClose(info);
	return(SDC_FAIL);
    }
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CREAD | CLOCAL  ; /* 8bit non parity stop 1 */
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 5 ;
    cfsetispeed(&tio, baud_rate);
    cfsetospeed(&tio, baud_rate);
    if (tcsetattr(info->fd, TCSANOW, &tio) < 0) {
	perror("Can't set port attribute.\n");
	return(SDC_FAIL);
    }
    info->speed = baud_rate;
    return(setBaudRateFinish(info));
}

#else HAVE_TERMIOS_H
#    ifdef HAVE_TERMIO_H
sdcStatus
sdcSetBaudRate(sdcInfo info, int baud_rate) {
    if (info->debug) {
	fprintf(stderr, "Entering sdcSetBaudRate:\n");
    }

    /* termio interface */
    /*  #  error not implemented yet. */
    fprintf(stderr, "Set baud rate not implemented yet.\n");
    return(SDC_FAIL);
    /* return(setBaudRateFinish(info)); */
}

#    else HAVE_TERMIO_H
int
sdcSetBaudRate(sdcInfo info, int baud_rate) {
    /* sgtty interface */
    struct sgttyb ttyb;

    if (info->debug) {
	fprintf(stderr, "Entering sdcSetBaudRate:\n");
    }

    if (ioctl(info->fd, TIOCGETP, &ttyb) < 0) {
	print_error(CANT_GET_ATTRIBUTE, "sdcSetBaudRate");
	perror("While trying to get port attributes.\n");
	return(SDC_FAIL);
    }

    ttyb.sg_ispeed = baud_rate;
    ttyb.sg_ospeed = baud_rate;
    ttyb.sg_flags = 0;		/* 8bit non parity stop 1 */
    if (ioctl(info->fd, TIOCSETP, &ttyb) < 0) {
	print_error(CANT_SET_ATTRIBUTE, "sdcSetBaudRate");
	perror("While trying to set port attributes.\n");
	return(SDC_FAIL);
    }
    
    info->speed = baud_rate;
    return(setBaudRateFinish(info));
}

#    endif /* HAVE_TERMIO_H */
#endif /* HAVE_TERMIOS_H */
