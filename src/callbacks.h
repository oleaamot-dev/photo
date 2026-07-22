#ifndef _CALLBACKS_H
#define _CALLBACKS_H
#include <gnome.h>
#include "gphoto.h"

gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);
gint thumb_click (GtkWidget *widget, GdkEventButton *event, gpointer callback_data);
char* find_tag(struct Image *im, char* name);
void index_page ();
void next_page ();
void prev_page ();
void set_camera (char *model);
gint load_config();
void save_config();
void configure_call();
void mail_image_call();
void web_browse_call();
void takepicture_call();
void format(GtkWidget *dialog, GtkObject *button);
void del_pics (GtkWidget *dialog, GtkObject *button);
void del_dialog (int type);
void savepictodisk (int picNum, int thumbnail, char *prefix);
void saveselectedtodisk (GtkWidget *widget, char *type);
void appendpic (int picNum, int thumbnail, int fromCamera, char *fileName, int factor);
void destroy (GtkWidget *widget, gpointer data);
void port_dialog();
void version_dialog();
void usersmanual_dialog();
void faq_dialog();
void about_dialog();
void show_license();
void menu_selected (gchar *toPrint);
void remove_thumbnail (int i);
void insert_thumbnail(struct ImageMembers *node);
void makeindex (int getthumbs);
void getindex ();
void getindex_empty ();
void halt_action ();
void getpics (char *type);
void getpics_with_scale (char *type, int factor);
void remove_image(int i);
void closepic ();
void print_pic ();
void select_all();
void select_inverse();
void select_none();
void toggle_in_area(double, double, double, double);
void color_dialog();
void resize_dialog();
void manip_pic (gchar *Option);
void summary_dialog();
void scale (int factor);
void scale_half ();
void scale_double ();
void save_images (gpointer data, guint action, GtkWidget *widget);
void open_images (gpointer data, guint action, GtkWidget *widget);
void open_images_and_scale25 (gpointer data, guint action, GtkWidget *widget);
void open_images_and_scale50 (gpointer data, guint action, GtkWidget *widget);
void save_thumbs (gpointer data, guint action, GtkWidget *widget);
void open_thumbs (gpointer data, guint action, GtkWidget *widget);
void save_both (gpointer data, guint action, GtkWidget *widget);
void open_both (gpointer data, guint action, GtkWidget *widget);

void post_process_change (GtkWidget *widget, GtkWidget *window);

void save_dialog (GtkWidget *widget, gpointer data);
void open_dialog (GtkWidget *widget, gpointer data);

void browse_gallery();
void browse_help();
void browse_gphoto();
void browse_feedback();
void browse_news();
void browse_download();
void browse_cameras();
void browse_supporting();
void browse_discussion();
void browse_team();
void browse_themes();
void browse_links();
void browse_todo();
#endif
