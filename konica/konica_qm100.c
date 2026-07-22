/* Konica Q-M100 Camera Functions */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <gdk_imlib.h>
#include <gdk/gdk.h>
#include "qm100.h"
#include "../src/gphoto.h"
#include "konica_qm100.h"
int konica_picCounter;
void dialog_notice(char *title, char *message, GtkJustification  jtype );
void konica_show_camera_error(void);
#if defined(FULL_CONFIG)
#include "configDialog.h"
#else
int konica_qm100_formatCF()
{
  int serialdev;
  if (setjmp(qm100_jmpbuf))
     {
     konica_show_camera_error();
     return 0;
     }
  serialdev = qm100_open(serial_port);
  qm100_formatCF(serialdev);
  qm100_close(serialdev);
  return (1);
}
#endif
static void dlprogress(void)
{
   qm100_percent += qm100_percentIncr;
   if (qm100_percent)
	   update_progress((int)(100.0 * qm100_percent));
}
int konica_qm100_initialize()
{
   qm100_readConfigData(&qm100_configData);
   if (*serial_port == '\0')
      strcpy(serial_port, qm100_configData.device);
   qm100_setTrace();
   qm100_setTransmitSpeed();
   konica_picCounter = 0;
   qm100_recovery = 1;
   return(1);
}
void konica_show_camera_error(void)
{
   dialog_notice("Camera failure", qm100_errmsg, GTK_JUSTIFY_FILL);
}
int konica_qm100_number_of_pictures ()
{
  int serialdev;
  if (setjmp(qm100_jmpbuf))
     {
     konica_show_camera_error();
     return 0;
     }
  serialdev = qm100_open(serial_port);
  qm100_getStatus(serialdev, NULL);
  qm100_close(serialdev);
  return qm100_pictureCount;
}
int konica_qm100_take_picture()
{
  int serialdev;
  if (setjmp(qm100_jmpbuf))
     {
     konica_show_camera_error();
     return 0;
     }
  serialdev = qm100_open(serial_port);
  qm100_takePic(serialdev);
  qm100_getStatus(serialdev, NULL);
  qm100_close(serialdev);
  return qm100_pictureCount;
}
struct Image *konica_qm100_get_picture (int picNum, int thumbnail)
{
  int serialdev;
  int pid;
  char tempName[1024];
  FILE *jpgfile;
  int jpgfile_size;
  struct Image *im;
  pid = getpid();
  if (setjmp(qm100_jmpbuf))
     {
     konica_show_camera_error();
     return NULL;
     }
  serialdev = qm100_open(serial_port);
  konica_picCounter++;
  sprintf(tempName, "%s/gphoto-konica-%i-%i.jpg", gphotoDir, pid,
          konica_picCounter);
  picNum = qm100_getRealPicNum(serialdev, picNum);
  qm100_percent=0.;
  qm100_percentIncr=.003;
  if (thumbnail)
     qm100_saveThumb(serialdev, tempName, picNum, dlprogress);
  else
     qm100_savePic(serialdev, tempName, picNum, dlprogress);
  qm100_close(serialdev);
  jpgfile = fopen(tempName, "r");
  fseek(jpgfile, 0, SEEK_END);
  jpgfile_size = ftell(jpgfile);
  rewind(jpgfile);
  im = (struct Image*)malloc(sizeof(struct Image));
  im->image = (char *)malloc(sizeof(char)*jpgfile_size);
  fread(im->image, (size_t)sizeof(char), (size_t)jpgfile_size, jpgfile);
  fclose(jpgfile);
  if (thumbnail)
     strcpy(im->image_type, "tif");
  else
     strcpy(im->image_type, "jpg");
  im->image_size = jpgfile_size;
  im->image_info_size = 0;
  remove(tempName);
  return (im);
}
int konica_qm100_delete_picture (int picNum)
{
  int serialdev;
  if (setjmp(qm100_jmpbuf))
     {
     konica_show_camera_error();
     return 0;
     }
  serialdev = qm100_open(serial_port);
  picNum = qm100_getRealPicNum(serialdev, picNum);
  qm100_erasePic(serialdev, picNum);
  qm100_close(serialdev);
  return (1);
}
struct Image *konica_qm100_get_preview () {
  return(0);
}
int konica_qm100_configure ()
{
  GtkWidget *dialog;
#if defined(FULL_CONFIG)
  dialog = qm100_createConfigDlg();
#else
  GtkWidget *button;
  GtkWidget *buttonbox;
  /* Set the Window Dialog up */
  dialog = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_window_set_title(GTK_WINDOW(dialog), "Camera Configuration");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  /* Set the buttonbox up */
  buttonbox = gtk_hbutton_box_new();
  gtk_container_border_width(GTK_CONTAINER(buttonbox), 10);
  /* One nice button which calls formatCf please... */
  button = gtk_button_new_with_label ( "Format CF Card" );
  gtk_signal_connect ( GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(konica_qm100_formatCF),
		       NULL);
  gtk_signal_connect_object ( GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(dialog) );
  gtk_container_add ( GTK_CONTAINER(buttonbox), GTK_WIDGET(button));
  gtk_widget_show ( button );
  /* Another to kill this whole window if you will Sir */
  button = gtk_button_new_with_label ( " Cancel " );
  gtk_signal_connect_object ( GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(dialog) );
  gtk_container_add ( GTK_CONTAINER(buttonbox), GTK_WIDGET(button));
  gtk_widget_show ( button );
  /* Stick the buttonbox to the window and show it all */
  gtk_container_add ( GTK_CONTAINER(dialog), GTK_WIDGET(buttonbox));
  gtk_widget_show( buttonbox );
#endif
  gtk_widget_show ( dialog );
  return(1);
}
char *konica_qm100_summary()
{
  char summary[512];
  int serialdev;
  QM100_CAMERA_INFO  cinfo;
  update_progress(0);
  if (setjmp(qm100_jmpbuf))
     return qm100_errmsg;
  serialdev = qm100_open(serial_port);
  qm100_getStatus(serialdev, &cinfo);
  sprintf(summary,
          "\nThis camera is a %s,\n"
          "product code %-4.4s, serial # %-10.10s.\n"
          "It has taken %u pictures and currently contains %d picture(s).\n"
          "The time according to the Camera is %d/%02d/%02d %02d:%02d:%02d\n",
          cinfo.name, cinfo.product, cinfo.serial,
          cinfo.totalCount, cinfo.pictureCount,
          cinfo.year+1900, cinfo.month, cinfo.day,
          cinfo.hour, cinfo.min, cinfo.sec);
  qm100_close(serialdev);
  update_progress(100);
  return (strdup(summary));
}
char *konica_qm100_description()
{
  return("Konica plugin for gPhoto\n"
         "(C) 1999 Phill Hugo/Jesper Skov\n"
         "Works for:\n"
         "\t\t\t\t\tKonica Q-M100\n"
         "\t\t\t\t\tHP C20\n"
         "\t\t\t\t\tHP C30");
}
/* Declare the camera function pointers */
struct _Camera konica_qm100 = {konica_qm100_initialize,
			       konica_qm100_get_picture,
			       konica_qm100_get_preview,
			       konica_qm100_delete_picture,
			       konica_qm100_take_picture,
			       konica_qm100_number_of_pictures,
			       konica_qm100_configure,
			       konica_qm100_summary,
			       konica_qm100_description,
			       NULL};
