#ifndef __JD11_SERIAL_H
#define __JD11_SERIAL_H
extern int serial_index_reader(int fd,unsigned char **indexbuf);
extern int serial_image_reader(int fd,int nr,unsigned char ***imagebufs,int *sizes);
extern int serial_open(char *device);
extern void serial_close(int fd);
#endif
