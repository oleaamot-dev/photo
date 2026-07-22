#include "dimage_v.h"

int dimage_v_initialize()
{
	return 1;
}

struct Image *dimage_v_get_picture(int picture_number, int thumbnail)
{
	char *tmpfile;
	struct Image *image, *bogus;
	struct stat filestats;
	FILE *imagefile;

	/* Create a bogus image here. */
	if ((bogus=malloc(sizeof(struct Image)))==NULL)
	{
		perror("dimage_v_get_picture::unable to allocate bogus image");
		return NULL;
	}

	bogus->image_size=NO_THUMBS_LENGTH;
	strcpy(bogus->image_type, NO_THUMBS_TYPE);
	bogus->image=NO_THUMBS;
	bogus->image_info_size=0;

	/* No thumbnail support, since they're in an unknown bitmap format. */
	if (thumbnail != 0)
	{
		return bogus;
	}

	if ((tmpfile=dimage_v_write_picture_to_file(picture_number))==NULL)
	{
		fprintf(stderr, "dimage_v_get_picture::returning a bogus image.\n");
		return bogus;
	}
	else
	{
		/* Now load the error image if any trouble. */
	
		if ((image=malloc(sizeof(struct Image)))==NULL)
		{
			perror("dimage_v_get_picture::unable to allocate image");
			return NULL;
		}

		if (stat(tmpfile, &filestats)<0)
		{
			perror("dimage_v_get_picture::unable to stat file");
			return bogus;
		}

		if (filestats.st_size <= 0)
		{
			fprintf(stderr, "dimage_v_get_picture::stat() claimed zero file size.\n");
			return bogus;
		}
		else
		{
			image->image = (char *)malloc(sizeof(char)*(filestats.st_size+2));
			image->image_size=filestats.st_size;
		}

		/* Someday we'll fix the attributes. */
		image->image_info_size=0;

		strcpy(image->image_type, "jpg");

		if ((imagefile=fopen(tmpfile, "r"))==NULL)
		{
			perror("dimage_v_get_picture::unable to open file");
		}

		fread(image->image, (size_t)sizeof(char), (size_t)filestats.st_size, imagefile);
		fclose(imagefile);

		unlink(tmpfile);
	}

	return image;
}

struct Image *dimage_v_get_preview()
{
	return NULL;
}

int dimage_v_delete_image(int picture_number)
{
	gpio_device *dev;
	int dimage_v_fd=-1;
	unsigned char tmp=0, del_cmd[3]="\x05\x00\x00";
	dimage_v_buffer *packet, *payload;

	if ((dev=dimage_v_open(serial_port))<0)
	{
		dialog_error("Unable to access serial_port");
		return 0;
	}

	/* Now we connect to the cam, query this image number, and then delete it. */

	/* Find out what we're set to now. */
	packet=dimage_v_make_packet("\x09", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	packet=dimage_v_read_packet(dev,0);
	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);
	usleep(100);

	/* Now enter host mode */
	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[0]= 0x82;
	payload->contents[8]= 0x00;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[8]= 0x81;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	fprintf(stderr, "We've entered host mode(tm)\n");
	fflush(stderr);

	/* What a great time to get an image. */
	del_cmd[0]='\x05';
	del_cmd[1]=(picture_number/256);
	del_cmd[2]=(picture_number%256);

	fprintf(stderr, "Preparing to delete an image, with command: %02x %02x %02x\n", del_cmd[0], del_cmd[1], del_cmd[2]);
	fflush(stderr);

	packet=dimage_v_make_packet(del_cmd, 3, 0);
	dimage_v_write_packet(packet, dev);
	switch(dimage_v_read_byte(dev))
	{
		case ACK:
			fprintf(stderr,"Got the ACK.\n");
			break;
		case NAK: case CAN: default:
			dialog_error("Unable to delete image");
			fprintf(stderr,"Unable to delete image %d\n", picture_number);
			return 0;
			break;
	}

	dimage_v_delete_packet(packet);

	packet=dimage_v_read_packet(dev, 0);
	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);
	dimage_v_delete_packet(payload);

	dimage_v_send_byte(dev, EOT);
	fprintf(stderr, "Sent the EOT\n");
	while((tmp=dimage_v_read_byte(dev))!=ACK){fprintf(stderr, "Waiting for an ACK\n");};
	fprintf(stderr, "Got the ACK. CLose up shop.\n");

	/* Now leave host mode. */
	packet=dimage_v_make_packet("\x09", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	packet=dimage_v_read_packet(dev,0);
	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);
	usleep(100);

	/* Now leave host mode */
	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[0]=payload->contents[0] - 0x80;
	payload->contents[8]= 0x00;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(payload);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	gpio_close(dev);
	gpio_free(dev);

	return 1;
}

int dimage_v_take_picture()
{
	gpio_device *dev;
	int dimage_v_fd=-1;
	dimage_v_buffer *packet, *payload;

	if ((dev = dimage_v_open(serial_port))<0)
	{
		dialog_error("Unable to access serial_port");
		return 0;
	}

	/* Find out what we're set to now. */
	packet=dimage_v_make_packet("\x09", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	packet=dimage_v_read_packet(dev,0);
	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);
	usleep(100);

	/* Now enter host mode */
	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[0]=payload->contents[0] | 0x80;
	payload->contents[8]= 0x00;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[0]=payload->contents[0] | 0x80;
	payload->contents[8]= 0x81;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	update_status("We've entered host mode(tm)");

	/* Leaving host mode. */
	fprintf(stderr, "We're leaving host mode(tm)\n");
	fflush(stderr);

	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	packet=dimage_v_read_packet(dev,0);
	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);
	usleep(100);

	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[0]=payload->contents[0] - 0x80;
	payload->contents[8]= 0x00;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	gpio_close(dev);
	gpio_free(dev);

	return 0;
}

int dimage_v_number_of_pictures()
{
	gpio_device *dev;
	int minoltafd=0, numpics=0;
	unsigned char response=0;
	dimage_v_buffer *packet, *payload;

	if ((dev = dimage_v_open(serial_port))< 0)
	{
		fprintf(stderr, "dimage_v_number_of_pictures::unable to open serial_port");
		return 0;
	}

	if ((packet=dimage_v_make_packet("\x07", 1, 0))==NULL)
	{
		fprintf(stderr, "dimage_v_number_of_pictures::unable to make packet");
		return 0;
	}

	dimage_v_write_packet(packet, dev);
	response = dimage_v_read_byte(dev);
	/* This might not need to be a switch now, but someday ... */
	switch (response)
	{
		case ACK:
			break;
		case NAK: case CAN: default:
		dialog_error("Bad response form camera while querying number of pictures");
			return 0;
			break;
	}

	dimage_v_delete_packet(packet);
	packet=dimage_v_read_packet(dev, 0);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);

	numpics = payload->contents[1] * 256;
	numpics += payload->contents[2]; 

	dimage_v_delete_packet(payload);

	/* Now leave host mode. */
	packet=dimage_v_make_packet("\x09", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	packet=dimage_v_read_packet(dev,0);
	payload=dimage_v_strip_packet(packet);
	dimage_v_delete_packet(packet);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);
	usleep(100);

	/* Now leave host mode */
	packet=dimage_v_make_packet("\x08", 1, 0);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	payload->contents[0]=payload->contents[0] - 0x80;
	payload->contents[8]= 0x00;
	packet=dimage_v_make_packet(payload->contents, payload->length, 1);
	dimage_v_write_packet(packet, dev);
	dimage_v_delete_packet(payload);
	dimage_v_delete_packet(packet);
	while (dimage_v_read_byte(dev)!=ACK);
	dimage_v_send_byte(dev, EOT);
	while (dimage_v_read_byte(dev)!=ACK);

	gpio_close(dev);
	gpio_free(dev);

	return numpics;
}

int dimage_v_configure()
{
	return 1;
}

char* dimage_v_summary()
{
	return "";
}

char* dimage_v_description()
{
	return "Minolta Dimage V gPhoto Library (c)\nGus Hartmann <gus@upl.cs.wisc.edu>\n* Thumbnails do not work - unknown bitmap type.";
}

struct _Camera dimage_v = {	dimage_v_initialize,
							dimage_v_get_picture,
							dimage_v_get_preview,
							dimage_v_delete_image,
							dimage_v_take_picture,
							dimage_v_number_of_pictures,
							dimage_v_configure,
							dimage_v_summary,
							dimage_v_description,
							NULL};
