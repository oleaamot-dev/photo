/* Kodak DC2X Camera Functions ----------------------------------
    Based heavily on the code for olympus.c 
   ----------------------------------------------------------- */

#include "change_res.h"
#include "get_info.h"
#include "session.h"
#include "comet_to_ppm.h"
#include "get_pic.h"
#include "pics_to_file.h"
#include "shoot.h"
#include "convert_pic.h"
#include "get_thumb.h"
#include "pixmaps.h"
#include "thumbs_to_file.h"
#include "dc20.h"
#include "hash_mark.h"
#include "read_data.h"
#include "erase.h"
#include "init_dc20.h"
#include "send_pck.h"
#include "../src/gphoto.h"

#include <gdk_imlib.h>
#include <gdk/gdk.h>

int verbose=FALSE,
    quiet=FALSE;

int kodak_dc2x_open_camera() {
  
  /* Open the camera for reading/writing */

#ifdef __NetBSD__
  char *serial_port = "/dev/tty00"; 
#else
  char *serial_port = "/dev/ttyS0"; 
#endif
 
  int tfd;

  speed_t tty_baud = B115200;


  if ((tfd = init_dc20(serial_port, tty_baud)) != -1)
    {
      fprintf(stderr, "Made it back from init_dc20 and tfd was set to %d !\n", tfd);
      return (tfd);
    } else {
      return (0);
    }
}

void kodak_dc2x_close_camera(int tfd) {
  
  /* Close the camera */

  close_dc20(tfd);
}

int kodak_dc2x_initialize() {
    return(1);
}

int kodak_dc2x_number_of_pictures () {

  int tfd;
  Dc20Info   *dc20_info;

	if ((tfd = kodak_dc2x_open_camera()) == 0) {
                dialog_error("Could not open camera.");
                return (0);
        }
	sleep(1);

	dc20_info = get_info(tfd);

	kodak_dc2x_close_camera(tfd);

	return (dc20_info->pic_taken);
}

int kodak_dc2x_take_picture() {

  int error = 0;
  int tfd;

  if ((tfd = kodak_dc2x_open_camera()) == 0) {
    dialog_error("Could not open camera.");
    return (0);
  }

  /* Actually snap the shot 
     The olympus way..  eph_action(iob,2,&zero,1);  */

  fprintf (stderr, "About to call shoot!\n");
  error = (shoot(tfd));
  fprintf (stderr, "After shoot the error was %d !\n", error);

  if (error == -1) {
    /* dc20_take_picture failed..  */
    return(0);
  }

  kodak_dc2x_close_camera(tfd);

  return (kodak_dc2x_number_of_pictures());

}

struct Image *kodak_dc2x_get_picture (int picNum, int thumbnail) {

  int tfd, image_size, image_width, net_width, camera_header, components;
  Dc20Info *my_info;
  unsigned char color_thumb[14400];
  unsigned char pic[MAX_IMAGE_SIZE];
  struct pixmap	*pp;

  GdkImlibImage *this_image, *scaled_image;
  GdkImlibColorModifier mod;

  FILE *jpgfile;
  int jpgfile_size;
  char filename[1024];
  struct Image *im;

  if ((tfd = kodak_dc2x_open_camera()) == 0) {
    dialog_error("Could not open camera.");
    return (0);
  }

  my_info = get_info(tfd);

  fprintf(stderr, "downloading from a DC%x\n", my_info->model);

  if (my_info->model == 0x25){
    fprintf(stderr, "Match with 25!\n");
    if (thumbnail) {
      fprintf(stderr, "Getting thumbnail #%d from a DC25!\n", picNum);
      if (get_thumb(tfd, picNum, color_thumb) == -1) {
	fprintf(stderr,"get_thumb failed!\n");
	return(0);
      } else {
	fprintf(stderr,"get_thumb returned ok! Creating ImLib image!\n");
	this_image = gdk_imlib_create_image_from_data(color_thumb, NULL, 80, 60);
	fprintf(stderr, "Made it back from imlib_create!\n");
        sprintf(filename, "%s/gphoto-kodak-%i.jpg", gphotoDir, picNum);
        gdk_imlib_save_image (this_image, filename, NULL);
	gdk_imlib_kill_image (this_image);
        jpgfile = fopen(filename, "r");
        fseek(jpgfile, 0, SEEK_END);
        jpgfile_size = ftell(jpgfile); 
        rewind(jpgfile);
        im = (struct Image*)malloc(sizeof(struct Image));
        im->image = (char *)malloc(sizeof(char)*jpgfile_size);
	fread(im->image, (size_t)sizeof(char), (size_t)jpgfile_size, jpgfile); 
	fclose(jpgfile);
        strcpy(im->image_type, "jpg");
        im->image_size = (int)jpgfile_size;
        im->image_info_size = 0;
        remove(filename);
	return (im);
      }
    } else {
      fprintf(stderr, "Getting picture #%d from a DC25!\n", picNum ); 
      if (get_pic(tfd, picNum, pic, 0) == -1) {
	fprintf(stderr, "get_pic puked!\n");
	return(0);
      } else {
	fprintf(stderr, "returned from get_pic ok!\n");

	/*
	 *	Setup image size with resolution
	 */

	image_size = IMAGE_SIZE(pic[4]);
	image_width = WIDTH(pic[4]);
	net_width = image_width - LEFT_MARGIN - RIGHT_MARGIN(pic[4]);
	camera_header = CAMERA_HEADER(pic[4]);
	components = 3;

	/*
	 *	Convert the image to 24 bits
	 */

	if ((pp = alloc_pixmap(net_width - 1, HEIGHT - BOTTOM_MARGIN - 1, components)) == NULL) {
	  if (!quiet) fprintf(stderr, "%s: convert_pic: error: alloc_pixmap\n", __progname);
	  return 0;
	}
	
	if (comet_to_pixmap(pic, pp) == -1) {
	  fprintf(stderr, "comet_to_pixmap puked!\n");
	  return (0);
	} else {
	  
	  fprintf(stderr, "attempting to imlib_create the image!\n");
	  this_image = gdk_imlib_create_image_from_data(pp->planes, NULL, pp->width, pp->height);
	  fprintf(stderr, "Made it back from imlib_create!\n");

	  /* now we just need to resize it! */
	  if (!pic[4]) {
	    /* high res 493x373 */
	    fprintf(stderr, "High Res!\n");
	    scaled_image = gdk_imlib_clone_scaled_image(this_image, 493, 373);

	  } else {
	    /* low res 320x240 */
	    fprintf(stderr, "Low Res!\n");
	    scaled_image = gdk_imlib_clone_scaled_image(this_image, 320, 240);

	    /* How? */
	  }

	  gdk_imlib_kill_image(this_image);

	  /* correct the contrast a bit before handing it back.. */
	  gdk_imlib_get_image_modifier(scaled_image,&mod);
	  mod.contrast = 256 * 1.3;
	  gdk_imlib_set_image_modifier(scaled_image,&mod);
	  gdk_imlib_apply_modifiers_to_rgb(scaled_image);

	  kodak_dc2x_close_camera(tfd);

	  sprintf(filename, "%s/gphoto-kodak-%i.jpg", gphotoDir, picNum);
	  gdk_imlib_save_image (scaled_image, filename, NULL);
	  gdk_imlib_kill_image (scaled_image);
	  jpgfile = fopen(filename, "r");
	  fseek(jpgfile, 0, SEEK_END);
	  jpgfile_size = ftell(jpgfile);
	  rewind(jpgfile);
	  im = (struct Image*)malloc(sizeof(struct Image));
	  im->image = (char *)malloc(sizeof(char)*jpgfile_size);
	  fread(im->image,(size_t)sizeof(char),(size_t)jpgfile_size,jpgfile); 
	  fclose(jpgfile);
	  strcpy(im->image_type, "jpg");
	  im->image_size = jpgfile_size;
	  im->image_info_size = 0;
	  remove(filename);

	  return (im);
	}

      }
    }
  } else {
    fprintf(stderr, "No match with 25!");
    return(0);
  }
  return(0);
}

int kodak_dc2x_delete_picture (int picNum) {

  int tfd;
  Dc20Info *my_info;

  /* need to check to see if it is a dc20 or dc25 here!!!  */
  
  if ((tfd = kodak_dc2x_open_camera()) == 0) {
    dialog_error("Could not open camera.");
    return (0);
  }

  my_info = get_info(tfd);

  if (my_info->model == 0x25){
    if (erase(tfd, picNum) == -1) {
      dialog_error("Deletion failed!");
      kodak_dc2x_close_camera(tfd);
      return(0);
    } else {
      kodak_dc2x_close_camera(tfd);
      return(1);
    }
    

  } else {
    /* must be a DC20?? */
    kodak_dc2x_close_camera(tfd);
    dialog_error("The DC20 won't let you delete a single pic! Sorry!");
    return 0;
  }
}


struct Image *kodak_dc2x_get_preview () {

  int new_pic_num;
  struct Image *new_pic;


  /* Take new pic */
  new_pic_num = kodak_dc2x_take_picture();

  if (new_pic_num != 0) {
    /* Get the new pic into memory */
    new_pic = kodak_dc2x_get_picture(new_pic_num, 0);
    
    /* delete it from the camera */
    kodak_dc2x_delete_picture(new_pic_num);

    /* give it back */
    return new_pic;

  } else {
    return(0);
  }

}


int kodak_dc2x_configure () {

  dialog_error("The DC25 has switches on the case for configuration. Use those for now! -Del");
  return(0);

}



char *kodak_dc2x_summary() {

  char summary_string[500];
  char *summary;
  int tfd;
  Dc20Info   *dc20_info;
  
  if ((tfd = kodak_dc2x_open_camera()) == 0) {
    dialog_error("Could not open camera.");
    return (0);
  }
  sleep(1);
  
  dc20_info = get_info(tfd);

/*    strcat(summary_string, "This camera is a Kodak DC"); */
/*    strcat (summary_string, dc20_info->model); */  
  snprintf(summary_string, 500, "This camera is a Kodak DC%d",(int)dc20_info->model);

  /*
  There are %d pictures in the camera\n
and there are %d pictures left in the camera\n", 
dc20_info->model, dc20_info->pic_taken, dc20_info->pic_left);
  */

  summary = (char *)malloc(sizeof(char)*strlen(summary_string)+32);
  strcpy(summary, summary_string);

  return(summary);


}




char *kodak_dc2x_description() {

	return(
"Kodak DC25 and DC20 support 
by Del Simmons <del@freespeech.com>. 
This code is working on my DC25 but 
I know it will have some problems with 
the DC20. If you have a DC20, please 
give it a shot and let me know all 
the places it fails. If compiling for
the DC20, please try removng the -DDC25
flag from the Makefile in the kodak 
directory. Thanks!");
}

/* Declare the camera function pointers */

struct _Camera kodak_dc2x = {kodak_dc2x_initialize,
			kodak_dc2x_get_picture,
			kodak_dc2x_get_preview,
			kodak_dc2x_delete_picture,
			kodak_dc2x_take_picture,
			kodak_dc2x_number_of_pictures,
			kodak_dc2x_configure,
			kodak_dc2x_summary,
			kodak_dc2x_description,
			NULL};

/* End of Kodak DC2X Camera functions ------------------------------
   -------------------------------------------------------------- */
