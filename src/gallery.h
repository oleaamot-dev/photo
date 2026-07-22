/* HTML Gallery Engine for generating a web gallery of your images
   from within gPhoto. Can be used with any camera library, or even
   with "browse directory" to create your html gallery...

	Gallery tags:
		GALLERY_NAME
		GALLERY_INDEX

		DATE

		THUMBNAIL
		THUMBNAIL_FILENAME
		THUMBNAIL_NUMBER

		PICTURE
		PICTURE_FILENAME
		PICTURE_NUMBER

		PICTURE_NEXT
		PICTURE_PREVIOUS
*/


void gallery_change_dir(GtkWidget *widget, GtkWidget *label);
void gallery_parse_tags(char *dest, char *source);
void gallery_main();

