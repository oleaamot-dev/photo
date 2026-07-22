#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "eph_io.h"
#include "olympus.h"
#include "photopc-usb.h"
#include "../src/gphoto.h"

/* Olympus Camera Functions ----------------------------------
   ----------------------------------------------------------- */

/*
 * (2jul2000) Adam Fritzler (mid@auk.cx): Upgraded to latest
 * version of photopc (3.04) and added in the descend function
 * from there in order to get this working on my C-2500L.
 */

eph_iob   *iob;                /* Olympus/photoPC io-buffer    */
struct olympus_device *gpdev;

extern float progress_min;
extern float progress_max;
	
static void
oly_update_progress (off_t count, off_t total)
{
	float percent = (float)count/(float)total;
	if (total)
		update_progress (progress_min+percent*(progress_max-progress_min));
}

int
oly_initialize () {

	iob = eph_new(NULL, NULL, oly_update_progress, NULL, 0);
	return 1;
}

#define MAXPATH 256

/*
 * xchdir, descend, and nonempty are from photopc 3.04.  Modified
 * for the context.  Cameras not supporting folders should not
 * reach this code, but should still work if they do. -mid 
 */
static int 
xchdir(eph_iob *iob,char *where, int have_folders)
{
      char path[MAXPATH],*p;

      if (!have_folders)
	      return -1;
      
      strncpy(path,where,sizeof(path)-1);
      path[sizeof(path)-1]='\0';
      for (p=path;*p;p++) if (*p == '/') *p='\\';
      if (path[0] == '\\') {
	      if (eph_setvar(iob,84,"\\",1)) {
		      fprintf(stderr,"could not chdir to \"\\\"\n");
		      return -1;
	      }
      }
      for (p=strtok(path,"\\");p;p=strtok(NULL,"\\")) {
	      if (eph_setvar(iob,84,p,strlen(p))) {
		      fprintf(stderr,"could not chdir to \"%s\"\n",p);
		      return -1;
	      }
      }
      return 0;
}

static int 
descend(eph_iob *iob,int depth,char *root,int(*each)(eph_iob *iob,char *path), int have_folders)
{
      long nfolders=0L,i;
      off_t f_size;
      char *folder,*p;
      char path[MAXPATH];
      int rc;
      int blah;

      if (!have_folders) return (*each)(iob,root);

      if (depth > 10) {
	      fprintf(stderr,"cannot be that deep (%d), error!\n",depth);
	      return -1;
      }
      if (xchdir(iob,root,have_folders)) {
	      fprintf(stderr,"chdir to \"%s\" failed\n",root);
	      return -1;
      }

      if ((rc=(*each)(iob,root)) != 0)
	      return rc;

      eph_getint(iob,83,&nfolders);

      if (nfolders == 0L) return 0;

      folder=(char*)malloc(2048);
      f_size=2048;
      path[sizeof(path)-1]='\0';
      for (i=1;i<=nfolders;i++) {
	      eph_setint(iob,83,i);
	      eph_getvar(iob,84,&folder,&f_size);

	      strncpy(path,root,sizeof(path)-2);
	      if (path[strlen(path)-1] != '\\')
		      strncat(path,"\\",sizeof(path)-2);
	      strncat(path,folder,sizeof(path)-2);
	      for (p=path+strlen(path)-1;(*p == ' ') && (p > path);p--)
		      *p='\0';
	      if ((rc=descend(iob,depth+1,path,each,have_folders))) {
		      free(folder);
		      return rc;
	      }
	      if (xchdir(iob,root,have_folders)) {
		      fprintf(stderr,"restore dir to \"%s\" failed\n",root);
		      return -1;
	      }
      }
      free(folder);

      return 0;
}

static int
nonempty(eph_iob *iob,char *path)
{
      unsigned long result;

      if (eph_getint(iob,10,&result)) {
	      return -1;
      }
      if (result) {
	      return 1;
      } else return 0;
}

int 
oly_init2(eph_iob *iob)
{
	long ret;
	int rc;

	if (eph_getint(iob,1,&ret)) return -1;
	
      eph_setnullint(iob,83);
      
      if (!eph_setvar(iob,84,"\\",1)) {
	      if (descend(iob, 0, "\\", nonempty, 1) < 0)
		      return -1;
	}

      return 0;
}

int 
oly_open_camera ()
{

	/* Open the camera for reading/writing */

	long ltemp;

	if (camera_type == GPHOTO_CAMERA_USB) {
		char *info;
		long info_size;

		gpdev = olympus_usb_open();
		if (!gpdev)
			return 0;
		info_size=16;
		info=malloc(info_size);
	} else {
		if (eph_open(iob, serial_port, 115200, 0, 0) == -1)
			return 0;
	}

	oly_init2(iob);
	return 1;
}

void 
oly_close_camera() 
{

	/* Close the camera */

	if (camera_type == GPHOTO_CAMERA_USB)
		olympus_usb_close(gpdev);
	else
		eph_close(iob, 0);
}

int 
oly_number_of_pictures () 
{

	long num_pictures_taken = 0;

	if (oly_open_camera() == 0)
		return (0);

	sleep(1);
	eph_getint(iob, 0x0a, &num_pictures_taken);
	oly_close_camera();

	return ((int)num_pictures_taken);
}

int 
oly_take_picture () 
{

	char zero = 0;

	if (oly_open_camera() == 0)
		return (0);

	eph_action(iob,2,&zero,1);
	oly_close_camera();

	return (oly_number_of_pictures());
}

struct Image *
oly_get_picture (int picNum, int thumbnail) 
{

	/*
	   Reads image #picNum the Olympus camera.
	   If thumbnail == TRUE, it reads just the thumbnail.
	   If thumbnail == FALSE, it reads the whole image.
	*/


	long thumbLength, picLength, Size;
	char *picData;
	char tempName[1024];
	long picSize;
	int pid;
	struct Image *im = NULL;

	if (picNum != 0) {
		if (oly_open_camera() == 0)
			return(im);
	}

	eph_setint(iob, 4, (long)picNum);
	eph_getint(iob, 0x0d, &thumbLength);
	eph_getint(iob, 0x0c, &picLength);

	if (thumbnail)
		Size = thumbLength;
	   else 
		Size = thumbLength + picLength;
	Size = ((Size-1)/2048+2)*2048;

	picData = malloc(Size);
	picSize = Size;

	if (thumbnail)
		eph_getvar(iob, 0x0f, &picData, &picSize);
	   else
		eph_getvar(iob, 0x0e, &picData, &picSize);
	pid = getpid();
	if (thumbnail)
		sprintf(tempName, "%s/gphoto-thumb-%i-%i.jpg",
			gphotoDir, pid, picNum);
	   else
		sprintf(tempName, "%s/gphoto-%i-%i.jpg", 
			gphotoDir, pid, picNum);
	im = (struct Image*)malloc(sizeof(struct Image));
	im->image = picData;
	im->image_size = Size;
	im->image_info_size = 0;
	strcpy(im->image_type, "jpg");

	oly_close_camera();
	return (im);
}

struct Image *
oly_get_preview () 
{

	char zero = 0;

	if (oly_open_camera() == 0) {
		dialog_error("Could not open camera.");
		return NULL;
	}
	eph_action(iob,5,&zero,1);
	return (oly_get_picture(0, 0));
}

int 
oly_configure () 
{

	/*
	   Shows the Olympus config dialog
	*/

	char *info, *camID;
	off_t info_size = 2048;
	time_t camtime;
	time_t gmttime;
	struct tm *loctime;
	char *atime;

	long value;

	GtkWidget *dialog, *table, *label, *spacer, *toggle;
	GtkWidget *save_button, *cancel_button;
	GtkObject *adj;
	GtkAdjustment *adjustment;
	GSList *group;

	struct ConfigValues {
		GtkWidget *cam_id;
		GtkWidget *qual_std;
		GtkWidget *qual_high;
		/* GtkWidget *qual_best; */
		GtkWidget *lcd;
		GtkWidget *docked;	
		GtkWidget *undocked;
		GtkWidget *lens_mac;
		GtkWidget *lens_norm;
		GtkWidget *flash_auto;
		GtkWidget *flash_red;
		GtkWidget *flash_force;
		GtkWidget *flash_none;
		GtkWidget *date_yymmdd;
	 	GtkWidget *date_ddmmhh;
		GtkWidget *clk_comp;
		GtkWidget *clk_none;
	} Config;

	info = malloc(2048);

	update_status("Getting Camera Configuration...");

	/* dialog window ---------------------- */
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW(dialog), "Configure Camera");
	gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
				   10);
	table = gtk_table_new(15,5,FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	spacer = gtk_vseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,2,3,2,15);	

	if (oly_open_camera() == 0) {
		dialog_error("Could not open camera.");
		return 0;
	}

	/* camera id ---------------------- */
	label = gtk_label_new("Camera ID:");
	gtk_widget_show(label);
	Config.cam_id = gtk_entry_new();
	gtk_widget_show(Config.cam_id);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.cam_id,1,5,0,1);

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,5,1,2);	

	info[0] = '\0'; info_size=2048;
	eph_getvar(iob,0x16,&info,&info_size);
	update_progress(12);
	gtk_entry_set_text(GTK_ENTRY(Config.cam_id), info);

	/* image quality ---------------------- */
	label = gtk_label_new("Image Quality:");
	gtk_widget_show(label);
	Config.qual_std = gtk_radio_button_new_with_label(NULL, "Standard");
	gtk_widget_show(Config.qual_std);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.qual_std));
	Config.qual_high = gtk_radio_button_new_with_label(group, "High");
	gtk_widget_show(Config.qual_high);
/*
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.qual_high));
	Config.qual_best = gtk_radio_button_new_with_label(group, "Best");
	gtk_widget_show(Config.qual_best);
*/
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.qual_std,1,2,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.qual_high,1,2,3,4);	
/*	gtk_table_attach_defaults(GTK_TABLE(table),Config.qual_best,1,2,4,5);*/

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,2,5,6);	

	eph_getint(iob,1,&value);
	update_progress(25);
	switch (value) {
		case 1:
			gtk_widget_activate(Config.qual_std);
			break;
		default:
			gtk_widget_activate(Config.qual_high);
	}
			

	/* lcd brightness ---------------------- */
	label = gtk_label_new("LCD Brightness:");
	gtk_widget_show(label);
	eph_getint(iob,35,&value);
	update_progress(37);
	adj = gtk_adjustment_new(value, 1, 7, 1, 0, 0);
	Config.lcd = gtk_hscale_new(GTK_ADJUSTMENT(adj));
	gtk_range_set_update_policy(GTK_RANGE(Config.lcd),GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_draw_value(GTK_SCALE(Config.lcd), TRUE);
	gtk_scale_set_digits(GTK_SCALE(Config.lcd), 0);
	gtk_widget_show(Config.lcd);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,6,7);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.lcd,0,2,7,8);	

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,2,8,9);	

	/* power save -------------------------- */
	label = gtk_label_new("Power Saving:");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,9,10);	
	label = gtk_label_new("Docked");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,10,11);	
	label = gtk_label_new("Undocked");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,11,12);	
	eph_getint(iob,23,&value);
	adj = gtk_adjustment_new(value, 0, 600, 5, 0, 0);
	Config.docked = gtk_hscale_new(GTK_ADJUSTMENT(adj));
	gtk_range_set_update_policy(GTK_RANGE(Config.docked),
				    GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_draw_value(GTK_SCALE(Config.docked), TRUE);
	gtk_scale_set_digits(GTK_SCALE(Config.docked), 0);
	gtk_widget_show(Config.docked);
	eph_getint(iob,24,&value);
	adj = gtk_adjustment_new(value, 0, 180, 5, 0, 0);
	Config.undocked = gtk_hscale_new(GTK_ADJUSTMENT(adj));
	gtk_range_set_update_policy(GTK_RANGE(Config.undocked),
				    GTK_UPDATE_CONTINUOUS);
	gtk_scale_set_draw_value(GTK_SCALE(Config.undocked), TRUE);
	gtk_scale_set_digits(GTK_SCALE(Config.undocked), 0);
	gtk_widget_show(Config.undocked);
	gtk_table_attach_defaults(GTK_TABLE(table),Config.docked,0,1,10,11);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.undocked,0,1,11,12);	

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,2,12,13);	
	update_progress(50);

	/* lens mode ----------------------- */
	label = gtk_label_new("Lens Mode:");
	gtk_widget_show(label);
	Config.lens_norm = gtk_radio_button_new_with_label(NULL, "Normal");
	gtk_widget_show(Config.lens_norm);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.lens_norm));
	Config.lens_mac = gtk_radio_button_new_with_label(group, "Macro");
	gtk_widget_show(Config.lens_mac);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,13,14);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.lens_norm,1,2,13,14);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.lens_mac,1,2,14,15);	
	
	eph_getint(iob,33,&value);
	update_progress(62);
	switch (value) {
		case 1:
			gtk_widget_activate(Config.lens_mac);
			break;
		default:
			gtk_widget_activate(Config.lens_norm);
	}

	/* flash mode ---------------------- */
	label = gtk_label_new("Flash Mode:");
	gtk_widget_show(label);
	Config.flash_auto = gtk_radio_button_new_with_label(NULL, "Auto");
	gtk_widget_show(Config.flash_auto);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.flash_auto));
	Config.flash_red = gtk_radio_button_new_with_label(group, "Red-eye");
	gtk_widget_show(Config.flash_red);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.flash_red));
	Config.flash_force = gtk_radio_button_new_with_label(group, "Force");
	gtk_widget_show(Config.flash_force);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.flash_force));
	Config.flash_none = gtk_radio_button_new_with_label(group, "None");
	gtk_widget_show(Config.flash_none);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_auto,4,5,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_red,4,5,3,4);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_force,4,5,4,5);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_none,4,5,5,6);	
	
	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,3,5,6,7);	

	eph_getint(iob,7,&value);
	update_progress(75);
	switch (value) {
		case 1:
			gtk_widget_activate(Config.flash_force);
			break;
		case 2:
			gtk_widget_activate(Config.flash_none);
			break;
		case 3:
			gtk_widget_activate(Config.flash_red);
			break;
		default:
			gtk_widget_activate(Config.flash_auto);
	}

	/* date format ------------------------- */
	label = gtk_label_new("Date Format:");
	gtk_widget_show(label);
	Config.date_yymmdd = gtk_radio_button_new_with_label(NULL, "YY MM DD");
	gtk_widget_show(Config.date_yymmdd);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.date_yymmdd));
	Config.date_ddmmhh = gtk_radio_button_new_with_label(group, "DD MM HH");
	gtk_widget_show(Config.date_ddmmhh);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,7,8);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.date_yymmdd,4,5,7,8);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.date_ddmmhh,4,5,8,9);	

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,3,5,9,10);	
	
	eph_getint(iob,41,&value);
	update_progress(87);
	switch (value) {
		case 2:
			gtk_widget_activate(Config.date_ddmmhh);
			break;
		default:
			gtk_widget_activate(Config.date_yymmdd);
	}	

	/* set clock ---------------------------- */
	label = gtk_label_new("Current Camera Time:");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,10,11);

	eph_getint(iob,2,&camtime);
	update_progress(100);
	atime = asctime(gmtime(&camtime));
	label = gtk_label_new(atime);
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,5,11,12);

	Config.clk_comp = gtk_radio_button_new_with_label(NULL,
							 "Set to Computer");
	gtk_widget_show(Config.clk_comp);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.clk_comp));
	Config.clk_none = gtk_radio_button_new_with_label(group, "No Change");
	gtk_widget_show(Config.clk_none);

	gtk_table_attach_defaults(GTK_TABLE(table),Config.clk_comp,4,5,12,13);
	gtk_table_attach_defaults(GTK_TABLE(table),Config.clk_none,4,5,13,14);

	gtk_widget_activate(Config.clk_none);

	/* WOW that was a lot of code... now connect some stuff... */

	oly_close_camera();
	toggle = gtk_toggle_button_new();
	gtk_widget_show(toggle);
	gtk_widget_hide(toggle);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   toggle, TRUE, TRUE, 0);

	save_button = gtk_button_new_with_label("Save");
	gtk_widget_show(save_button);
	GTK_WIDGET_SET_FLAGS (save_button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   save_button, FALSE, FALSE, 0);
	cancel_button = gtk_button_new_with_label("Cancel");
	gtk_widget_show(cancel_button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   cancel_button, FALSE, FALSE, 0);
	gtk_widget_grab_default (save_button);

	gtk_object_set_data(GTK_OBJECT(dialog), "button", "CANCEL");
	gtk_widget_show(dialog);
	update_status("Done.");
	update_progress(0);

	if (wait_for_hide(dialog, save_button, cancel_button) == 0)
		return (1);

	update_status("Saving Configuration...");

	if (oly_open_camera() == 0) {
		dialog_error("Could not open camera.");
		return 0;
	}
	update_progress(0);

	/* Set camera name... */
	camID = gtk_entry_get_text(GTK_ENTRY(Config.cam_id));
	eph_setvar(iob,0x16,camID,strlen(camID));
	update_progress(12);


	/* Set image quality... */
	if (GTK_WIDGET_STATE(Config.qual_std) == GTK_STATE_ACTIVE)
		value = 1;
	  else
		value = 2;

	eph_setint(iob,1,value);
	update_progress(25);

	/* Set flash mode... */
	if (GTK_WIDGET_STATE(Config.flash_auto) == GTK_STATE_ACTIVE)
		value = 0;
	 else if (GTK_WIDGET_STATE(Config.flash_force) == GTK_STATE_ACTIVE)
 		value = 1;
	 else if (GTK_WIDGET_STATE(Config.flash_none) == GTK_STATE_ACTIVE)
 		value = 2;
	 else 
 		value = 4;
	eph_setint(iob,7,value);
	update_progress(37);

	/* Set lens mode... */
	if (GTK_WIDGET_STATE(Config.lens_mac) == GTK_STATE_ACTIVE)
		value = 1;
	   else
		value = 2;
	eph_setint(iob,33,value);
	update_progress(50);

	/* Set date format... */
	if (GTK_WIDGET_STATE(Config.date_yymmdd) == GTK_STATE_ACTIVE)
		value = 1;
	   else
		value = 2;
	eph_setint(iob,41,value);
	update_progress(62);

	/* Set LCD brightness ... */
	adjustment = gtk_range_get_adjustment(GTK_RANGE(Config.lcd));
	value = adjustment->value;
	eph_setint(iob,35,value);
	update_progress(75);

	/* Set power savings ... */
	adjustment = gtk_range_get_adjustment(GTK_RANGE(Config.docked));
	value = adjustment->value;
	eph_setint(iob,23,value);
	adjustment = gtk_range_get_adjustment(GTK_RANGE(Config.undocked));
	value = adjustment->value;
	eph_setint(iob,24,value);
	update_progress(87);

	/* Set the clock... */
	if (GTK_WIDGET_STATE(Config.clk_comp) == GTK_STATE_ACTIVE) {
		gmttime = time(NULL);
		loctime = localtime(&gmttime);

		/*
		 * Let's get the Seconds east of UTC
		 */
#ifdef __USE_BSD
		eph_setint(iob,2,gmttime+loctime->tm_gmtoff);
#else
		eph_setint(iob,2,gmttime+loctime->__tm_gmtoff);
#endif
	}
	update_progress(100);
	oly_close_camera();
	gtk_widget_destroy(dialog);
	update_status("Done.");
	update_progress(0);
	return 1;
}

int 
oly_delete_image (int picNum) 
{

	/*
	   deletes image #picNum from the Olympus camera.
	*/

	char z=0;

	if (oly_open_camera() == 0)
		return 0;

	eph_setint(iob,4,(long)picNum);
	sleep(2);
	eph_action(iob,7,&z,1);
	oly_close_camera();
	return (1);
}

char oly_summary_text[1024];


char *
oly_summary() 
{

	unsigned long l;
	char *s;
	off_t s_size;

	sprintf(oly_summary_text, "");

	s = (char*)malloc(sizeof(char)*2048);

	oly_open_camera();

	eph_getint(iob, 3, &l);
	sprintf(oly_summary_text, "%sShutter: %lu\n", 
		oly_summary_text, l);

	eph_getint(iob, 6, &l);
	sprintf(oly_summary_text, "%sColor Mode: %s\n", oly_summary_text,
		(l==1)?"Color":(l==2)?"B/W":"Unknown");

	eph_getint(iob, 10, &l);
	sprintf(oly_summary_text, "%sFrames Taken: %lu\n", 
		oly_summary_text, l);

	eph_getint(iob, 11, &l);
	sprintf(oly_summary_text, "%sFrames Left: %lu\n",
		oly_summary_text, l);

	eph_getint(iob, 16, &l);
	sprintf(oly_summary_text, "%sBattery: %lu%%\n", 
		oly_summary_text, l);

	eph_getint(iob, 28, &l);
	sprintf(oly_summary_text, "%sFree Memory: %lu bytes\n", 
		oly_summary_text, l);

	s_size = 2048; s[0] = '\0';
	eph_getvar(iob, 0x19, &s, &s_size);
	sprintf(oly_summary_text, "%sSerial #: %s\n", oly_summary_text, s);

	s_size = 2048; s[0] = '\0';
	eph_getvar(iob, 0x1a, &s, &s_size);
	sprintf(oly_summary_text, "%sVersion: %s\n", oly_summary_text, s);
	
	s_size = 2048; s[0] = '\0';
	eph_getvar(iob, 0x1b, &s, &s_size);
	sprintf(oly_summary_text, "%sModel: %s\n", oly_summary_text, s);

	s_size = 2048; s[0] = '\0';
	eph_getvar(iob, 48, &s, &s_size);
	sprintf(oly_summary_text, "%sManufacturer: %s\n", oly_summary_text, s);

	free(s);

	oly_close_camera();

	return (oly_summary_text);
}

char *oly_description() {

	return(
"Olympus/PhotoPC Digital Camera Support
Scott Fritzinger <scottf@unr.edu>
using the photoPC library by
Eugene Crosser <crosser@average.org>
http://www.average.org/digicam");
}

/* Declare the camera function pointers */

struct _Camera olympus = {oly_initialize,
			  oly_get_picture,
			  oly_get_preview,
			  oly_delete_image,
			  oly_take_picture,
			  oly_number_of_pictures,
			  oly_configure,
			  oly_summary,
			  oly_description,
			  NULL};

/* End of Olympus Camera functions ------------------------------
   -------------------------------------------------------------- */
