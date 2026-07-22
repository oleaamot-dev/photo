#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#include "../src/gphoto.h"

#include "serial.h"
#include "decomp.h"

struct Image*
jd11_get_picture(int picnum,int thumbnail) {
        struct Image	*image;
	int		nrofpics;
	int		h,fd;
	unsigned char	*indexbuf,*uncomp[3],**imagebufs;
	int		sizes[3];
	char		*s;

	if (picnum==0) {
	    fprintf(stderr,"jd11_get_picture: picnum 0???\n");
	    assert(picnum);
	}
	picnum--;
	fd=serial_open(serial_port);
	if (fd==-1) {
	    return NULL;
	}
	nrofpics = serial_index_reader(fd,&indexbuf);
	if (!nrofpics || (picnum>nrofpics)) {
	    return NULL;
	}

	image = (struct Image*)malloc(sizeof(struct Image));
	memset(image,0,sizeof(*image));

	if (thumbnail) {
	    	int ind;

		image->image = malloc(1*64*48*5+200); /* guessing imagesize */
		strcpy(image->image,"P2\n64 48\n255\n");
		s=image->image+strlen(image->image);
		ind = picnum*64*48;
		for (h=48;h--;) { /* upside down */
		    int w;
		    for (w=64;w--;) { /* and right to left */
			sprintf(s,"%d ",indexbuf[ind+(h*64)+w]);s+=strlen(s);
			if ((w&0xf)==0xf)
			    	*s++='\n';
		    }
		}
		*s++='\0';
		image->image_size = strlen(image->image);
		strcpy(image->image_type,"ppm");
		free(indexbuf);
		return image;
	}
	free(indexbuf);
	serial_image_reader(fd,picnum,&imagebufs,sizes);
	uncomp[0] = malloc(320*480);
	uncomp[1] = malloc(320*480/2);
	uncomp[2] = malloc(320*480/2);
	if (sizes[0]!=115200) {
		picture_decomp_v1(imagebufs[0],uncomp[0],320,480);
		picture_decomp_v1(imagebufs[1],uncomp[1],320,480/2);
		picture_decomp_v1(imagebufs[2],uncomp[2],320,480/2);
	} else {
		picture_decomp_v2(imagebufs[0],uncomp[0],320,480);
		picture_decomp_v2(imagebufs[1],uncomp[1],320,480/2);
		picture_decomp_v2(imagebufs[2],uncomp[2],320,480/2);
	}

	image->image = malloc(1*640*480*3+200); /* guessing imagesize */
	strcpy(image->image,"P6\n640 480\n255\n");
	s=image->image+strlen(image->image);
	image->image_size = strlen(image->image)+640*480*3;
	for (h=480;h--;) { /* upside down */
	    int w;
	    for (w=640;w--;) { /* right to left */
		/* and images are in green red blue */
		*s++=uncomp[1][(h/2)*320+(w/2)];
		*s++=uncomp[0][h*320+(w/2)];
		*s++=uncomp[2][(h/2)*320+(w/2)];
	    }
	}
	free(uncomp[0]);free(uncomp[1]);free(uncomp[2]);
	free(imagebufs[0]);free(imagebufs[1]);free(imagebufs[2]);free(imagebufs);
	strcpy(image->image_type,"ppm");
	serial_close(fd);
	return image;
}

int
jd11_number_of_pictures() {
	int		nrofpics;
	int		fd;
	unsigned char		*indexbuf;

	fd=serial_open(serial_port);
	if (fd==-1)
	    	return 0;
	nrofpics = serial_index_reader(fd,&indexbuf);
	free(indexbuf);
	serial_close(fd);
	return nrofpics;
}

static char *jd11_summary(void) {
    return "JenOptics Digital JenCam JD11.\nNo more Summary.\n";
}

static char *jd11_description(void)
{
    return  "JenCam JD11 driver by\n"
	    "Marcus Meissner <marcus.meissner@caldera.de>\n";
}    

static int jd11_delete_image(int nr) {
    fprintf(stderr,"jd11_delete_image(%d): not implemented yet.\n",nr);
    return 0;
}

static struct Image* jd11_get_preview(void) {
    return NULL;
}

static int jd11_take_picture(void) {
    return 0;
}

static int jd11_configure(void) {
    return 0;
}

static int jd11_initialize(void) {
    return 1;
}

struct _Camera jd11 = {
    jd11_initialize,
    jd11_get_picture,
    jd11_get_preview,
    jd11_delete_image,
    jd11_take_picture,
    jd11_number_of_pictures,
    jd11_configure,
    jd11_summary,
    jd11_description,
    NULL
};
