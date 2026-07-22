
int fuji_init();
int fuji_configure();
struct Image *fuji_get_picture (int picture_number,int thumbnail);
int fuji_delete_image (int picture_number);
int fuji_number_of_pictures ();
int fuji_initialize ();
struct Image *fuji_get_preview ();
int fuji_take_picture ();
char *fuji_summary();
char *fuji_description();

/* Global Configuration Variables */

extern int fuji_debug; 
