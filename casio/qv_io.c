#include <stdio.h>

#include "sdComm.h"
#include "casio_qv_defines.h"

static int checksum = 0;
extern int qvverbose;

void
casio_qv_reset_checksum() {
    checksum = 0;
}

static void
add_to_checksum(unsigned char c) {
    checksum += (int)c;
}

static void
add_buf_to_checksum(unsigned char *buf, int len) {
    unsigned char *c;
    int i;

    for (i = 0, c = buf; i < len; i++, c++) {
	add_to_checksum(*c);
    }
}

int
casio_qv_send_byte(sdcInfo info, unsigned char c) {
    add_to_checksum(c);
    return(sdcSendByte(info, c));
}

int
casio_qv_write(sdcInfo info, unsigned char *buf, int len) {
    add_buf_to_checksum(buf, len);
    return(sdcWrite(info, buf, len));
}

int
casio_qv_read(sdcInfo info, unsigned char *buf, int len) {
    return(sdcRead(info, buf, len));
}

int
casio_qv_read_all(sdcInfo info, unsigned char *buf, int *len) {
    return(sdcReadAll(info, buf, len));
}

int
casio_qv_confirm_checksum(sdcInfo info, unsigned char c) {
    unsigned char p;

    p = 0xFF & ~checksum;
    if (c != p && c == ACK) {
	if (qvverbose)
	    fprintf(stderr, "Compensating for potential loss of data synchronization\n");
	casio_qv_read(info, &c, 1);
    }
    return(c == p);
}
