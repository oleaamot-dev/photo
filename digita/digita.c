/*
 * digita.c
 *
 * Copyright 1999-2000 Johannes Erdfelt
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <termios.h>

#include "digita.h"

#include "../src/gphoto.h"

#include <gpio.h>

static struct digita_device *dev = NULL;

int (*digita_send)(struct digita_device *dev, void *buffer, int buflen) = NULL;
int (*digita_read)(struct digita_device *dev, void *buffer, int buflen) = NULL;

int digita_initialize(void)
{
	if (camera_type == GPHOTO_CAMERA_USB)
		dev = digita_usb_open();
	else
		dev = digita_serial_open();

	return dev != NULL;
}

/*
 * CC		'B' 'M'
 * LLLL		FileSize
 * CCCC		Reserved (0's)
 * LLLL		Offset from end of header to data
 * LLLL		Version (0x40 = Windows, 0x12 = OS/2)
 * LLLL		Columns
 * LLLL		Rows
 * SS		Planes
 * SS		Bits per Pixel
 *
 * LLLL		Compression (0 = None, 1 = 8bit RLE, 2 = 4bit RLE)
 * LLLL		Compressed Size
 * LLLL		xScale
 * LLLL		yScale
 * LLLL		Colors (0 = All possible)
 * LLLL		Important Colors (0 = All)
 */
char bmpheader[] = {
  'B',  'M',			/* Ident */
  0xD6, 0x32, 0x00, 0x00,	/* Filesize */
  0x00, 0x00, 0x00, 0x00,	/* Reserved */
  0x36, 0x00, 0x00, 0x00,	/* Offset */
  0x28, 0x00, 0x00, 0x00,	/* Version */
  0x50, 0x00, 0x00, 0x00,	/* Columns */
  0x36, 0x00, 0x00, 0x00,	/* Rows */
  0x01, 0x00,			/* Planes */
  0x18, 0x00,			/* Bits per pixel */

  0x00, 0x00, 0x00, 0x00,	/* Compression */
  0xA0, 0x32, 0x00, 0x00,	/* Compressed Size */
  0x00, 0x00, 0x00, 0x00,	/* xScale */
  0x00, 0x00, 0x00, 0x00,	/* yScale */
  0x00, 0x00, 0x00, 0x00,	/* Colors */
  0x00, 0x00, 0x00, 0x00	/* Important Colors */
};

#define GFD_BUFSIZE 19432
struct Image *digita_get_picture(int index, int thumbnail)
{
	struct filename fn;
	struct partial_tag tag;
	unsigned char *data;
	int i, ret, pos, len, buflen;
	static struct Image *image;

	printf("digita_get_picture\n");

	if (index > digita_num_pictures) {
		fprintf(stderr, "index %d out of range\n", index);
 		return NULL;
	}

	index--;

	printf("getting %d, %s%s\n", index, digita_file_list[index].fn.path, digita_file_list[index].fn.dosname);

	image = malloc(sizeof(*image));
	if (!image) {
		fprintf(stderr, "couldn't allocate memory\n");
		return NULL;
	}

	/* Setup the filename */
	fn.driveno = digita_file_list[index].fn.driveno;
	strcpy(fn.path, digita_file_list[index].fn.path);
	strcpy(fn.dosname, digita_file_list[index].fn.dosname);

	/* How much data we're willing to accept */
	tag.offset = htonl(0);
	tag.length = htonl(GFD_BUFSIZE);
	tag.filesize = htonl(0);

	buflen = GFD_BUFSIZE;
	data = malloc(buflen);
	if (!data) {
		fprintf(stderr, "allocating memory\n");
		return NULL;
	}
	memset(data, 0, buflen);

	update_progress(0);

	if (digita_get_file_data(dev, thumbnail, &fn, &tag, data) < 0) {
		printf("digita_get_picture: digita_get_file_data failed\n");
		return NULL;
	}

	buflen = ntohl(tag.filesize);
	if (thumbnail)
		buflen += 16 + sizeof(bmpheader);

	data = realloc(data, buflen);
	if (!data) {
		fprintf(stderr, "couldn't reallocate memory\n");
		return NULL;
	}

	len = ntohl(tag.filesize);
	pos = ntohl(tag.length);
	while (pos < len) {
		if (len)
			update_progress(100 * pos / len);
		tag.offset = htonl(pos);
		if ((len - pos) > GFD_BUFSIZE)
			tag.length = htonl(GFD_BUFSIZE);
		else
			tag.length = htonl(len - pos);

		if (digita_get_file_data(dev, thumbnail, &fn, &tag, data + pos) < 0) {
			printf("digita_get_picture: digita_get_file_data failed\n");
			return NULL;
		}
		pos += ntohl(tag.length);
	}

	update_progress(100);

	if (thumbnail) {
		unsigned char *rgb, *ps;
		unsigned int *ints, width, height;

		ints = (unsigned int *)data;
		width = ntohl(ints[2]);
		height = ntohl(ints[1]);

		rgb = malloc((width * height * 3) + sizeof(bmpheader));
		if (!rgb) {
			fprintf(stderr, "error allocating rgb data\n");
			return NULL;
		}

		memcpy(rgb, bmpheader, sizeof(bmpheader));

		ps = data + 16;
		for (i = 0; i < height; i++) {
			char *pd = rgb + sizeof(bmpheader) + (width * ((height
- 1) - i) * 3);
		while (ps < data + 16 + (width * i * 2)) {
			int y, u, y1, v, r, g, b;

#define LIMIT(x) ((((x)>0xffffff)?0xff0000:(((x)<=0xffff)?0:(x)&0xff0000))>>16)

u =  *ps++ - 128;
y =  *ps++ - 16;
v =  *ps++ - 128;
y1 = *ps++ - 16;
r = 104635 * v;
g = -25690 * u + -53294 * v;
b = 132278 * u;
y  *= 76310;
y1 *= 76310;
*pd++ = LIMIT(b + y); *pd++ = LIMIT(g + y); *pd++ = LIMIT(r + y);
*pd++ = LIMIT(b + y1); *pd++ = LIMIT(g + y1); *pd++ = LIMIT(r + y1);
		}
}

		free(data);
		
		image->image_info = NULL;
		image->image_info_size = 0;
		image->image = rgb;
		strcpy(image->image_type, "bmp");
		image->image_size = (width * height * 3) + sizeof(bmpheader);
	} else {
		image->image_info = NULL;
		image->image_info_size = 0;
		image->image = data;
		strcpy(image->image_type, "jpg");
		image->image_size = buflen;
	}

	return image;
}

struct Image *digita_get_preview(void)
{
	printf("digita_get_preview\n");
	return NULL;
}

static int delete_picture(int index)
{
	struct filename fn;
	int ret;

	printf("digita_delete_picture\n");

	if (index > digita_num_pictures)
 		return 0;

	index--;

	printf("deleting %d, %s%s\n", index, digita_file_list[index].fn.path, digita_file_list[index].fn.dosname);

	/* Setup the filename */
	fn.driveno = digita_file_list[index].fn.driveno;
	strcpy(fn.path, digita_file_list[index].fn.path);
	strcpy(fn.dosname, digita_file_list[index].fn.dosname);

	if (digita_delete_picture(dev, &fn) < 0)
		return 0;

	if (digita_get_file_list(dev) < 0)
		return 0;

	return 1;
}

int digita_take_picture(void)
{
	printf("digita_take_picture\n");
	return 0;
}

int digita_number_of_pictures(void)
{
	int taken;

	printf("digita_number_of_pictures\n");

	if (digita_get_storage_status(dev, &taken, NULL, NULL) < 0)
		return 0;

	if (digita_get_file_list(dev) < 0)
		return 0;

	return taken;
}

int digita_configure(void)
{
	printf("digita_configure\n");
	return 0;
}

char *digita_summary(void)
{
	static char buffer[256];
	int available;

	if (digita_get_storage_status(dev, NULL, &available, NULL) < 0)
		return NULL;

#if 0
	sprintf(buffer, "%s\n%d pictures available\n",
		camera->name, available);
#endif

	return buffer;
}

char *digita_description(void)
{
	return "Digita.  Copyright (c) 1999-2000, Johannes Erdfelt <johannes@erdfelt.com>";
}

struct _Camera digita = {
	digita_initialize,
	digita_get_picture,
	digita_get_preview,
	delete_picture,
	digita_take_picture,
	digita_number_of_pictures,
	digita_configure,
	digita_summary,
	digita_description,
	NULL
};
