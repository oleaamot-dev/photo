#include "dimage_v.h"
#include <gpio.h>


/* The below declarations emulate the functiuonality provided by GPhoto.
   This allows me to test my libs, or to simply have a lightweight utility
   without relying on the rest of gphoto. This is very nice when working
   in the development tree.
*/   
char serial_port[20];

void update_progress(float percentage) {
	fprintf(stderr,"Retrieval at %f\n", percentage);
	return;
}

void update_status(char *newStatus) {
	fprintf(stderr,"Status message: %s\n",newStatus);
	return;
}

void dialog_error(char *Error) {
	fprintf(stderr, "Error message: %s\n",Error);
	return;
}

void usage_msg(char* argv0) {
	fprintf(stderr, "Usage:\n\t%s [-a | -n num ] [ -d ] [ -f prefix ] [ -p port ]\n", argv0);
	fflush(stderr);
	return;
}	

int main(int argc, char** argv) {
	gpio_device *dev;
	int num = -1, target_num = -1, delete_flag = 0, i = 0 ;
	char *file_prefix=NULL, *port=NULL, *filename, *srcnamebuffer, *dstnamebuffer;

#if HAVE_GETOPT || HAVE_GETOPT_LONG
	int value, opt = 0, error_flag = 0;
	extern int opterr, optind;
	extern char* optarg;

#if HAVE_GETOPT_LONG
	static struct option long_options[] = {
		{"all", no_argument, 0, 'a'},
		{"delete", no_argument, 0, 'd'},
		{"file-prefix", required_argument, 0, 'f'},
		{"number", required_argument, 0, 'n'},
		{"port",required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	opterr = 0;
	while ((opt = getopt_long(argc, argv, "adf:n:p:", long_options, NULL)) != EOF)
#elif HAVE_GETOPT
	opterr = 0;
	while ((opt = getopt(argc, argv, "adf:n:p:")) != EOF)
#endif /* HAVE_GETOPT_LONG */
	{
		switch (opt) {
			case 'a':
				if ( target_num == -1 ) {
					target_num = 0;
				} else {
					usage_msg(argv[0]);
					fprintf(stderr, "Cannot select both all images and specific images\n");
					fflush(stderr);
					return 1;
				}
				break;
			case 'd':
				delete_flag = 1;
				break;
			case 'f':
				file_prefix = optarg;
				break;
			case 'n':
				if ( target_num == -1 ) {
					target_num = atoi(optarg);
				} else {
					usage_msg(argv[0]);
					fprintf(stderr, "Cannot select both all images and specific images\n");
					fflush(stderr);
					return 1;
				}
				break;
			case 'p':
				port = optarg;
				break;
			case '?':
			default:
				error_flag++;
				break;
		}
	}

#endif /* HAVE_GETOPT || HAVE_GETOPT_LONG */

#if DIMAGE_V_DEBUG
	debug=fopen("debug.log","w");
#endif


	if ( port ==  NULL ) {
		usage_msg(argv[0]);
		fprintf(stderr, "Must specify a value for port\n");
		fflush(stderr);
		return 1;
	}

	if ( file_prefix == NULL ) {
		fprintf(stderr, "Using file as default file prefix\n");
		fflush(stderr);
		if ( ( file_prefix = malloc(5) ) == NULL ) {
			perror("Unable to allocate file_prefix");
			return 1;
		}
		snprintf(file_prefix, 5, "file");
	}	

	if ( strlen(port) >= 20 ) {
		fprintf(stderr, "Bad value for port, using /dev/dimagev\n");
		fflush(stderr);
		snprintf(serial_port, 20, "/dev/dimagev");
	} else {
		snprintf(serial_port, 20, port);
	}
	
	dev = dimage_v_open(serial_port);
	if ( dev == NULL ) {
		fprintf(stderr, "Got a null pointer back from dimage_v_open\n");
		return 1;
	}	
	
	num = dimage_v_number_of_pictures();

	if ( num > 0 ) {
		printf("Number of pictures is %d\n", num);
		sleep(3);
	}	

	if ( target_num == 0 ) {
		for ( i = 1 ; i <= num ; i++ ) {

			printf("Picture will be named %s%04d.jpg\n", file_prefix, i);

			filename = dimage_v_write_picture_to_file(i);

			printf("filename %d is %s\n", i, filename);

			if ( dstnamebuffer == NULL ) {
				perror("Error mallocing memory for name buffers");
				gpio_close(dev);
			}	

			/* There must be a better way to see if the camera is ready... */
			sleep(3);
		}
	} else {
		/* Just get the specific image. */
		printf("Picture will be named %s%04d.jpg\n", file_prefix, target_num);
		filename = dimage_v_write_picture_to_file(target_num);
		printf("filename %d is %s\n", target_num, filename);
	}

	gpio_close(dev);
	gpio_free(dev);
	return 0;
}
