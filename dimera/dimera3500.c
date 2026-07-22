#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#include <sys/time.h>
#include <signal.h>
#include "../src/gphoto.h"
#include "../src/util.h"
#include <errno.h>
#include "mesalib.h"
#include "dimeratab.h" // Added by Chuck -- 12-May-2000

/* PNM headers */
static char     Dimera_thumbhdr[] =
"P5\n# Dimera 3500 Thumbnail\n64 48\n255\n";

#define VIEWFIND_SZ	12288
#define VIEW_TYPE	251
#define THIRTY		1666	/* 1/30 of a second */

static char     Dimera_viewhdr[] = 
"P5\n# Dimera 3500 Viewfinder\n128 96\n15\n";

static char     Dimera_finehdr[] =
"P6\n# Dimera 3500 Image\n640 480\n255\n";

static char     Dimera_stdhdr[] =
"P6\n# Dimera 3500 Image\n320 240\n255\n";

/*
 * Configure menu Widgets
 */

static GtkWidget *radiobutton_speed[8];

static struct speeds {
	char		*label;
	int		speed;
	u_int8_t	code;
} Speeds[] = {
	{ "9600", B9600, 0 },
#ifdef B14400
	{ "14400", B14400, 1 },
#endif
	{ "19200", B19200, 2 },
#ifdef B28800
	{ "28800", B28800, 3 },
#endif
#ifdef B38400
	{ "38400", B38400, 4 },
#endif
#ifdef B57600
	{ "57600", B57600, 5 },
#endif
#ifdef B76800
	{ "76800", B76800, 6 },
#endif
#ifdef B115200
	{ "115200", B115200, 7 },
#endif
};

#define ERROR( s ) { \
	fprintf( stderr, "%s: %s\n", __FUNCTION__, s ); \
	dialog_error( s ); }

static int	Dimera_speed = -1;
static int	Dimera_exposure = THIRTY;


static int
openrc( char *flag )
{
	char	*dir;
	char	filename[1024];
	int	oflag, i;

	dir = getenv( "HOME" );
	if ( !dir )
		dir = ".";
	
	strcpy( filename, dir );
	strcat( filename, "/.gphoto/dimerarc" );

	if ( strcmp( flag, "r" ) == 0 )
		oflag = O_RDONLY;
	else if ( strcmp( flag, "w" ) == 0 )
		oflag = O_WRONLY|O_TRUNC|O_CREAT;
	else if ( strcmp( flag, "rw" ) == 0 )
		oflag = O_RDWR;
	else
		oflag = O_RDONLY;
	i = open( filename, oflag, 0644 );

	return i;
}

static int
parsenum( char *s )
{
	int	i;

	while( *s == ' ' || *s == '\t' )
	{
		if ( *s == '\0' )
			return 0;
		else
			s++;
	}

	if ( *s != '=' )	/* must be an equals */
		return 0;

	return (atoi ( ++s ));
}

static int
getline( int fd, char *l, int n )
{
	int	i;
	char	c;

	for ( i = 0; i < n-1; i++)
	{
		if ( read( fd, l+i, 1 ) == 1 )
			if ( l[i] != '\n' )
				continue;
		l[i] = 0;
		return i;
	}

	l[i] = '\0';

		/* read the rest of the line */
	while ( read( fd, &c, 1 ) == 1 )
	{
		if ( c == '\n' )
			break;
	}
	return i;
}

static int
Dimera_Initialise(void) 
{
	char	line[1024], *lp;
	int	fd;

	if (( fd = openrc( "r" )) >= 0 )
	{
		while( getline( fd, line, sizeof( line ) ) > 0 )
		{
			for( lp = line; lp < (line + sizeof( line )); lp++ )
			{		/* skip leading white space */
				if ( *lp != ' ' && *lp != '\t' )
					break;
			}
			if ( *lp == '\0' )	/* ignore empty lines */
				continue;
			if ( *lp == '#' )
				continue;	/* ignore comment lines */
			if ( strncasecmp( lp, "speed", 5 ) == 0 )
			{
				Dimera_speed = parsenum( lp+5 );
				continue;
			}
			if ( strncasecmp( lp, "exposure", 8 ) == 0 )
			{
				Dimera_exposure = parsenum( lp+8 );
				continue;
			}

			/* unrecognized option */
		}
	}

	if ( mesa_port_open( serial_port ) == 0 )
	{
		ERROR("Couldn't open camera device");
		return 0;
	}

	if ( ! mesa_reset() )
	{
		ERROR("Camera Reset Failed");
		return 0;
	}

	switch ( mesa_modem_check() )
	{
	case -1:
		ERROR("No or Unknown Response");
		return 0;
	case 0:
		ERROR("Probably a modem");
		return 0;
	case 1:
		break;
	}

	if ( Dimera_speed < 0 )
		Dimera_speed = 115200;
	else
		if ( mesa_set_speed( Dimera_speed ) == 0 )
		{
			ERROR("Camera speed setting failed");
			return 0;
		}

	return 1;
}

static int
Dimera_Number_of_pics() 
{
	int		pictures;

	if ( (pictures = mesa_get_image_count() ) < 0 )
	{
			ERROR( "Bad or No response from camera" );
	}
	return pictures;
}

static int
Dimera_Get_Thumbnail( int picnum, struct Image *image )
{
	int32_t		r;

	if ( !(image->image = (unsigned char *) malloc( MESA_THUMB_SZ +
			sizeof( Dimera_thumbhdr ) - 1 )) )
	{
		ERROR( "Get Thumbnail, allocation failed" );
		image->image_size = 0;
		return 0;
	}

		 /* set image size */
	image->image_size = MESA_THUMB_SZ + sizeof( Dimera_thumbhdr ) - 1;

		/* set image header */
	memcpy( image->image, Dimera_thumbhdr, sizeof( Dimera_thumbhdr ) - 1 );

		/* set image type */
	strcpy( image->image_type, "pgm" );

	image->image_info_size = 0;

	if ( (r = mesa_read_thumbnail( picnum, image->image +
			sizeof( Dimera_thumbhdr ) - 1 )) < 1 )
	{
		ERROR( "Get Thumbnail, read of thumbnail failed" );
		free( image->image );
		image->image_size = 0;
		return 0;
	}
	return (0 != (r&0x1000000));
}

static int
Dimera_convert_raw( u_int8_t *rbuffer, int height, int width,
		struct Image *image )
{
	int		x, y;
	int		p1, p2, p3, p4;
	int		red, green, blue;
	u_int8_t	*b;

	if ( (image->image = (u_int8_t *)malloc( height*width*3 +
			sizeof( Dimera_finehdr ) - 1 )) == NULL )
	{
		ERROR( "Dimera_convert_raw: alloc failed" );
		return 0;
	}
	if( width == 640 )
	{
		strcpy( image->image, Dimera_finehdr );
	} else {
		strcpy( image->image, Dimera_stdhdr );
	}

	b = image->image + sizeof( Dimera_finehdr ) - 1;

	/*
	   Added by Chuck -- 12-May-2000
	   Convert the colors based on location, and my wacky
	   color table
	*/
	for (y=0;y<height;y++) for (x=0;x<width;x++) {
		p1 = ((y==0?y+1:y-1)*width) + (x==0?x+1:x-1);
		p2 = ((y==0?y+1:y-1)*width) + x;
		p3 = (y*width) + (x==0?x+1:x-1);
		p4 = (y*width) + x;
		/*
		   I'm not sure about the y+1 thing.  It works for my
		   camera, but it may not work for yours!
	   */
		switch ( (((y+1) & 1) << 1) + (x & 1) ) {
			case 0:	/* even row, even col, green */
				green = green_table[rbuffer[ p1 ]];
				red   = red_table[rbuffer[ p2 ]];
				blue  = blue_table[rbuffer[ p3 ]];
				green+= green_table[rbuffer[ p4 ]];
				break;
			case 1:	/* even row, odd col, blue */
				red   = red_table[rbuffer[ p1 ]];
				green = green_table[rbuffer[ p2 ]];
				green+= green_table[rbuffer[ p3 ]];
				blue  = blue_table[rbuffer[ p4 ]];
				break;
			case 2: /* odd row, even col, red */
				blue  = blue_table[rbuffer[ p1 ]];
				green = green_table[rbuffer[ p2 ]];
				green+= green_table[rbuffer[ p3 ]];
				red   = red_table[rbuffer[ p4 ]];
				break;
			case 3: /* odd row, odd col, green */
				green = green_table[rbuffer[ p1 ]];
				blue  = blue_table[rbuffer[ p2 ]];
				red   = red_table[rbuffer[ p3 ]];
				green+= green_table[rbuffer[ p4 ]];
				break;
		}
		*b++=(unsigned char)red;
		*b++=(unsigned char)(green/2);
		*b++=(unsigned char)blue;
	}
	
	
	image->image_size = height*width*3 + sizeof( Dimera_finehdr ) - 1;
	return (height*width*3);
}

static int
Dimera_Get_Full_Image( int picnum, struct Image *image )
{
	static struct mesa_image_arg	ia;
	int32_t				r;
	u_int8_t			*b, *rbuffer;
	int				height, width, hires, s, retry;

	image->image_size = 0;
	image->image_info_size = 0;
	if ( picnum != RAM_IMAGE_NUM )
	{
		update_status( "Loading Image" );
		if ( mesa_load_image( picnum ) < 1 )
		{
			ERROR("Image Load failed");
			return 0;
		}

		update_status( "Getting Image Info" );
		if ( (r = mesa_read_image_info( picnum, NULL )) < 0 )
		{
			ERROR("Can't get Image Info");
			return 0;
		}

		if ( r )
		{
			hires = FALSE;
			height = 240;
			width = 320;
		} else {
			hires = TRUE;
			height = 480;
			width = 640;
		}
	} else {
			/* load the snapped image */
		hires = TRUE;
		height = 480;
		width = 640;
	}
	update_status( "Downloading Image" );

	rbuffer = (u_int8_t *)malloc( height*width );
	if ( rbuffer == NULL )
	{
		return 0;
	}

	ia.start = 28;
	ia.send = 4;
	ia.skip = 0;
	ia.repeat = ( hires ? 160 : 80 );
	ia.row_cnt = 40;
	ia.inc1 = 1;
	ia.inc2 = 128;
	ia.inc3 = ia.inc4 = 0;

	update_progress( 0 );

	for ( ia.row = 4, b = rbuffer; ia.row < (height + 4) ;
			ia.row += ia.row_cnt, b += s )
	{
		update_status( "Downloading Image" );
		for ( retry = 10;; )
		{

			s = mesa_read_image( b, &ia );
			if( s > 0)
				break;

				/* check for check sum error */
			/*
			   Modified by Chuck -- 12-May-2000
			   Included "s == 0" to get it to work with my serial port
			*/
			if ( (s == -2 || s == 0) && --retry > 0)
			{
				update_status( "Retransmitting" );
				printf("Dimera_Get_Full_Image: retrans\n");
				continue;
			}
			printf("Dimera_Get_Full_Image: read error %d (retry %d)\n",s,retry);
				/* retry count exceeded, or other error */
			free( rbuffer );
			image->image_size = 0;
			return 0;
		}
		update_progress( 100 * ia.row /(height + 4) );
	}
	return Dimera_convert_raw( rbuffer, height, width, image );
}

static struct Image *
Dimera_Get_Picture(int picnum, int thumbnail)
{
	struct Image	*image = (struct Image *)malloc( sizeof( struct Image));
	unsigned char	b[3];

	if ( image == NULL )
	{
		ERROR( "Get Picture, allocation failed" );
	} else {

		if ( thumbnail )
		{
			Dimera_Get_Thumbnail( picnum, image );
		} else {
			Dimera_Get_Full_Image( picnum, image );
		}
		
	}
	if ( image->image_size )
		return image;
	free( image );
	return 0;
}

static int
Dimera_DeleteImage(int image_no)
{
	return 0;
}

static char *
Dimera_Summary(void)
{
	static char	Summary[1024];
	struct mesa_id	Id;
	int		pictures;

	pictures = mesa_get_image_count();
	(void) mesa_send_id( &Id );

	snprintf( Summary, sizeof( Summary ),
			"Dimera 3500 vers %s %d/%d %d:%d\n%d pictures\n",
			mesa_version(), Id.year, Id.week, Id.man,
			Id.ver, pictures );

	return Summary;
}

static char *
Dimera_Description(void)
{
	return
		"gPhoto Mustek VDC 3500/Relisys Dimera 3500\n"
		"Version 0.5 -- This software was created with\n"
		"the help of proprietary information belonging\n"
		"to StarDot Technologies."
		"Author:\n"
		"Brian Beattie http://www.beattie-home.net\n"
		"Contributors:\n"
		"Chuck Homic <chuck@vvisions.com>\n"
		"     Converting raw camera images to RGB\n"
		"Dan Fandrich <dan@coneharvesters.com>\n"
		"     Information on protocol, raw image format\n"
		;
}

static struct Image *
Dimera_Preview(void)
{
	unsigned char	b[7];
	unsigned char	buffer[VIEWFIND_SZ/2], *p;
	int		n, i;
	struct Image	*image;

	if ( (image = (struct Image *)malloc( sizeof( struct Image))) == NULL)
	{
		ERROR( "Get Preview, allocation failed" );
		return NULL;
	}

	if ( !(image->image = (unsigned char *) malloc( VIEWFIND_SZ +
			sizeof( Dimera_viewhdr ) - 1 )) )
	{
		ERROR( "Get Preview, allocation failed" );
		free( image );
		return NULL;
	}
	image->image_info_size = 0;

		/* set image size */
	image->image_size = VIEWFIND_SZ + sizeof( Dimera_viewhdr ) - 1;

		/* set image header */
	memcpy( image->image, Dimera_viewhdr, sizeof( Dimera_viewhdr ) - 1 );

		/* set image type */
	strcpy( image->image_type, "pgm" );


	if ( mesa_snap_view( buffer, TRUE, 0, 0, 0, Dimera_exposure,
			VIEW_TYPE)<1 )
	{
		ERROR( "Get Preview, mesa_snap_view failed" );
		free (image->image);
		free (image);
		return NULL;
	}

		/* copy the buffer, splitting the pixels up */
	for ( p = image->image + sizeof( Dimera_viewhdr ) - 1, i = 0;
			i < (VIEWFIND_SZ/2) ; i++ )
	{
		*p++ = buffer[i] >> 4;
		*p++ = buffer[i];
	}

	return image;
}

static int
Dimera_TakePicture()
{
	if ( mesa_snap_picture( Dimera_exposure*4 ) )
	{
		return ( RAM_IMAGE_NUM );
	}
	return -1;
}

static void
Done( GtkWidget *widget, GtkWidget *window )
{
	gtk_widget_destroy( window );
}

static void
Save( GtkWidget *widget, GtkWidget *window ) 
{
	int	fd;
	char	line[1024];

	if (( fd = openrc( "w" )) < 0 )
	{
		ERROR("Save Options failed");
		return;
	}

	snprintf( line, sizeof( line ), "# Dimera Options\n");
	write( fd, line, strlen( line ) );
	snprintf( line, sizeof( line ), "Speed = %d\n", Dimera_speed );
	write( fd, line, strlen( line ) );
	snprintf( line, sizeof( line ), "Exposure = %d\n", Dimera_exposure );
	write( fd, line, strlen( line ) );
}

static void
speed_changed( GtkButton *button, gpointer user_data )
{
	if (mesa_set_speed( (int) user_data ))
		Dimera_speed = (int) user_data;
}

static int
Dimera_Configure( void )
{
	GtkWidget	*window, *box, *save, *done;   
	GtkWidget	*frame;
	GSList		*radiobuttongroup_speed = NULL;
	int		i;

	window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	if ( !window )
	{
		return 0;
	}

	gtk_signal_connect( GTK_OBJECT( window ), "delete_event",
			GTK_SIGNAL_FUNC( gtk_widget_destroy ), window );

	box = gtk_vbox_new( FALSE, 0 );
	if ( !box )
	{
		gtk_widget_destroy( window );
		return 0;
	}
	gtk_container_add( GTK_CONTAINER( window ), box );
	gtk_widget_show( box );

	/* Create frame and radio buttons for speed */
	for ( i = 0; i < (sizeof( Speeds )/sizeof( struct speeds )); i++ )
	{
		radiobutton_speed[i] = gtk_radio_button_new_with_label(
				radiobuttongroup_speed, Speeds[i].label );
		radiobuttongroup_speed = gtk_radio_button_group(
				GTK_RADIO_BUTTON( radiobutton_speed[i]));
		gtk_widget_show(radiobutton_speed[i]);
		gtk_box_pack_start(GTK_BOX(box), radiobutton_speed[i], FALSE,
				FALSE, 0);
		gtk_signal_connect_after(GTK_OBJECT(radiobutton_speed[i]),
				"pressed", GTK_SIGNAL_FUNC(speed_changed),
				(gpointer) Speeds[i].speed);
		if ( atoi( Speeds[i].label ) == Dimera_speed )
			gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(radiobutton_speed[i]), TRUE );
	}

	save = gtk_button_new_with_label( "Save Options" );
	if ( !save )
	{
		gtk_widget_destroy( window );
		return 0;
	}
	gtk_signal_connect( GTK_OBJECT( save ), "clicked",
			GTK_SIGNAL_FUNC( Save ), window );
	gtk_widget_show( save );
	gtk_box_pack_start( GTK_BOX( box ),save,FALSE,FALSE,FALSE );

	done = gtk_button_new_with_label( "Done" );
	if ( !done )
	{
		gtk_widget_destroy( window );
		return 0;
	}
	gtk_signal_connect( GTK_OBJECT( done ), "clicked",
			GTK_SIGNAL_FUNC( Done ), window );
	gtk_widget_show( done );
	gtk_box_pack_start( GTK_BOX( box ), done, FALSE, FALSE, FALSE );

	gtk_widget_show( window );

	return 1;
}

struct _Camera dimera_3500 =
{
	Dimera_Initialise,
	Dimera_Get_Picture,
	Dimera_Preview,
	Dimera_DeleteImage,
	Dimera_TakePicture,
	Dimera_Number_of_pics,
	Dimera_Configure,
	Dimera_Summary,
	Dimera_Description,
	NULL
};
