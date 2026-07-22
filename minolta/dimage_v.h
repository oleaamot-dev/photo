#ifndef __MINOLTA_H__

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "../src/gphoto.h"
#include "../src/util.h"
#include "no_thumbs.h"

#include <gpio.h>

#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define MAX_BAD_READS 5

#if DIMAGE_V_DEBUG
FILE* debug;
#endif

/* Make a struct for storing char arrays. Since they may contain NULLs,
   stuff in strings.h can't be used.
*/
typedef struct _dimage_v_buffer
{
	int length;
	unsigned char* contents;
} dimage_v_buffer;

/* GPhoto globals */
extern char	serial_port[20];
extern void update_status(char *newStatus);
extern void dialog_error(char *Error);

/* Globals for my own use. */
struct termios oldt, newt;

/* Prototypes for the GPhoto lib. */
int dimage_v_initialize();
char* dimage_v_summary();
char* dimage_v_description();
struct Image* dimage_v_get_picture(int picture_number, int thumbnail);
int dimage_v_configure();
struct Image* dimage_v_get_preview();
int dimage_v_take_picture();

/* Functions for our use only. */
gpio_device *dimage_v_open(char* dimage_v_device);
int dimage_v_write_packet(dimage_v_buffer* packet, gpio_device *dev);
unsigned char dimage_v_read_byte(gpio_device *dev);
int dimage_v_send_byte(gpio_device *dev, unsigned char value);
dimage_v_buffer* dimage_v_read_packet(gpio_device *dev, int started);
dimage_v_buffer* dimage_v_make_packet(unsigned char* payload, int payload_length, int seq);
void dimage_v_delete_packet(dimage_v_buffer* packet);
unsigned char dimage_v_decimal_to_bcd(unsigned char decimal);
int dimage_v_delete_image(int picture_number);
int dimage_v_number_of_pictures();
dimage_v_buffer* dimage_v_strip_packet(dimage_v_buffer* packet);
char* dimage_v_write_picture_to_file(int picture_number);

void dimage_v_dump_packet(dimage_v_buffer* packet);

#endif /* __MINOLTA_H__ */
