#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "barbie.h"

#include <gpio.h>

#ifndef BUILD_PROGRAM
#include "../src/gphoto.h"
#endif

extern char serial_port[];

char barbie_summary_text[1024];

gpio_device *dev=NULL;
gpio_device_settings settings;

/* packet headers/footers */
char packet_header[3]           = {0x02, 0x01};
char packet_header_data[6]      = {0x02, 0x01, 0x01, 0x01, 0x01}; 
char packet_header_firmware[4]  = {0x02, 'V', 0x01};
char packet_footer[2]           = {0x03};

/* Some simple packet templates */
char packet_1[4]                = {0x02, 0x01, 0x01, 0x03};
char packet_2[5]                = {0x02, 0x01, 0x01, 0x01, 0x03};

/* Utility Functions
   =======================================================================
*/

void barbie_packet_dump(int direction, char *buf, int size) {
	int x;

	if (direction == 0)
		printf("\tRead  Packet (%i): ", size);
	   else
		printf("\tWrite Packet (%i): ", size);
	for (x=0; x<size; x++) {
		if (isalpha(buf[x]))
			printf("[ '%c' ] ", (unsigned char)buf[x]);
		   else
			printf("[ x%02x ] ", (unsigned char)buf[x]);
	}
	printf("\n");
}

int barbie_write_command(char *command, int size) {

	int x;

	barbie_packet_dump(1, command, size);
	x=gpio_write(dev, command, size);
	return (x == GPIO_OK);
}

int barbie_read_response(char *response, int size) {

	int x;
	char ack = 0;

	/* Read the ACK */
	x=gpio_read(dev, &ack, 1);
#ifdef BARBIE_DEBUG
	barbie_packet_dump(0, &ack, 1);
#endif
	if ((ack != ACK)||(x<0))
		return (0);

	/* Read the Response */
	memset(response, 0, size);
	x=gpio_read(dev,response, size);
#ifdef BARBIE_DEBUG
	barbie_packet_dump(0, response, x);
#endif
	return (x > 0);
}

int barbie_exchange (char *cmd, int cmd_size, char *resp, int resp_size) {

	int count = 0;

	while (count++ < 10) {
		if (barbie_write_command(cmd, cmd_size) != 1)
			return (0);
		if (barbie_read_response(resp, resp_size) != 1)
			return (0);
		/* if it's not busy, return */
		if (resp[RESPONSE_BYTE] != '!')
			return (1);
		/* if busy, sleep 2 seconds */
		sleep(2);
	}

	return (0);
}

int barbie_ping() {

	char cmd[4], resp[4];

#ifdef BARBIE_DEBUG
	printf("Pinging the camera\n");
#endif

	memcpy(cmd, packet_1, 4);
	cmd[COMMAND_BYTE] = 'E';
	cmd[DATA1_BYTE]   = 'x';

	if (barbie_exchange(cmd, 4, resp, 4) == 0)
		return (0);

	if (resp[DATA1_BYTE] != 'x')
		return (0);

	return (1);
}

char *barbie_read_firmware() {

	char cmd[4];
	int x;
	
	memcpy(cmd, packet_1, 4);
	cmd[COMMAND_BYTE] = 'V';
	cmd[DATA1_BYTE]   = '0';
	
	return (barbie_read_data(cmd, 4, BARBIE_DATA_FIRMWARE, &x));
}

char *barbie_read_picture(int picture_number, int get_thumbnail, int *size) {

	char cmd[4], resp[4];

	memcpy(cmd, packet_1, 4);
	cmd[COMMAND_BYTE] = 'A';
	cmd[DATA1_BYTE]   = picture_number;

	if (barbie_exchange(cmd, 4, resp, 4) != 1)
		return (NULL);
	
	memcpy(cmd, packet_1, 4);
	if (get_thumbnail)
		cmd[COMMAND_BYTE] = 'M';
	   else
		cmd[COMMAND_BYTE] = 'U';

	cmd[DATA1_BYTE] = 0;

	return (barbie_read_data(cmd, 4, BARBIE_DATA_PICTURE, size));
}

char *barbie_read_data (char *cmd, int cmd_size, int data_type, int *size) {

	char c, resp[4];
	int n1, n2, n3, n4, x, y, z;
	unsigned char r, g, b;
	char *s = NULL, *us = NULL, *rg = NULL;
	char *ppmhead_t = "P6\n# test.ppm\n%i %i\n255\n";
	char ppmhead[64];

	if (barbie_exchange(cmd, cmd_size, resp, 4) != 1)
		return (0);
	switch (data_type) {
		case BARBIE_DATA_FIRMWARE:
printf("Getting Firmware\n");
			/* we're getting the firmware revision */
			*size = resp[2];
			s = (char *)malloc(sizeof(char)*(*size));
			memset(s, 0, *size);
			s[0] = resp[3];
			if (gpio_read(dev, &s[1], (*size)-1) < 0) {
				free(s);
				return (NULL);
			}
			break;
		case BARBIE_DATA_PICTURE:
printf("Getting Picture\n");
			/* we're getting a picture */
			n1 = (unsigned char)resp[2];
			n2 = (unsigned char)resp[3];
			if (gpio_read(dev, &c, 1) < 0)
				return (NULL);
			n3 = (unsigned char)c;
			if (gpio_read(dev, &c, 1) < 0)
				return (NULL);
			n4 = (unsigned char)c;
			*size = PICTURE_SIZE(n1, n2, n3, n4);
printf("\tn1=%i n2=%i n3=%i n4=%i size=%i\n", n1, n2 ,n3, n4, *size);
			sprintf(ppmhead, ppmhead_t, n1-1, (n2+n3-1));
			us = (char *)malloc(sizeof(char)*(*size));
			rg = (char *)malloc(sizeof(char)*(*size));
			s  = (char *)malloc(sizeof(char)*(n1-1)*(n2+n3-1)*3+strlen(ppmhead));
			memset(us, 0, *size);
			memset(rg, 0, *size);
			memset(s , 0, *size+strlen(ppmhead));
			if (gpio_read(dev, us, *size)<0) {
				free(us);
				free(rg);
				free(s);
				return (NULL);
			}
			/* Unshuffle the data */
			*size = *size - 16;
			for (x=0; x<(n2+n3); x++) {
				for (y=0; y<n1; y++) {
					z = x*n1 + y/2 + y%2*(n1/2+2);
					rg[x*n1+y] = us[z];
				}
			}
			/* Camera uses Bayen array:
			 *		bg  bg   ...
			 *		gr  gr   ...
			 */
			strcpy(s, ppmhead);
			z = strlen(s);
			for (x=0; x<(n2+n3-1); x++) {
				for (y=0; y<(n1-1); y++) {
					b = (unsigned char)rg[x*n1+y];
					g = (((unsigned char)rg[(x+1)*n1+y] + 
					      (unsigned char)rg[x*n1+y+1]) / 2);
					r = (unsigned char)rg[(x+1)*n1+y+1];
					s[z++] = r;
					s[z++] = g;
					s[z++] = b;
				}
			}
			*size = z;
printf("size=%i\n", *size);
			break;
		case BARBIE_DATA_THUMBNAIL:
			break;
		default:
			break;
	}
	/* read the footer */
	if (gpio_read(dev, &c, 1) < 0) {
		free(us);
		free(rg);
		free(s);
		return (0);
	}
	free(us);
	free(rg);
	return(s);
}

/* gPhoto Functions
   =======================================================================
*/

int barbie_initialize(void) {

#ifdef BARBIE_DEBUG
	printf("Initializing the camera\n");
#endif

	if (dev) {
		gpio_free(dev);
		gpio_close(dev);
	}
	dev = gpio_new(GPIO_DEVICE_SERIAL);
	gpio_set_timeout(dev, 5000);

	strcpy(settings.serial.port, serial_port);

	settings.serial.speed	= 57600;
	settings.serial.bits	= 8;
	settings.serial.parity	= 0;
	settings.serial.stopbits= 1;

	gpio_set_settings(dev, settings);

	gpio_open(dev);

	return (barbie_ping());
}

struct Image *barbie_get_picture(int index, int thumbnail)
{
#ifndef BUILD_PROGRAM
	struct Image *im = NULL;
	int size;
#ifdef BARBIE_DEBUG
	printf("Getting a picture\n");
#endif
	im = (struct Image*)malloc(sizeof(struct Image));

	update_progress(0.00);
	im->image 	= barbie_read_picture(index, thumbnail, &size);;
	im->image_size  = size;
	im->image_info 	= NULL;
	im->image_info_size = 0;
	strcpy(im->image_type, "ppm");

	return im;
#else
	return NULL;
#endif
}

struct Image *barbie_get_preview(void) {
#ifdef BARBIE_DEBUG
	printf("Getting a preview\n");
#endif
	return NULL;
}

int barbie_delete_picture(int index) {

#ifdef BARBIE_DEBUG
	printf("Deleting a picture\n");
#endif
	return 0;
}

int barbie_take_picture(void) {

	char cmd[4], resp[4];
#ifdef BARBIE_DEBUG
	printf("Taking a picture\n");
#endif
	memcpy(cmd, packet_1, 4);

	/* Initiated the grab */
	cmd[COMMAND_BYTE] = 'G';
	cmd[DATA1_BYTE]   = 0x40;
	if (barbie_exchange(cmd, 4, resp, 4) == 0)
		return (0);

	/* Get the response (error code) to the grab */
	cmd[COMMAND_BYTE] = 'Y';
	cmd[DATA1_BYTE]   = 0;
	if (barbie_exchange(cmd, 4, resp, 4) == 0)
		return (0);

	return(resp[DATA1_BYTE] == 0);
}

int barbie_number_of_pictures(void) {
	char cmd[4], resp[4];
#ifdef BARBIE_DEBUG
	printf("Getting the number of pictures\n");
#endif
	memcpy(cmd, packet_1, 4);

	cmd[COMMAND_BYTE] = 'I';
	cmd[DATA1_BYTE]   = 0;

	if (barbie_exchange(cmd, 4, resp, 4) != 1)
		return (0);

	return (resp[DATA1_BYTE]);
}

int barbie_configure(void) {
#ifdef BARBIE_DEBUG
	printf("Configuring the camera\n");
#endif
	return 0;
}

char *barbie_summary(void) {
	int num;
	char *firm;

	num = barbie_number_of_pictures();
	firm = barbie_read_firmware();

	sprintf(barbie_summary_text, 
"Number of pictures: %i
Firmware Version: %s", num,firm);

	free(firm);

	return (barbie_summary_text);
}

char *barbie_description(void) {
	return (
"Barbie/HotWheels/WWF
Scott Fritzinger <scottf@unr.edu>
Andreas Meyer <ahm@spies.com>
Pete Zaitcev <zaitcev@metabyte.com>

Reverse engineering of image data by:
Jeff Laing <jeffl@SPATIALinfo.com>

Implemented using documents found on
the web. Permission given by Vision.");
}

#ifndef BUILD_PROGRAM
struct _Camera barbie = {
	barbie_initialize,
	barbie_get_picture,
	barbie_get_preview,
	barbie_delete_picture,
	barbie_take_picture,
	barbie_number_of_pictures,
	barbie_configure,
	barbie_summary,
	barbie_description,
	NULL
};
#endif
