#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exif.h"
#include "gphoto_fuji.h"
#include "../src/gphoto.h"

GtkWidget *debugset;

get_fuji_config(GtkWidget *thewid, gpointer *data){
  fuji_debug=GTK_TOGGLE_BUTTON(debugset)->active;
  printf("Debug is %d\n",fuji_debug);
};

GtkWidget *open_fuji_config_dialog(void){
  GtkWidget *dialog, *label,*okbutton,*cancelbutton;
  

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "gPhoto Message");
  /*  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);*/
  label = gtk_label_new("Fuji Library Configuration");
  debugset = gtk_check_button_new_with_label("Debug Mode");
  okbutton = gtk_button_new_with_label("Accept");
  cancelbutton = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS (cancelbutton, GTK_CAN_DEFAULT);

  gtk_signal_connect_object(GTK_OBJECT(okbutton), "clicked",
                            GTK_SIGNAL_FUNC(get_fuji_config),
                            GTK_OBJECT(dialog));

  gtk_signal_connect_object(GTK_OBJECT(okbutton), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(dialog));

  gtk_signal_connect_object(GTK_OBJECT(cancelbutton), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(dialog));

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     debugset, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     okbutton, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     cancelbutton, TRUE, TRUE, 0);

#ifdef GTK_HAVE_FEATURES_1_1_4
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
#endif
  gtk_widget_show(label);
  gtk_widget_show(okbutton);
  gtk_widget_show(cancelbutton);
  gtk_widget_show(debugset);
  gtk_widget_show(dialog);

};
