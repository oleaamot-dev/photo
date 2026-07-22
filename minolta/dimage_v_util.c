#include "dimage_v.h"

/* Open the device, then perform all the serial magic on it, if applicable. */
gpio_device *dimage_v_open(char* dimage_v_device)
{
	gpio_device *dev;
	gpio_device_settings conf;
	int port_len=0;


	dev = gpio_new(GPIO_DEVICE_SERIAL);
	conf.serial.speed = 38400;
	conf.serial.bits = 8;
	conf.serial.parity = 0;
	conf.serial.stopbits = 1;
	port_len = strlen(dimage_v_device);
	if ( port_len >= 128 ) {
		dialog_error("Absurdly long device path!");
		return NULL;
	}
	strncpy(conf.serial.port, dimage_v_device, port_len + 1);

	/* set the serial device configuration */
	gpio_set_settings(dev, conf); 
	gpio_set_timeout(dev, 500);

	if ( gpio_open(dev) == GPIO_OK ) {
		return dev;
	} else {	
		return NULL;
	}	
}

unsigned char dimage_v_read_byte(gpio_device *dev)
{
	unsigned char value;
	if ( gpio_read(dev, &value, 1) != 1 ) {
		return GPIO_ERROR;
	} else {
		return value;
	}	
}

int dimage_v_send_byte(gpio_device *dev, unsigned char value)
{
	if ( gpio_write(dev, &value, 1) != 1 ) {
		return GPIO_ERROR;
	} else {
		return GPIO_OK;
	}	
}


/* Minolta's packets are wrong more often than not. Bastards!
return 0 if not a valid packet, non-zero if valid. */
int dimage_v_verify_packet(dimage_v_buffer* packet)
{
	int checksum=0, this_check=0, i=0, difference=0, num_ffs=0;

	/* Packets that don't end with an ETX aren't valid, period. */
	if (packet->contents[(packet->length - 1)] != ETX)
	{
		return 0;
	}

	/* Now do the checksum */
	checksum=(packet->contents[(packet->length - 3)] * 256) + packet->contents[(packet->length - 2)];

	for (i=0 ; i < packet->length - 3 ; i++ )
	{
		if (packet->contents[i] == 0xff)
		{
			num_ffs++;
		}
		this_check += packet->contents[i];
		this_check= this_check % 65536;
	}

	difference=this_check - checksum;

	if ((difference%255)==0)
	{
/*		fprintf(stderr, "Packet VERIFIED: %d %% 255 == 0\n", difference);*/
		return 1;
	}
	else if (difference < 0)
	{
		fprintf(stderr, "Negative difference - probably bad: %d\n--> packet->length = %d\n", difference, packet->length);
		return 0;
	}
	else
	{
		fprintf(stderr, "Packet FAILED: Checksum had an unknown difference: %d\n", this_check - checksum);
		return 0;
	}
}

int dimage_v_write_packet(dimage_v_buffer* packet, gpio_device *dev)
{
	int written=0;

	if (packet == NULL)
	{
		perror("dimage_v_write_packet::unable to write null packet");
		return 1;
	}

	written = gpio_write(dev, packet->contents, packet->length);
	if ( ( written == GPIO_ERROR ) || ( written != packet->length ) ) {
		return GPIO_ERROR;
	} else {
		return GPIO_OK;
	}	
}


#if EXPIRIMENTAL_READ_PACKET

dimage_v_buffer* dimage_v_read_packet(gpio_device *dev, int started)
{
	dimage_v_buffer* packet;
	unsigned char header[4], extra_byte, buffer[1024];
	int numread=0, totalread=0, i=0, lefttoread=0, num_ffs=0, total_useful_read=0;

	if ((packet=malloc(sizeof(dimage_v_buffer)))==NULL)
	{
		perror("dimage_v_read_packet::unable to allocate packet");
		return NULL;
	}

	if (started != 0)
	{
		/* We already got the first byte - just make it valid. */
		header[0]=STX;
		numread = gpio_read(dev, ( header + 1 ) , 3);
		if ( ( numread == GPIO_ERROR ) || ( numread != 3 ) ) {
			perror("dimage_v_read_packet::failed to read packet header");
			return NULL;
		}
	}
	else
	{
		/* Read the first four bytes to determine how long the packet is. */
		numread = gpio_read(dev, header, 4);
		if ( ( numread == GPIO_ERROR ) || ( numread != 4 ) ) {
			perror("dimage_v_read_packet::failed to read packet header");
			return NULL;
		}
	}

	packet->length = 0;
	packet->length += header[2] * 256;
	packet->length += header[3];

	/* Now alloc packet->contents including an extra byte or two. */
	if ((packet->contents = ( malloc((packet->length)+ 2)))==NULL)
	{
		perror("dimage_v_read_packet::unable to allocate packet->contents");
		return NULL;
	}

	/* Copy over the first four bytes, then start reading the rest. */
	memcpy(packet->contents, header, 4);

	totalread = 4;

	/* Just to clarify: totalread is the actual number of bytes read.
						numread is the number read on the last pass.
	*/					

	while ( totalread < packet->length ) {
		numread = gpio_read(dev, packet->contents + totalread, ( packet->length ) - totalread);
		if ( numread != GPIO_ERROR ) {
			totalread+=numread;
		} else {
			fprintf(stderr, "Got GPIO_ERROR reading\n");
			fflush(stderr);
			usleep(1000);
		}

	}

	if (dimage_v_verify_packet(packet)==0)
	{
		/* Maybe I can get rid of this soon... */
		fprintf(stderr, "Got a bad packet after reading:\n--> packet->length == %d\totalread == %d\n", packet->length, totalread);
		fflush(stderr);
	}
	dimage_v_dump_packet(packet);
	return packet;
}

#else /* EXPIRIMENTAL_READ_PACKET */

dimage_v_buffer* dimage_v_read_packet(gpio_device *dev, int started)
{
	dimage_v_buffer* packet;
	unsigned char header[4], extra_byte, buffer[1024];
	int numread=0, totalread=0, i=0, lefttoread=0, last_ff=0, no_read=0, num_ffs=0;

	if ((packet=malloc(sizeof(dimage_v_buffer)))==NULL)
	{
		perror("dimage_v_read_packet::unable to allocate packet");
		return NULL;
	}

	if (started != 0)
	{
		/* We already got the first byte - just make it valid. */
		header[0]=STX;
		numread = gpio_read(dev, ( header + 1 ) , 3);
		if ( ( numread == GPIO_ERROR ) || ( numread != 3 ) ) {
			perror("dimage_v_read_packet::failed to read packet header");
			return NULL;
		}
	}
	else
	{
		/* Read the first four bytes to determine how long the packet is. */
		numread = gpio_read(dev, header, 4);
		if ( ( numread == GPIO_ERROR ) || ( numread != 4 ) ) {
			perror("dimage_v_read_packet::failed to read packet header");
			return NULL;
		}
	}

	packet->length = 0;
	packet->length += header[2] * 256;
	packet->length += header[3];

	/* Now alloc packet->contents including an extra buffer. Thanks Minolta! */
	if ((packet->contents = ( malloc((packet->length)+ 24)))==NULL)
	{
		perror("dimage_v_read_packet::unable to allocate packet->contents");
		return NULL;
	}

	/* Copy over the first four bytes, then start reading the rest. */
	memcpy(packet->contents, header, 4);

	last_ff=0;
	totalread=4;

	while ( ( numread = gpio_read(dev, &extra_byte, 1)) != GPIO_ERROR )
	{
		switch (extra_byte)
		{
			case 0xff:
				if (last_ff==0)
				{
					packet->contents[totalread++]=extra_byte;
					last_ff=1;
				}
				else
				{
					last_ff=0;
				}
				break;
			default:
				last_ff=0;
				packet->contents[totalread++]=extra_byte;
				break;
		}
	}

	while (dimage_v_verify_packet(packet)==0)
	{
		/* Maybe I can get rid of this soon... */
		fprintf(stderr, "Got a bad packet after reading:\n--> packet->length == %d\ttotalread == %d\n", packet->length, totalread);
		fflush(stderr);
		numread = gpio_read(dev, &extra_byte, 1);
		if (numread == 0)
		{
			no_read++;
		}
		else
		{
			fprintf(stderr, "Got an extra byte\n");
			fflush(stderr);
			packet->contents[totalread++]=extra_byte;
		}

		if (no_read > MAX_BAD_READS )
		{
			fprintf(stderr, "Giving up on this packet after %d bad reads\n", MAX_BAD_READS);
			fflush(stderr);
			return packet;
		}
		
	}
	dimage_v_dump_packet(packet);
	return packet;
}
#endif

dimage_v_buffer* dimage_v_make_packet(unsigned char* payload, int payload_length, int seq)
{
	dimage_v_buffer* packet;
	int i=0, checksum=0;

	if ((packet=malloc(sizeof(dimage_v_buffer)))==NULL)
	{
		perror("dimage_v_make_packet::unable to allocate packet");
		return NULL;
	}

	packet->length=(7 + payload_length);

	if ((packet->contents = malloc((8 + payload_length)))==NULL)
	{
		perror("dimage_v_make_packet::unable to allocate packet->contents");
		return 0;
	}

	/* Make the header. */
	packet->contents[0]=STX;
	packet->contents[1]=seq & 0x000000ff;
	packet->contents[2]=(packet->length & 0x0000ff00) >> 8;
	packet->contents[3]=packet->length & 0x000000ff;

	/* Copy in the payload. */
	for (i = 0 ; i < payload_length ; i++ )
	{
		packet->contents[(i + 4)] = payload[i];
	}

	/* Now the footer. */
	for (i=0 ; i < packet->length - 3 ; i++ )
	{
		checksum += packet->contents[i];
	}
	packet->contents[(packet->length - 3)]=(checksum & 0x0000ff00) >> 8;
	packet->contents[(packet->length - 2)]=checksum & 0x000000ff;
	packet->contents[(packet->length - 1)]=ETX;

	return packet;
}

void dimage_v_delete_packet(dimage_v_buffer* packet)
{
	if (packet->contents != NULL)
	{
		free(packet->contents);
	}

	if (packet != NULL)
	{
		free(packet);
	}
	return;
}

unsigned char dimage_v_decimal_to_bcd(unsigned char decimal)
{
	unsigned char bcd=0;
	int tens_digit=0;

	if (decimal > 99)
	{
		/* Tough. */
		return 0;
	}

	tens_digit=decimal/10;
	bcd=tens_digit*16;
	bcd+=decimal%10;

	return bcd;
}

unsigned char dimage_v_bcd_to_decimal(unsigned char bcd)
{
	if (bcd > 0x99)
	{
		return 99;
	}
	else
	{
		return ((bcd/16)*10 + (bcd%16));
	}
}

dimage_v_buffer* dimage_v_strip_packet(dimage_v_buffer* packet)
{
	dimage_v_buffer* payload;
	int i=0;

	if ((payload=malloc(sizeof(dimage_v_buffer)))==NULL)
	{
		perror("dimage_v_strip_packet::unable to allocate packet");
		return NULL;
	}

	payload->length = packet->length -7;

	if ((payload->contents=malloc(payload->length))==NULL)
	{
		perror("dimage_v_strip_packet::unable to allocate packet-contents");
		return NULL;
	}

	for (i=0; i< payload->length ; i++)
	{
		payload->contents[i]=packet->contents[i+4];
	}

	return payload;
}

char* dimage_v_write_picture_to_file(int picture_number)
{
	gpio_device *dev;

	char *tmpfile;
	FILE* imagefile;
	int total_packets=0, packets_gotten=0;
	unsigned char tmp=0, get_cmd[3]="\x04\x00\x00";
	dimage_v_buffer *packet, *payload;

	if ((tmpfile=malloc(256))==NULL)
	{
		perror("dimage_v_write_picture_to_file::unable to allocate tmpfile");
		return NULL;
	}

	if ((tmpnam(tmpfile))==NULL)
	{
		perror("dimage_v_write_picture_to_file::unable to generate temporary filename");
		return NULL;
	}

	if ((imagefile=fopen(tmpfile, "w"))==NULL)
	{
		perror("dimage_v_write_picture_to_file::unable to create temporary file");
		return NULL;
	}

	if ((dev = dimage_v_open(serial_port)) == NULL)
	{
		dialog_error("Unable to access serial_port");
		return NULL;
	}

	/* Now we connect to the cam, query this image number, and then save it. */

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

	update_status("Entered host mode(tm)");

	/* What a great time to get an image. */
	get_cmd[0]='\x04';
	get_cmd[1]=(picture_number/256);
	get_cmd[2]=(picture_number%256);

	packet=dimage_v_make_packet(get_cmd, 3, 0);
	dimage_v_write_packet(packet, dev);
	switch(dimage_v_read_byte(dev))
	{
		case ACK:
			update_status("Recieved ACK");
			fprintf(stderr,"Got the ACK.\n");
			break;
		case NAK: case CAN: default:
			dialog_error("Unable to get image");
			return NULL;
			break;
	}

	dimage_v_delete_packet(packet);


	packet=dimage_v_read_packet(dev, 0);
	payload=dimage_v_strip_packet(packet);
	total_packets=payload->contents[0];
	fprintf(stderr, "Payload length is %d, total packets should be %d\n", payload->length, payload->contents[0]);
/*	fprintf(stderr, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", payload->contents[0], payload->contents[1], payload->contents[2], payload->contents[3], payload->contents[4], payload->contents[5], payload->contents[6], payload->contents[7], payload->contents[8], payload->contents[9], payload->contents[10]);*/
	fflush(stderr);
	fwrite(&(payload->contents[1]), 1, payload->length -1 , imagefile);
	dimage_v_delete_packet(packet);
	fflush(imagefile);
	dimage_v_delete_packet(payload);

	if (total_packets<1)
	{
		dialog_error("Supposed to get less than one packet?\n");
/*		fprintf(stderr, "Supposed to get less than one packet? WTF?\n");*/
		return NULL;
	}

	for (packets_gotten=1; packets_gotten < total_packets; packets_gotten++)
	{
		dimage_v_send_byte(dev, ACK);
		while((tmp=dimage_v_read_byte(dev))!=STX)
		{
			fprintf(stderr, "Missed a byte... %02x\n", tmp);
			fflush(stderr);
		}

		packet = dimage_v_read_packet(dev, 1);
		payload = dimage_v_strip_packet(packet);
		if (total_packets)
			update_progress((float)packets_gotten / 
					(float)total_packets);
		fwrite(payload->contents, 1, payload->length, imagefile);
		fflush(imagefile);
/*		gdk_flush();*/
		dimage_v_delete_packet(packet);
		dimage_v_delete_packet(payload);
	}
	dimage_v_send_byte(dev, EOT);
	fprintf(stderr, "Sent the EOT\n");
	while((tmp=dimage_v_read_byte(dev))!=ACK){fprintf(stderr, "Waiting for an ACK\n");};
	fprintf(stderr, "Got the ACK. CLose up shop.\n");

	/* Now that the whole file is written, close it, and load the image. */
	if (fclose(imagefile) != 0)
	{
		perror("What? Can't close it?");
		return NULL;
	}

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

	/* Now enter host mode */
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

	return tmpfile;
}

void dimage_v_dump_packet(dimage_v_buffer* packet) {
#ifdef DIMAGE_V_DEBUG
	int i=0;

	fprintf(debug, "packet->length == %d\n");
	for ( i = 0 ; i < packet->length ; i++ ) {
		fprintf(debug, "%02x ", packet->contents[i]);
	}

	fprintf(debug, "\n\n");
	fflush(debug);
#endif
}
