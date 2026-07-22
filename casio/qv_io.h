#ifndef _QV_IO_H
#define _QV_IO_H

extern void casio_qv_reset_checksum();
extern int casio_qv_send_byte(sdcInfo info, unsigned char c);
extern int casio_qv_write(sdcInfo info, unsigned char *buf, int len);
extern int casio_qv_read(sdcInfo info, unsigned char *buf, int len);
extern int casio_qv_read_all(sdcInfo info, unsigned char *buf, int *len);
extern int casio_qv_confirm_checksum(sdcInfo info, unsigned char c);

#define RESET_CHECKSUM() casio_qv_reset_checksum()

#endif /* _QV_IO_H */
