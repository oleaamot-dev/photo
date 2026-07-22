#include "philips.h"
/* #include <gnome.h> */


void
on_Camera_Configuration_destroy        (GtkObject       *object,
                                        gpointer         user_data);

void
on_copyright_string_activate           (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_copyright_string_focus_out_event    (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_date_togglebutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_maunual_checkbutton_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_exposure_hscale_focus_out_event     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_macro_checkbutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_zoom_hscale_focus_out_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_memory_togglebutton_event           (GtkWidget       *widget,
                                        GdkEventClient  *event,
                                        gpointer         user_data);

void
on_date_imprint_togglebutton_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_debug_togglebutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_apply_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancel_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);
