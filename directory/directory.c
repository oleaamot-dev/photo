/* Directory Browsing Interface -----------------------------------
   ---------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include "../src/gphoto.h"
#include "../src/util.h"

extern struct ImageMembers Thumbnails;

char *dir_directory;
char dir_images[1024][256];
int dir_num_images;
int dir_get_index;

int dir_initialize () {

	int i=0;

	dir_num_images = 0;

	for (i=0; i<1024; i++)
		strcpy(dir_images[i], "");

	if (dir_directory != NULL)
		free (dir_directory);
	dir_directory = NULL;

	return 1;
}

int dir_get_dir() {

	DIR *dir;
	struct dirent *file;
	char filename[1024];
	GtkWidget *filesel;
	GdkImlibImage *imlibimage;

	filesel = gtk_directory_selection_new("Select a directory to open...");
	gtk_window_set_position (GTK_WINDOW (filesel),
		GTK_WIN_POS_CENTER);
	if (wait_for_hide(filesel,
	   GTK_FILE_SELECTION(filesel)->ok_button,
	   GTK_FILE_SELECTION(filesel)->cancel_button) == 0)
		return 0;

	(void)dir_initialize();	

	dir_directory = gtk_file_selection_get_filename(
					GTK_FILE_SELECTION(filesel));
	dir = opendir(dir_directory);
	file = readdir(dir);
	while (file != NULL) {
		update_progress(-1);
		if ((strcmp(file->d_name, ".") != 0) &&
		    (strcmp(file->d_name, "..") != 0)) {
			sprintf(filename, "%s%s", dir_directory,
				file->d_name);
			if ((imlibimage = gdk_imlib_load_image(
			   filename)) == 0)
				{}
			else {
				dir_num_images++;
				strcpy(dir_images[dir_num_images-1],
					file->d_name);
				gdk_imlib_kill_image(imlibimage);
			}
		}
		file = readdir(dir);
	}
	closedir(dir);
	free (file);
	return 1;
}

struct Image *dir_get_picture (int picture_number, int thumbnail) {

	GdkImlibImage *imlibimage, *thumbimlibimage;
	char filename[1024], fname[1024], type[5];
	int w, h, i=0;
	struct Image *im;
	FILE *fp;
	char *imagedata, *dot;
	long imagesize;

	if (dir_num_images == 0)
		return 0;
	sprintf(filename, "%s%s", dir_directory,
		dir_images[picture_number-1]);
	
	dot = strrchr(filename, '.');
	while ((dot) && (i<5)) {
		type[i] = dot[i+1];
		i++;
	}
	if (!thumbnail) {
		fp = fopen(filename, "r");
		fseek(fp, 0, SEEK_END);
		imagesize = ftell(fp);
		rewind(fp);
		imagedata = (char *)malloc(sizeof(char)*imagesize);
		fread(imagedata, (size_t)imagesize, (size_t)sizeof(char),
			fp);
		fclose(fp);
		im = (struct Image*)malloc(sizeof(struct Image));
		im->image = imagedata;
		im->image_size = imagesize;
		im->image_info_size = 0;
		strcpy(im->image_type, type);
		return (im);
	}
	imlibimage = gdk_imlib_load_image(filename);
	w = imlibimage->rgb_width; 
	h = imlibimage->rgb_height;
	if (w > 64) {
		h = h * 80 / w;
		w = 80;
	}
	if (h > 64) {
		w = w * 60/ h;
		h = 60;
	}
	if (w == 0)
		w = 1;
	if (h == 0)
		h = 1;
	thumbimlibimage = gdk_imlib_clone_scaled_image(imlibimage, w, h);
	sprintf(fname, "%s/dir_thumb_%s", gphotoDir, dir_images[picture_number-1]);
	gdk_imlib_save_image(thumbimlibimage, fname, NULL);
	gdk_imlib_kill_image(imlibimage);
	gdk_imlib_kill_image(thumbimlibimage);
	fp = fopen(fname, "r");
	fseek(fp, 0, SEEK_END);
	imagesize = ftell(fp);
	rewind(fp);
	imagedata = (char *)malloc(sizeof(char)*imagesize);
	fread(imagedata, (size_t)imagesize, (size_t)sizeof(char),
		fp);
	fclose(fp);
	remove(fname);
	im = (struct Image*)malloc(sizeof(struct Image));
	im->image = imagedata;
	im->image_size = imagesize;
	im->image_info_size = 2;
	im->image_info = (char **)malloc(sizeof(char*)*2);
	im->image_info[0] = "Name";
	im->image_info[1] = strdup(dir_images[picture_number-1]);
	strcpy(im->image_type, type);
	return (im);
}

int dir_delete_image (int picture_number) {

	/* Can not delete images in directory browse mode for
	   safety's sake */

	return 0;
}

int dir_number_of_pictures () {

	if (Thumbnails.next == NULL) {
		if (dir_get_dir() == 0)
			return (0);
	}
	return (dir_num_images);
}

int dir_configure () {

	/* No configuration option available in Directory Browse mode */

	return 0;
}

struct Image *dir_get_preview () {

	/* No way to get a live preview... :) */

	return 0;
}

int dir_take_picture () {

	/* wow, a trend.. no way to take a live picture */

	return 0;
}

char *dir_summary() {

	char *summary;

	summary = (char*)malloc(sizeof(char)*1024);
	sprintf(summary, "Current Directory:\n%s", dir_directory);

	return(summary);
}

/* Directory Browsing Interface -----------------------------------
   ---------------------------------------------------------------- */

static char *dir_description() {

	return(
"Browse Directory\nScott Fritzinger <scottf@unr.edu>
Allows the user to \"open\" a directory of
images and all gPhoto functions will work
on those images (HTML Gallery, etc...)\n
This will not work in command-line mode,
due to the high amount of input needed
to set it up.");

}

struct _Camera directory = {dir_initialize,
			    dir_get_picture,
			    dir_get_preview,
			    dir_delete_image,
			    dir_take_picture,
			    dir_number_of_pictures,
			    dir_configure,
			    dir_summary,
			    dir_description,
			    NULL};
