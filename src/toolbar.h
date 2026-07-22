#ifndef TOOLBAR_H
#define TOOLBAR_H
enum {
	OPEN_BUTTON=0,
	SAVE_BUTTON,
	PRINT_BUTTON,
	CLOSE_BUTTON,
	PREVIOUS_BUTTON,
	NEXT_BUTTON,
	DOWNLOAD_THUMB_BUTTON,
	DOWNLOAD_EMPTY_BUTTON,
	DOWNLOAD_SELECTED_BUTTON,
	DELETE_BUTTON,
	STOP_BUTTON,
	ROTATE_CLOCKWISE_BUTTON,
	ROTATE_CCLOCKWISE_BUTTON,
	FLIP_HORIZ_BUTTON,
	FLIP_VERT_BUTTON,
	RESIZE_BUTTON,
	COLORS_BUTTON,
	GALLERY_BUTTON,
	LIVE_BUTTON,
	CONFIG_BUTTON,
	HELP_BUTTON,
	EXIT_BUTTON,
	NB_BUTTONS
};

GtkWidget *add_to_toolbar (GtkWidget *mainWin, 
		     int button_number, gchar *tooltipText, 
		     gchar ** xpmIcon, GtkSignalFunc f, gpointer data,
		     GtkWidget *box, int Beginning);
void create_toolbar (GtkWidget *box, GtkWidget *mainWin);
void deactivate_button (int button_number);
void activate_button (int button_number);
void set_toolbar_busy ();
void set_toolbar_ready ();
#endif
