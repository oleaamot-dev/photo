/* Directory Browsing Interface -----------------------------------
   ---------------------------------------------------------------- */

extern char *dir_directory;
extern char dir_images[1024][256];
extern int dir_num_images;
extern int dir_get_index;

int dir_initialize ();
int dir_get_dir();
Image *dir_get_picture (int picture_number, int thumbnail);
int dir_delete_image (int picture_number);
int dir_number_of_pictures ();
int dir_configure ();
Image *dir_get_preview ();
int dir_take_picture ();
char *dir_summary();
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
			    dir_description};
