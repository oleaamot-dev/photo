#include "config.h"
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <gdk_imlib.h>
#include <gdk/gdk.h>

#include "../src/gphoto.h"
#include "sdComm.h"
#include "printMsg.h"
#include "messages.h"
#include "configure.h"
#include "command.h"
#include "qv_io.h"
#include "casio_qv_defines.h"

#define CASIO_CONFIG_FILE "casiorc"

static sdcInfo cameraPort;

static int usePortSpeed = DEFAULT;
static int lowResPictureSize = SIZE_320_x_240;
static int doDebug = 0;

int currentBaudRate = DEFAULTBAUD;
int qvverbose = 0;
int qv7xxprotocol = 0;

void
read_casio_config() {
    char configFileName[1024];
    char buf[1024];
    char prop[256];
    FILE *configFP;
    
    sprintf(configFileName, "%s/%s", gphotoDir, CASIO_CONFIG_FILE);
    configFP = fopen(configFileName, "r");
    if (configFP == NULL)
        return;

    while(fgets(buf, 1024, configFP)) {
	sscanf(buf, "%s", prop);
	if (strcmp(prop, "PortSpeed") == 0) {
	    sscanf(buf, "%s %d", prop, &usePortSpeed);
	} else if (strcmp(prop, "LowResPictureSize") == 0) {
	    sscanf(buf, "%s %d", prop, &lowResPictureSize);
	} 
    }
    fclose(configFP);
}

void
write_casio_config() {
    char configFileName[1024];
    FILE *configFP;
    
    sprintf(configFileName, "%s/%s", gphotoDir, CASIO_CONFIG_FILE);
    configFP = fopen(configFileName, "w");
    if (configFP == NULL) {
	print_error(CANT_OPEN_FILE, "configuration file", configFileName);
	return;
    }

    fprintf(configFP, "PortSpeed %d\n", usePortSpeed);
    fprintf(configFP, "LowResPictureSize %d\n", lowResPictureSize);
    fclose(configFP);
}
    
int
casio_qv_initialize() {
    cameraPort = sdcInit(serial_port);
    if (cameraPort == NULL) return GPHOTO_FAIL;

    read_casio_config();
    return(GPHOTO_SUCCESS);
}

static int
convertToBaudRate(int speed) {
    int baud;

    switch(speed) {
	case LIGHT:			/* 115200 baud */
#if defined(WIN32) || defined (OS2) || defined(BSD) || defined(DOS) || defined(__linux__)
	    baud = B115200;
#else
	    baud = B38400;
#endif
	    break;

	case TOP:			/* 57600 baud */
#if defined(WIN32) || defined(OS2) || defined(BSD) || defined(DOS) || defined(__linux__)

	    baud = B57600;
#else
	    baud = B38400;
#endif
	    break;
	    
	case HIGH:			/* 38400 baud */
	    baud = B38400;
	    break;
	    
	case MID:			/* 19200 baud */
	    baud = B19200;
	    break;
	    
	case DEFAULT:
	default:
	    baud = B9600;
	    break;
    }

    return(baud);
}

void
casio_set_config(int photoSize, int speed, int debug) {
    lowResPictureSize = photoSize;
    usePortSpeed = speed;
    doDebug = debug;
    sdcDebug(cameraPort, doDebug);
    write_casio_config();
}

int
casio_qv_open_camera() {
    int baudrate = convertToBaudRate(usePortSpeed);
    
    if (sdcIsClosed(cameraPort)) {
	if (sdcOpen(cameraPort) == SDC_FAIL ||
	    sdcSetBaudRate(cameraPort, currentBaudRate) == SDC_FAIL) {
	    return(GPHOTO_FAIL);
	}
	if (currentBaudRate != baudrate)
	    casioSetPortSpeed(cameraPort, usePortSpeed);
    }

    return(GPHOTO_SUCCESS);
}

void
casio_qv_close_camera() {
    if (!sdcIsClosed(cameraPort)) {
	QVreset(cameraPort, 0);
	sdcClose(cameraPort);
    }
}

char *
casio_qv_description() {
    return("Casio QV plugin for gPhoto\n"
    	   "Gary Ross <gdr@hooked.net\n"
	   "Adapted from qvplay093 program\n"
	   "Tested only on QV-10 so far.");
}

struct Image *
casio_qv_get_picture(int picture_number, int thumbnail) {
    struct Image *cameraImage;

    if (casio_qv_open_camera() == SDC_FAIL) {
	return(NULL);
    }

    QVsectorsize(cameraPort, NEW_SECTOR_SIZE);

    if (thumbnail) {
	cameraImage = casio_qv_download_thumbnail(cameraPort, picture_number);
    } else {
	cameraImage = casio_qv_download_picture(cameraPort, picture_number,
						lowResPictureSize);
    }

    casio_qv_close_camera();

    return(cameraImage);
}

int
casio_qv_delete_picture(int picture_number) {

    if (casio_qv_open_camera() == GPHOTO_FAIL) {
	return(GPHOTO_FAIL);
    }
    
    if (QVdeletepicture(cameraPort, picture_number) == GPHOTO_FAIL) {
	return(GPHOTO_FAIL);
    }

    casio_qv_close_camera();

    return(GPHOTO_SUCCESS);
}

int
casio_qv_number_of_pictures() {
    int attempts = 0;
    unsigned char c;

    if (casio_qv_open_camera() == SDC_FAIL) {
	return(0);
    }

    while (attempts < RETRY_COUNT) {
	if (QVok(cameraPort) != GPHOTO_SUCCESS)
	    return 0;

	if (casio_qv_write(cameraPort, "MP", 2) == SDC_FAIL)
	    return 0;

	if (casio_qv_read(cameraPort, &c, 1) == SDC_FAIL)
	    return 0;
	    
	if (c == 0x62) break;
    }
    
    casio_qv_send_byte(cameraPort, ACK);
    if (casio_qv_read(cameraPort, &c, 1) == SDC_FAIL)
	return 0;

    casio_qv_close_camera();
    return((int)c);
}

char *
casio_qv_summary() {
    static char summary[1024];
    char line[256];
    float batteryLevel;
    int baudRate;
    long revision;

    if (casio_qv_open_camera() == SDC_FAIL) {
	return(NULL);
    }

    strcpy(summary, "");

    batteryLevel = QVbattery(cameraPort);
    switch(sdcGetBaudRate(cameraPort)) {
	case B115200:
	    baudRate = 115200;
	    break;

	case B57600:
	    baudRate = 57600;
	    break;

	case B38400:
	    baudRate = 38400;
	    break;

	case B19200:
	    baudRate = 19200;
	    break;

	case B9600:
	default:
	    baudRate = 9600;
	    break;
    }

    revision = QVrevision(cameraPort);

    sprintf(line, "Casio Camera Library\n");
    strcat(summary, line);

    switch(revision) {
	case 0x00531719:
	case 0x00538b8f:
	    sprintf(line, "Model QV10 detected\n");
	    strcat(summary, line);
	    break;

	case 0x00800003:
	    sprintf(line, "Model QV10A detected\n");
	    strcat(summary, line);
	    break;

	case 0x00835321:
	    sprintf(line, "Model QV70 detected\n");
	    strcat(summary, line);
	    break;

	case 0x0103ba90:
	    sprintf(line, "Model QV100 detected\n");
	    strcat(summary, line);
	    break;

	case 0x01048dc0:
	    sprintf(line, "Model QV300 detected\n");
	    strcat(summary, line);
	    break;

	case 0x01a0e081:
	    sprintf(line, "Model QV700 detected\n");
	    strcat(summary, line);
	    break;

	case 0x01a10000:
	    sprintf(line, "Model QV770 detected\n");
	    strcat(summary, line);
	    break;
    }

    sprintf(line, "Connected to %s at %d baud\n", serial_port, baudRate);
    strcat(summary, line);

    sprintf(line, "%d photos in camera\n", casio_qv_number_of_pictures());
    strcat(summary, line);

    if (batteryLevel > 0) {
	sprintf(line, "Battery Level: %.1f Volts\n",
		      (float)batteryLevel / 16.0);
	strcat(summary, line);

	if (batteryLevel < LOW_BATT) {
	    strcat(summary,
	           "WARNING: Low Battery Level!! Replace the batteries\n\n");
	}
    }

    casio_qv_close_camera();

    return(summary);
}

static void
setDlgState(GtkWidget *cfgDlg) {
    GtkWidget *normalSize;
    GtkWidget *doubleSize;
    GtkWidget *spd_9600;
    GtkWidget *spd_19200;
    GtkWidget *spd_38400;
    GtkWidget *spd_57600;
    GtkWidget *spd_115200;
    GtkWidget *debugToggle;

    normalSize = gtk_object_get_data (GTK_OBJECT(cfgDlg), "normalSize");
    doubleSize = gtk_object_get_data (GTK_OBJECT(cfgDlg), "doubleSize");
    if (lowResPictureSize == SIZE_320_x_240) {
#ifdef GTK_HAVE_FEATURES_1_1_4
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(normalSize), TRUE);
#else
	if (!GTK_TOGGLE_BUTTON(normalSize)->active)
                gtk_button_clicked(GTK_BUTTON(normalSize));
#endif
    } else {
#ifdef GTK_HAVE_FEATURES_1_1_4
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(doubleSize), TRUE);
#else
	if (!GTK_TOGGLE_BUTTON(doubleSize)->active)
                gtk_button_clicked(GTK_BUTTON(doubleSize));
#endif


    }

    spd_9600 = gtk_object_get_data (GTK_OBJECT(cfgDlg), "spd_9600");
    spd_19200 = gtk_object_get_data (GTK_OBJECT(cfgDlg), "spd_19200");
    spd_38400 = gtk_object_get_data (GTK_OBJECT(cfgDlg), "spd_38400");
    spd_57600 = gtk_object_get_data (GTK_OBJECT(cfgDlg), "spd_57600");
    spd_115200 = gtk_object_get_data (GTK_OBJECT(cfgDlg), "spd_115200");
    switch(usePortSpeed) {
	case LIGHT:
#ifdef GTK_HAVE_FEATURES_1_1_4
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(spd_115200), TRUE);
#else
	    if (!GTK_TOGGLE_BUTTON(spd_115200)->active)
            	gtk_button_clicked(GTK_BUTTON(spd_115200));
#endif
	    break;

	case TOP:
#ifdef GTK_HAVE_FEATURES_1_1_4
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(spd_57600), TRUE);
#else
	    if (!GTK_TOGGLE_BUTTON(spd_57600)->active)
                gtk_button_clicked(GTK_BUTTON(spd_57600));
#endif
	    break;

	case HIGH:
#ifdef GTK_HAVE_FEATURES_1_1_4
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(spd_38400), TRUE);
#else
	    if (!GTK_TOGGLE_BUTTON(spd_38400)->active)
                gtk_button_clicked(GTK_BUTTON(spd_38400));
#endif
	    break;

	case MID:
#ifdef GTK_HAVE_FEATURES_1_1_4
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(spd_19200), TRUE);
#else
	    if (!GTK_TOGGLE_BUTTON(spd_19200)->active)
                gtk_button_clicked(GTK_BUTTON(spd_19200));
#endif
	    break;

	case DEFAULT:
#ifdef GTK_HAVE_FEATURES_1_1_4
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(spd_9600), TRUE);
#else
	    if (!GTK_TOGGLE_BUTTON(spd_9600)->active)
                gtk_button_clicked(GTK_BUTTON(spd_9600));
#endif
	    break;
    }
	
    debugToggle = gtk_object_get_data(GTK_OBJECT(cfgDlg), "debugToggle");
#ifdef GTK_HAVE_FEATURES_1_1_4
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(debugToggle), doDebug);
#else
    if (!GTK_TOGGLE_BUTTON(debugToggle)->active)
        gtk_button_clicked(GTK_BUTTON(debugToggle));
#endif
}

int
casio_qv_configure() {
    static GtkWidget *cfgDlg = NULL;
    
    cfgDlg = create_casioConfigDlg();
    setDlgState(cfgDlg);
#ifdef GTK_HAVE_FEATURES_1_1_4
    gtk_window_set_modal(GTK_WINDOW(cfgDlg), TRUE);
#endif
    gtk_widget_show(cfgDlg);
    return(1);
}

int
casio_qv_take_picture() {

    if (casio_qv_open_camera() == SDC_FAIL) {
	return(GPHOTO_FAIL);
    }
    
    if (casio_qv_record(cameraPort) == GPHOTO_FAIL) {
	return(GPHOTO_FAIL);
    }

    return(casio_qv_number_of_pictures());
}

struct Image *
casio_qv_get_preview() {
    struct Image *cameraImage;
    int picture_number;
    
    if ((picture_number = casio_qv_take_picture()) < 0) {
	return(NULL);
    }

    if ((cameraImage = casio_qv_get_picture(picture_number, 0)) == NULL) {
	return(NULL);
    }
    
    if (casio_qv_delete_picture(picture_number) == GPHOTO_FAIL) {
	return(NULL);
    }

    return(cameraImage);
}

/* Declare the camera function pointers */

struct _Camera casio_qv = {casio_qv_initialize,
			   casio_qv_get_picture,
			   casio_qv_get_preview,
			   casio_qv_delete_picture,
			   casio_qv_take_picture,
			   casio_qv_number_of_pictures,
			   casio_qv_configure,
			   casio_qv_summary,
			   casio_qv_description,
			   NULL};
