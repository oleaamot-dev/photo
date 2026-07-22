/*
 *	Konica-qm-sio-sample version 1.02
 *
 *	Copyright (C) 1999 Konica corporation .
 *
 *                                <qm200-support@konica.co.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "../src/gphoto.h"
#include "../src/util.h"

// #define DEBUG

#include "log.h"
#include "os.h"
#include "dcx.h"

#define ANS_OK	 1
#define ANS_NG	-1

/*----------------------------------------------------------------------*/
static void
disp_func(long percent)
{
	MSG(_ksprintf("%3d%%\n", percent ));
}
/*----------------------------------------------------------------------*/
static int
qm_initialize(void)
{
	DB(_ksprintf("qm_initialize\n"));
	return(ANS_OK);
}
/*----------------------------------------------------------------------*/
static int
qm_number_of_pictures(void)
{
	DB(_ksprintf("qm_number_of_picture\n"));
	return dcx_get_number_of_pictures();
}
/*----------------------------------------------------------------------*/
static int
qm_take_picture(void)
{
	DB(_ksprintf("qm_take_picture\n"));
	if( dcx_take_picture() == NG ){
		return ANS_NG;
	}
	return ANS_OK;
}
/*----------------------------------------------------------------------*/
static struct Image *
qm_get_picture(int no, int thumbnail)
{
	struct Image	*im;
	dcx_image_t	ans;

	DB(_ksprintf("qm_get_picture\n"));

	im = malloc(sizeof(struct Image));

	if( thumbnail ){
		if( dcx_alloc_and_get_thum(no, &ans) == NG ){ return NULL; };
	}else{
		if( dcx_alloc_and_get_exif(no, &ans, disp_func) == NG ){ return NULL; };
	}
	im->image      = ans.alloc_buf;
	im->image_size = ans.size;

        #if 1 /* 1999.10.1 */
	im->image_info_size = 0;
	strcpy(im->image_type, "jpg"); /* jpeg only */
        #endif

	return im;
}
/*----------------------------------------------------------------------*/
static int
qm_delete_picture(int no)
{
	DB(_ksprintf("qm_delete_picture\n"));
	if( dcx_delete_picture(no) == NG ){ return ANS_NG; }
	return ANS_OK;
}
/*----------------------------------------------------------------------*/
int
static qm_formatCF(void)
{
	DB(_ksprintf("qm_formatCF\n"));
	if( dcx_format_cf_card() == NG ){ return ANS_NG; };
	return ANS_OK;
}
/*----------------------------------------------------------------------*/
static struct Image *
qm_get_preview(void)
{
	DB(_ksprintf("qm_get_preview\n"));
	return (struct Image *)NULL;
}
/*----------------------------------------------------------------------*/
int
static qm_configure(void)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *buttonbox;

	DB(_ksprintf("qm_configure\n"));

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
		       GTK_SIGNAL_FUNC(qm_formatCF),
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
	gtk_widget_show ( dialog );
  
	return ANS_OK;
}
/*----------------------------------------------------------------------*/
static char *
qm_summary()
{
	dcx_summary_t		ans;
	static char		buf[1024];

	DB(_ksprintf("qm_summary"));
	if( dcx_get_summary(&ans) == NG ){
		return "Error!!";
	}

	#if 0 /* delete 1999.10.1 for gphoto-0.3.9 */
	update_progress(0);
	update_progress(1);
	#endif

	sprintf(buf, 
		"This camera is a Konica QM100/200 \n"
		"It has taken %ld pictures and currently contains %ld picture(s)\n"
		"The time according to the Camera is %d:%d:%d %d/%d/%d",
			ans.total_pict, ans.picture_count,
			(int)ans.hour, (int)ans.minute, (int)ans.second,
			(int)ans.day, (int)ans.month, (int)ans.year);

	return buf;
}
/*----------------------------------------------------------------------*/
static int
setup(dcx_camera_model_t camera_model)
{
	char *str;

	switch( camera_model ){
	case QM100:         str = "QM100";   break;
	case QM200:         str = "QM200";   break;
	case UNKNOWN_MODEL: str = "UNKNOWN"; break;
	default:            str = "BUG";     break;
	}

	DB(_ksprintf("setup port=[%s] model=%s\n", serial_port, str ));

	if( dcx_init(serial_port, camera_model) == NG ){
		return ANS_NG;
	}
	return ANS_OK;
}
/*======================================================================*/
static int
konica_qm1xx_initialize(void){
	if( setup(QM100) == ANS_NG ) return ANS_NG;
	return qm_initialize();
}
/*----------------------------------------------------------------------*/
static int
konica_qm2xx_initialize(void){
	if( setup(QM200) == ANS_NG ) return ANS_NG;
	return qm_initialize();
}
/*======================================================================*/
static int
konica_qm1xx_number_of_pictures(void)
{
	if( setup(QM100) == ANS_NG ) return ANS_NG;
	return qm_number_of_pictures();
}
/*----------------------------------------------------------------------*/
static int
konica_qm2xx_number_of_pictures(void)
{
	if( setup(QM200) == ANS_NG ) return ANS_NG;
	return qm_number_of_pictures();
}
/*======================================================================*/
static int
konica_qm1xx_take_picture(void)
{
	if( setup(QM100) == ANS_NG ) return ANS_NG;
	return qm_take_picture();
}
/*----------------------------------------------------------------------*/
static int
konica_qm2xx_take_picture(void)
{
	if( setup(QM200) == ANS_NG ) return ANS_NG;
	return qm_take_picture();
}
/*======================================================================*/
static struct Image *
konica_qm1xx_get_picture(int no, int thumbnail)
{
	if( setup(QM100) == ANS_NG ) return (struct Image *)NULL;
	return qm_get_picture(no, thumbnail);
}
/*----------------------------------------------------------------------*/
static struct Image *
konica_qm2xx_get_picture(int no, int thumbnail)
{
	if( setup(QM200) == ANS_NG ) return (struct Image *)NULL;
	return qm_get_picture(no, thumbnail);
}
/*======================================================================*/
static int
konica_qm1xx_delete_picture(int no)
{
	if( setup(QM100) == ANS_NG ) return ANS_NG;
	return qm_delete_picture(no);
}
/*----------------------------------------------------------------------*/
static int
konica_qm2xx_delete_picture(int no)
{
	if( setup(QM200) == ANS_NG ) return ANS_NG;
	return qm_delete_picture(no);
}
/*======================================================================*/
static int
konica_qm1xx_formatCF(void)
{
	if( setup(QM100) == ANS_NG ) return ANS_NG;
	return qm_formatCF();
}
/*----------------------------------------------------------------------*/
static int
konica_qm2xx_formatCF(void)
{
	if( setup(QM200) == ANS_NG ) return ANS_NG;
	return qm_formatCF();
}
/*======================================================================*/
struct Image *
konica_qm1xx_get_preview(void)
{
	if( setup(QM100) == ANS_NG ) return (struct Image *)NULL;
	return qm_get_preview();
}
/*----------------------------------------------------------------------*/
struct Image *
konica_qm2xx_get_preview(void)
{
	if( setup(QM200) == ANS_NG ) return (struct Image *)NULL;
	return qm_get_preview();
}
/*======================================================================*/
static int
konica_qm1xx_configure(void)
{
	if( setup(QM100) == ANS_NG ) return ANS_NG;
	return qm_configure();
}
/*----------------------------------------------------------------------*/
static int
konica_qm2xx_configure(void)
{
	if( setup(QM200) == ANS_NG ) return ANS_NG;
	return qm_configure();
}
/*======================================================================*/
static char *
konica_qm1xx_summary(void)
{
	if( setup(QM100) == ANS_NG ) return "QM100 SIO ERROR!!!";
	return qm_summary();
}
/*----------------------------------------------------------------------*/
static char *
konica_qm2xx_summary(void)
{
	if( setup(QM200) == ANS_NG ) return "QM200 SIO ERROR!!!";
	return qm_summary();
}
/*======================================================================*/
static char *
konica_qm1xx_description(void)
{
	DB(_ksprintf("qm100_description\n"));
	return "Konica QM-100 gPhoto Plugin (C) 1999 Konica corp.";
}
/*----------------------------------------------------------------------*/
static char *
konica_qm2xx_description(void)
{
	DB(_ksprintf("qm200_description\n"));
	return "Konica QM-200 gPhoto Plugin (C) 1999 Konica corp.";
}
/*======================================================================*/
struct _Camera konica_qm1xx = {konica_qm1xx_initialize,
			       konica_qm1xx_get_picture,
			       konica_qm1xx_get_preview,
			       konica_qm1xx_delete_picture,
			       konica_qm1xx_take_picture,
			       konica_qm1xx_number_of_pictures,
			       konica_qm1xx_configure,
			       konica_qm1xx_summary,
			       konica_qm1xx_description,
			       NULL};
/*----------------------------------------------------------------------*/
struct _Camera konica_qm2xx = {konica_qm2xx_initialize,
			       konica_qm2xx_get_picture,
			       konica_qm2xx_get_preview,
			       konica_qm2xx_delete_picture,
			       konica_qm2xx_take_picture,
			       konica_qm2xx_number_of_pictures,
			       konica_qm2xx_configure,
			       konica_qm2xx_summary,
			       konica_qm2xx_description,
			       NULL};
/*----------------------------------------------------------------------*/
/* end of 'qm.c' */
