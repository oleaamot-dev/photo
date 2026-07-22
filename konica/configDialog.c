#include "qm100.h"
static int  needSave=0;
static GtkWidget *qm100ConfigDlg;
static GtkObject *selfTimer;
static GtkObject *autoOff;
GtkWidget        *clockLabel;
static GSList    *Speed_group = NULL;
static GSList    *Port_group = NULL;
/*---------------------------------------------------------------------*
 *                                                                     *
 * Forward declaration of local functions.                             *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void button_clicked(GtkButton *button, gpointer user_data);
static void closeDialog(void);
static void port_pressed(GtkButton *button, gpointer user_data);
static void reloadValues(void);
static void resetValues(void);
static void setSize(GtkButton *button, gpointer user_data);
static void set_notebook_tab(GtkWidget *notebook, gint page_num, GtkWidget *widget);
static void spd_pressed(GtkButton *button, gpointer user_data);
static void tracePressed(GtkButton *button, gpointer user_data);
static void setClock(void);
static void showCameraClock(void);
/*---------------------------------------------------------------------*
 *                                                                     *
 * Label/titles for the tab pages                                      *
 *                                                                     *
 *---------------------------------------------------------------------*/
static char *nbtitle[] =
{
   "Communication Settings\n",
   "Camera Settings\n",
   "Photo Settings\n",
   "Format CF\n",
   "Tracing Options\n"
};
static char *nbtab[] =
{
   "Communication",
   "Camera",
   "Photo",
   "Format",
   "Tracing"
};
#define NBPAGES   (sizeof(nbtitle)/sizeof(char *))
#define CommPage   nbpage[0]
#define CameraPage nbpage[1]
#define PhotoPage  nbpage[2]
#define FormatPage nbpage[3]
#define TracePage  nbpage[4]
/*---------------------------------------------------------------------*
 *                                                                     *
 * Names for the button controls                                       *
 *                                                                     *
 *---------------------------------------------------------------------*/
static char *buttonNames[] =
{
   "Save",
   "Reload",
   "Defaults",
   "Close"
};
typedef enum
{
   SAVEBUTTON,
   RELOADBUTTON,
   DEFAULTSBUTTON,
   CLOSEBUTTON,
   CLOCKBUTTON
} buttonvals;
/*---------------------------------------------------------------------*
 *                                                                     *
 * Labels/values for the Port radio buttons                            *
 *                                                                     *
 *---------------------------------------------------------------------*/
static char *PortLabel[] =
{
   "Autodetect",
#ifdef __NetBSD__
   "/dev/tty00",
   "/dev/tty01",
   "/dev/tty02",
   "/dev/tty03"
#else
   "/dev/ttyS0",
   "/dev/ttyS1",
   "/dev/ttyS2",
   "/dev/ttyS3"
#endif
};
/*---------------------------------------------------------------------*
 *                                                                     *
 * Labels/values for the Speed radio buttons.                          *
 *                                                                     *
 *---------------------------------------------------------------------*/
static char *SpeedLabel[] =
{
   "Optimize",
   "9600",
   "19200",
   "38400",
   "57600",
   "115200",
};
/*---------------------------------------------------------------------*
 *                                                                     *
 * set_notebook_tab - create tab for notebook dialog - borrowed        *
 *                    from ../casio/configure.c                        *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void set_notebook_tab(GtkWidget *notebook,
                             gint page_num,
                             GtkWidget *widget)
{
   GtkNotebookPage *page;
   GtkWidget *notebook_page;
   page = (GtkNotebookPage*) g_list_nth(GTK_NOTEBOOK(notebook)->children, page_num)->data;
   notebook_page = page->child;
   gtk_widget_ref(notebook_page);
   gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);
   gtk_notebook_insert_page(GTK_NOTEBOOK(notebook), notebook_page,
                             widget, page_num);
   gtk_widget_unref(notebook_page);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * configureDialog - called from qm100 CLI utility.  Create a gtk      *
 *                   environment, then invokes the configuration       *
 *                   dialog.                                           *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_configureDialog()
{
   GtkWidget *dialog;
   gtk_init(0, NULL);
   dialog = qm100_createConfigDlg();
   gtk_widget_show(dialog);
   gtk_main();
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * createConfigDlg - crete configuration dialog                        *
 *                                                                     *
 *---------------------------------------------------------------------*/
GtkWidget *qm100_createConfigDlg()
{
   GtkWidget *TraceBytesToggle;
   GtkWidget *TraceToggle;
   GtkWidget *BeepToggle;
   GtkWidget *dialog_action_area1;
   GtkWidget *dialog_vbox1;
   GtkWidget *frame;
   GtkWidget *hbuttonbox1;
   GtkWidget *label;
   GtkWidget *nbpage[NBPAGES];
   GtkWidget *notebook1;
   GtkWidget *otable;
   GtkWidget *itable;
   GtkWidget *twgt;
   GtkWidget *vbox;
   QM100_CONFIGDATA *cp = &qm100_configData;
   int        i;
   int        tabnum=0;
   char       tstring[60];
   /*------------------------------------------------------------------*
    *                                                                  *
    * Set up basic frame and title                                     *
    *                                                                  *
    *------------------------------------------------------------------*/
   qm100ConfigDlg = gtk_dialog_new();
   gtk_widget_set_name(qm100ConfigDlg, "qm100ConfigDlg");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "qm100ConfigDlg", qm100ConfigDlg);
   GTK_WIDGET_SET_FLAGS(qm100ConfigDlg, GTK_CAN_DEFAULT);
   gtk_widget_grab_focus(qm100ConfigDlg);
   gtk_widget_grab_default(qm100ConfigDlg);
   gtk_widget_set_extension_events(qm100ConfigDlg, GDK_EXTENSION_EVENTS_ALL);
   gtk_window_set_title(GTK_WINDOW(qm100ConfigDlg), "Configure Konica/HP Camera");
   gtk_window_set_policy(GTK_WINDOW(qm100ConfigDlg), TRUE, TRUE, FALSE);
   gtk_signal_connect_object(GTK_OBJECT(qm100ConfigDlg), "delete_event",
                             GTK_SIGNAL_FUNC(button_clicked),
                             (gpointer) CLOSEBUTTON);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create the box and container for the notebook pages              *
    *                                                                  *
    *------------------------------------------------------------------*/
   dialog_vbox1 = GTK_DIALOG(qm100ConfigDlg)->vbox;
   gtk_widget_set_name(dialog_vbox1, "dialog_vbox1");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "dialog_vbox1", dialog_vbox1);
   gtk_widget_show(dialog_vbox1);
   notebook1 = gtk_notebook_new();
   gtk_widget_set_name(notebook1, "notebook1");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "notebook1", notebook1);
   gtk_widget_show(notebook1);
   gtk_box_pack_start(GTK_BOX(dialog_vbox1), notebook1, TRUE, TRUE, 0);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create the individual notebook pages and tabs.                   *
    *                                                                  *
    *------------------------------------------------------------------*/
   for(i=0; i<NBPAGES; i++)
      {
      nbpage[i] = gtk_vbox_new(FALSE, 0);
      gtk_widget_set_name(nbpage[i], nbtitle[i]);
      gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), nbtitle[i], nbpage[i]);
      gtk_container_add(GTK_CONTAINER(notebook1), nbpage[i]);
      gtk_widget_show(nbpage[i]);
      gtk_container_set_border_width(GTK_CONTAINER(nbpage[i]), 20);
      label = gtk_label_new(nbtitle[i]);
      gtk_widget_set_name(label, nbtitle[i]);
      gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), nbtitle[i], label);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(nbpage[i]), label, FALSE, FALSE, 0);
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_FILL);
      label = gtk_label_new(nbtab[i]);
      gtk_widget_set_name(label, nbtab[i]);
      gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), nbtab[i], label);
      gtk_widget_show(label);
      set_notebook_tab(notebook1, tabnum++, label);
      }
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create the frame and radio buttons for port selection            *
    *                                                                  *
    *------------------------------------------------------------------*/
   otable = gtk_table_new (2, 2, FALSE);
   gtk_table_set_row_spacing(GTK_TABLE(otable), 0, 20);
   gtk_table_set_col_spacing(GTK_TABLE(otable), 0, 20);
   gtk_container_add(GTK_CONTAINER(CommPage), otable);
   frame = gtk_frame_new(NULL);
   gtk_frame_set_label(GTK_FRAME(frame), "Port" );
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   gtk_table_attach_defaults(GTK_TABLE(otable), frame, 0, 1, 0, 1);
   vbox  = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   for(i=0; i<5; i++)
      {
      twgt = gtk_radio_button_new_with_label(Port_group, PortLabel[i]);
      Port_group = gtk_radio_button_group(GTK_RADIO_BUTTON(twgt));
      gtk_widget_set_name(twgt, PortLabel[i]);
      gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), PortLabel[i], twgt);
      gtk_widget_show(twgt);
      gtk_box_pack_start(GTK_BOX(vbox), twgt, FALSE, FALSE, 0);
      gtk_signal_connect_after(GTK_OBJECT(twgt), "pressed",
                                GTK_SIGNAL_FUNC(port_pressed),
                               (gpointer) PortLabel[i]);
      if (strcasecmp(PortLabel[i], cp->device) == 0)
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(twgt), TRUE);
      }
   gtk_widget_show(vbox);
   gtk_widget_show(frame);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create the frame and radio buttons for speed selection           *
    *                                                                  *
    *------------------------------------------------------------------*/
   frame = gtk_frame_new(NULL);
   gtk_frame_set_label(GTK_FRAME(frame), "Speed" );
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   gtk_table_attach_defaults(GTK_TABLE(otable), frame, 1, 2, 0, 1);
   vbox  = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   for(i=0; i<(sizeof(SpeedLabel)/sizeof(char *)); i++)
      {
      twgt   = gtk_radio_button_new_with_label(Speed_group, SpeedLabel[i]);
      Speed_group = gtk_radio_button_group(GTK_RADIO_BUTTON(twgt));
      gtk_widget_set_name(twgt, SpeedLabel[i]);
      gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), SpeedLabel[i], twgt);
      gtk_widget_show(twgt);
      gtk_box_pack_start(GTK_BOX(vbox), twgt, FALSE, FALSE, 0);
      gtk_signal_connect_after(GTK_OBJECT(twgt), "pressed",
                                GTK_SIGNAL_FUNC(spd_pressed),
                               (gpointer) i);
      if (strcasecmp(SpeedLabel[i], cp->speed) == 0)
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(twgt), TRUE);
      }
   gtk_widget_show(vbox);
   gtk_widget_show(otable);
   gtk_widget_show(frame);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create page for the tracing options                              *
    *                                                                  *
    *------------------------------------------------------------------*/
   TraceToggle = gtk_check_button_new_with_label("Trace data packets");
   gtk_widget_set_name(TraceToggle, "TraceToggle");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "TraceToggle", TraceToggle);
   gtk_widget_show(TraceToggle);
   gtk_box_pack_start(GTK_BOX(TracePage), TraceToggle, FALSE, FALSE, 0);
   gtk_signal_connect(GTK_OBJECT(TraceToggle), "pressed",
                       GTK_SIGNAL_FUNC(tracePressed),
                      (gpointer) "Trace");
   if (strcasecmp("Off", cp->tracefile))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(TraceToggle), TRUE);
   TraceBytesToggle = gtk_check_button_new_with_label("Trace all bytes");
   gtk_widget_set_name(TraceBytesToggle, "TraceBytesToggle");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "TraceBytesToggle", TraceBytesToggle);
   gtk_widget_show(TraceBytesToggle);
   gtk_box_pack_start(GTK_BOX(TracePage), TraceBytesToggle, FALSE, FALSE, 0);
   gtk_signal_connect(GTK_OBJECT(TraceBytesToggle), "pressed",
                       GTK_SIGNAL_FUNC(tracePressed),
                      (gpointer) "TraceBytes");
   if (strcasecmp("Off", cp->tracebytes))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(TraceBytesToggle), TRUE);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create page for Camera settings.                                 *
    *                                                                  *
    *------------------------------------------------------------------*/
   otable = gtk_table_new (2, 2, FALSE);
   gtk_table_set_row_spacing(GTK_TABLE(otable), 0, 20);
   gtk_table_set_col_spacing(GTK_TABLE(otable), 0, 20);
   gtk_container_add(GTK_CONTAINER(CameraPage), otable);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Timer controls                                                   *
    *                                                                  *
    *------------------------------------------------------------------*/
   frame = gtk_frame_new(NULL);
   gtk_frame_set_label(GTK_FRAME(frame), "Timers" );
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   gtk_table_attach_defaults(GTK_TABLE(otable), frame, 0, 2, 1, 2);
   itable = gtk_table_new(2, 2, TRUE);
   gtk_container_add(GTK_CONTAINER(frame), itable);
   label = gtk_label_new("Self\nTimer");
   gtk_widget_show(label);
   gtk_table_attach_defaults(GTK_TABLE(itable), label, 0, 1, 0, 1);
   selfTimer = gtk_adjustment_new (3.0, 3.0, 45.0, 1.0, 1.0, 1.0);
   twgt = gtk_hscale_new(GTK_ADJUSTMENT(selfTimer));
   gtk_table_attach_defaults(GTK_TABLE(itable), twgt, 1, 2, 0, 1);
   gtk_widget_show (twgt);
   label = gtk_label_new("Auto\nShut Off");
   gtk_widget_show(label);
   gtk_table_attach_defaults(GTK_TABLE(itable), label, 0, 1, 1, 2);
   autoOff = gtk_adjustment_new (1.0, 1.0, 45.0, 1.0, 1.0, 1.0);
   twgt = gtk_hscale_new(GTK_ADJUSTMENT(autoOff));
   gtk_table_attach_defaults(GTK_TABLE(itable), twgt, 1, 2, 1, 2);
   gtk_widget_show (twgt);
   gtk_widget_show(itable);
   gtk_widget_show(frame);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Date and time                                                    *
    *                                                                  *
    *------------------------------------------------------------------*/
   frame = gtk_frame_new(NULL);
   gtk_frame_set_label(GTK_FRAME(frame), "Clock" );
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   gtk_table_attach_defaults(GTK_TABLE(otable), frame, 0, 1, 0, 1);
   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   clockLabel = gtk_label_new("yyyy/mm/dd hh:mm");
   gtk_label_set_justify(GTK_LABEL(clockLabel), GTK_JUSTIFY_FILL);
   showCameraClock();
   gtk_box_pack_start(GTK_BOX(vbox), clockLabel, TRUE, TRUE, 0);
   hbuttonbox1 = gtk_hbutton_box_new();
   gtk_widget_set_name(hbuttonbox1, "hbuttonbox1");
   gtk_widget_show(hbuttonbox1);
   gtk_container_add(GTK_CONTAINER(vbox), hbuttonbox1);
   twgt = gtk_button_new_with_label("Set");
   gtk_widget_set_name(twgt, "clockbutton");
   gtk_widget_show(twgt);
   gtk_container_add(GTK_CONTAINER(hbuttonbox1), twgt);
   gtk_signal_connect(GTK_OBJECT(twgt), "clicked",
                      GTK_SIGNAL_FUNC(button_clicked),
                      (gpointer) CLOCKBUTTON);
   gtk_widget_show(vbox);
   gtk_widget_show(frame);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Beep control                                                     *
    *                                                                  *
    *------------------------------------------------------------------*/
   frame = gtk_frame_new(NULL);
   gtk_frame_set_label(GTK_FRAME(frame), "Camera Beep");
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   gtk_table_attach_defaults(GTK_TABLE(otable), frame, 1, 2, 0, 1);
   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   label = gtk_label_new("label1");
   gtk_widget_show(label);
   gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
   label = gtk_label_new("label21");
   gtk_widget_show(label);
   gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
   gtk_widget_show(vbox);
   gtk_widget_show(frame);
   gtk_widget_show(otable);
   /*------------------------------------------------------------------*
    *                                                                  *
    * Create the action buttons, and connect them to the handlers.     *
    *                                                                  *
    *------------------------------------------------------------------*/
   dialog_action_area1 = GTK_DIALOG(qm100ConfigDlg)->action_area;
   gtk_widget_set_name(dialog_action_area1, "dialog_action_area1");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "dialog_action_area1",
                        dialog_action_area1);
   gtk_widget_show(dialog_action_area1);
   gtk_container_border_width(GTK_CONTAINER(dialog_action_area1), 10);
   hbuttonbox1 = gtk_hbutton_box_new();
   gtk_widget_set_name(hbuttonbox1, "hbuttonbox1");
   gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), "hbuttonbox1", hbuttonbox1);
   gtk_widget_show(hbuttonbox1);
   gtk_box_pack_start(GTK_BOX(dialog_action_area1), hbuttonbox1, TRUE, TRUE, 0);
   for(i=0; i<(sizeof(buttonNames)/sizeof(char *)); i++)
      {
      twgt = gtk_button_new_with_label(buttonNames[i]);
      gtk_widget_set_name(twgt, buttonNames[i]);
      gtk_object_set_data(GTK_OBJECT(qm100ConfigDlg), buttonNames[i], twgt);
      gtk_widget_show(twgt);
      gtk_container_add(GTK_CONTAINER(hbuttonbox1), twgt);
      gtk_signal_connect(GTK_OBJECT(twgt), "clicked",
                          GTK_SIGNAL_FUNC(button_clicked),
                         (gpointer) i);
      }
   return qm100ConfigDlg;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * setSize - handle selection of Photo size                            *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void setSize(GtkButton *button, gpointer user_data)
{
   needSave = 1;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * port_pressed - handle selection of port/device                      *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void port_pressed(GtkButton *button, gpointer user_data)
{
   char *port =(char *) user_data;
   QM100_CONFIGDATA *cp = &qm100_configData;
   strcpy(cp->device, port);
   needSave = 1;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * trace_pressed - handle selection of trace options.                  *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void tracePressed(GtkButton *button, gpointer user_data)
{
   char *wp =(char *) user_data;
   QM100_CONFIGDATA *cp = &qm100_configData;
   if (strcmp("Trace", wp))
      wp = cp->tracebytes;
   else
      wp = cp->tracefile;
   if (strcasecmp(wp, "On") == 0)
      strcpy(wp, "Off");
   else
      strcpy(wp, "On");
   needSave = 1;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * spd_pressed - handle selection of port speed.                       *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void spd_pressed(GtkButton *button, gpointer user_data)
{
   int speed =(int) user_data;
   QM100_CONFIGDATA *cp = &qm100_configData;
   strcpy(cp->speed, SpeedLabel[speed]);
   needSave = 1;
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * formatCF - event handler for the Format button                      *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void formatCF(GtkButton *button, gpointer user_data)
{
  int serialdev;
  QM100_CONFIGDATA *cp = &qm100_configData;
  serialdev = qm100_open(cp->device);
  qm100_formatCF(serialdev);
  qm100_close(serialdev);
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * button_clicked - handle the action buttons                          *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void button_clicked(GtkButton *button, gpointer user_data)
{
   switch((int) user_data)
      {
      case SAVEBUTTON:
         qm100_saveConfigData(&qm100_configData);
         needSave = 0;
         break;
      case RELOADBUTTON:
         reloadValues();
         break;
      case DEFAULTSBUTTON:
         resetValues();
         break;
      case CLOCKBUTTON:
         setClock();
         break;
      default:
         printf("button_clicked: unknown button type %x\n", (int) user_data);
      case CLOSEBUTTON:
         closeDialog();
         break;
      }
}
/*---------------------------------------------------------------------*
 *                                                                     *
 * resetValues - reset everything to default values,                   *
 *               then re-display the dialog.                           *
 *                                                                     *
 *---------------------------------------------------------------------*/
static void resetValues(void)
{
   printf("Reset to Defaults not yet implemented\n");
   needSave = 1;
}
static void reloadValues(void)
{
   printf("Reload not yet implemented\n");
   needSave = 0;
}
static void closeDialog(void)
{
   if (needSave)
      {
      printf("Data not saved\n");
      return;
      }
   if (qm100_main)
      gtk_main_quit();
   else
      gtk_widget_destroy(qm100ConfigDlg) ;
}
static void setClock(void)
{
   showCameraClock();
}
static void showCameraClock(void)
{
   gtk_label_set_text(GTK_LABEL(clockLabel), "new clock value");
   gtk_widget_show(clockLabel);
}
