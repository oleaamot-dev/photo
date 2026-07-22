/*
 *	File: comet_to_ppm.c
 *
 *	Copyright (C) 1998 Ugo Paternostro <paterno@dsi.unifi.it>
 *
 *	This file is part of the dc20ctrl package. The complete package can be
 *	downloaded from:
 *	    http://aguirre.dsi.unifi.it/~paterno/binaries/dc20ctrl.tar.gz
 *
 *	This package is derived from the dc20 package, built by Karl Hakimian
 *	<hakimian@aha.com> that you can find it at ftp.eecs.wsu.edu in the
 *	/pub/hakimian directory. The complete URL is:
 *	    ftp://ftp.eecs.wsu.edu/pub/hakimian/dc20.tar.gz
 *
 *	This package also includes a sligthly modified version of the Comet to ppm
 *	conversion routine written by YOSHIDA Hideki <hideki@yk.rim.or.jp>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published 
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Converts the raw CCD data to 24 bits image.
 *
 *	$Id: comet_to_ppm.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

/*
 *
 * Converts CMT file of Chinon ES-1000 or IMG file of LXDC to PPM file.
 *
 *	written by YOSHIDA Hideki <hideki@yk.rim.or.jp>
 *	In public domain; you can do whatever you want to this program
 *	as long as you admit that the original code is written by me.
 *
 * $hideki-Id: cmttoppm.c,v 1.10 1996/09/28 03:01:57 hideki Exp hideki $
 *
 */

#include <math.h>

#include "dc20.h"
#include "comet_to_ppm.h"

#ifndef STAND_ALONE
#define STAND_ALONE	0
#include "main.h"
#include "pixmaps.h"
#endif

#define BIDIM_ARRAY(name, x, y, width)	(name[((x) + ((y) * (width)))])

/*
 *	These definitons depend on the resolution of the image
 */

#define MY_LOW_RIGHT_MARGIN 6

/*
 *	These definitions are constant with resolution
 */

#define MY_LEFT_MARGIN 2

#define NET_COLUMNS (columns - MY_LEFT_MARGIN - right_margin)
#define NET_LINES   (HEIGHT - TOP_MARGIN - BOTTOM_MARGIN)
#define NET_PIXELS  (NET_COLUMNS * NET_LINES)

#if STAND_ALONE == 1
#define MAGIC "COMET"
#define CMT_HEADER_SIZE 128
#define IMG_THUMBNAIL_SIZE 5120
#endif

#define SCALE 64
#define SMAX (256 * SCALE - 1)
#define HORIZONTAL_INTERPOLATIONS 3
#define HISTOGRAM_STEPS 4096

#define RFACTOR 0.64
#define GFACTOR 0.58
#define BFACTOR 1.00
#define RINTENSITY 0.476
#define GINTENSITY 0.299
#define BINTENSITY 0.175

#define SATURATION 1.0
// #define SATURATION 1.5
#define NORM_PERCENTAGE 3
#define GAMMA 0.35
// #define GAMMA 0.5

static int		 columns = HIGH_WIDTH,
				 right_margin = HIGH_RIGHT_MARGIN,
				 camera_header_size = HIGH_CAMERA_HEADER;
int				 low_i = -1,
				 high_i = -1,
				 norm_percentage = NORM_PERCENTAGE;
float			 gamma_value = GAMMA,
				 saturation = SATURATION,
				 rfactor = RFACTOR,
				 gfactor = GFACTOR,
				 bfactor = BFACTOR;

#if STAND_ALONE == 1

int	quiet = FALSE;

static void read_cmt_file(int infd, unsigned char ccd[])
{ 
  int cmt_format_p;
  char magic[sizeof(MAGIC)];
  read(infd, magic, sizeof(magic));
  cmt_format_p = !strncmp(magic, MAGIC, sizeof(MAGIC));
  if (!cmt_format_p && !quiet) {  
    fprintf(stderr, "%s: read_cmt_file: not in CMT format; IMG format assumed\n", __progname);
  }
  lseek(infd,
    cmt_format_p ? CMT_HEADER_SIZE +    camera_header_size
                 : IMG_THUMBNAIL_SIZE + camera_header_size,
    SEEK_SET);
  read(infd, ccd, columns * HEIGHT);
}
#endif

static void set_initial_interpolation(const unsigned char ccd[],
			       short horizontal_interpolation[])
{
  int column, line;
  for (line = 0; line < HEIGHT; line++) {
	BIDIM_ARRAY(horizontal_interpolation, MY_LEFT_MARGIN, line, columns) =
//    horizontal_interpolation[line][MY_LEFT_MARGIN] =
	  BIDIM_ARRAY(ccd, MY_LEFT_MARGIN + 1, line, columns) * SCALE;
//      ccd[line][MY_LEFT_MARGIN + 1] * SCALE;
	BIDIM_ARRAY(horizontal_interpolation, columns - right_margin - 1, line, columns) =
//    horizontal_interpolation[line][columns - right_margin - 1] =
	  BIDIM_ARRAY(ccd, columns - right_margin - 2, line, columns) * SCALE;
//      ccd[line][columns - right_margin - 2] * SCALE;
    for (column = MY_LEFT_MARGIN + 1; column < columns - right_margin - 1;
	 column++) {
	  BIDIM_ARRAY(horizontal_interpolation, column, line, columns) =
//      horizontal_interpolation[line][column] =
	    ( BIDIM_ARRAY(ccd, column - 1, line, columns) +
		  BIDIM_ARRAY(ccd, column + 1, line, columns) ) *
		( SCALE / 2 );
// (ccd[line][column - 1] + ccd[line][column + 1]) * (SCALE / 2);
    }
  }
}

static void interpolate_horizontally(const unsigned char ccd[],
			      short horizontal_interpolation[])
{
  int column, line, i, initial_column;
  for (line = TOP_MARGIN - 1; line < HEIGHT - BOTTOM_MARGIN + 1; line++) {
    for (i = 0; i < HORIZONTAL_INTERPOLATIONS; i++) {
      for (initial_column = MY_LEFT_MARGIN + 1; initial_column <= MY_LEFT_MARGIN + 2;
	   initial_column++) {
	for (column = initial_column; column < columns - right_margin - 1;
	     column += 2) {
	  BIDIM_ARRAY(horizontal_interpolation, column, line, columns) =
//	  horizontal_interpolation[line][column] =
	    ((float)
	     BIDIM_ARRAY(ccd, column - 1, line, columns) /
//		 ccd[line][column - 1] /
	     BIDIM_ARRAY(horizontal_interpolation, column - 1, line, columns) +
//		 horizontal_interpolation[line][column - 1] +
	     (float)
	     BIDIM_ARRAY(ccd, column + 1, line, columns) /
//		 ccd[line][column + 1] /
	     BIDIM_ARRAY(horizontal_interpolation, column +1, line, columns)) *
//		 horizontal_interpolation[line][column + 1]) *
	     BIDIM_ARRAY(ccd, column, line, columns) *
//	     ccd[line][column] *
	     (SCALE * SCALE / 2) + 0.5;
	}
      }
    }
  }
}

static void interpolate_vertically(const unsigned char ccd[],
		       const short horizontal_interpolation[],
		       short red[], short green[],
		       short blue[])
{
  int column, line;
  for (line = TOP_MARGIN; line < HEIGHT - BOTTOM_MARGIN; line++) {
    for (column = MY_LEFT_MARGIN; column < columns - right_margin; column++) {
      int r2gb, g2b, rg2, rgb2, r, g, b;
      int this_ccd =
	     BIDIM_ARRAY(ccd, column, line, columns) * SCALE;
//         ccd[line][column] * SCALE;
      int up_ccd   =
	     BIDIM_ARRAY(ccd, column, line - 1, columns) * SCALE;
//		  ccd[line - 1][column] * SCALE;
      int down_ccd =
	     BIDIM_ARRAY(ccd, column, line + 1, columns) * SCALE;
//        ccd[line + 1][column] * SCALE;
      int this_horizontal_interpolation =
		 BIDIM_ARRAY(horizontal_interpolation, column, line, columns);
//	horizontal_interpolation[line][column];
      int this_intensity = this_ccd + this_horizontal_interpolation;
      int up_intensity = 
	    BIDIM_ARRAY(horizontal_interpolation, column, line - 1, columns) +
//	    horizontal_interpolation[line - 1][column] +
	    up_ccd;
      int down_intensity =
	    BIDIM_ARRAY(horizontal_interpolation, column, line + 1, columns) +
//        horizontal_interpolation[line+1][column] +
        down_ccd;
      int this_vertical_interpolation;


/* PSF */
if ( down_ccd == 0 ) printf("down_ccd==0 at %d,%d\n",line,column);
if ( up_ccd == 0 ) printf("up_ccd==0 at %d,%d\n",line,column);
if ( down_intensity == 0 ) {
printf("Found down_intensity==0 at %d,%d down_ccd=%d\n",line,column,down_ccd);
down_intensity=1;
}
if ( up_intensity == 0 ) {
printf("Found up_intensity==0 at %d,%d up_ccd=%d\n",line,column,up_ccd);
up_intensity=1;
}

      if (line == TOP_MARGIN ) {
	this_vertical_interpolation =
	  (float)down_ccd / down_intensity * this_intensity + 0.5;
      } else if (line == HEIGHT - BOTTOM_MARGIN - 1 ) {
	this_vertical_interpolation =
	  (float)up_ccd / up_intensity * this_intensity + 0.5;
      } else {
	this_vertical_interpolation =
	  ((float)up_ccd / up_intensity + (float)down_ccd / down_intensity) *

	    this_intensity / 2.0 + 0.5;
      }
      if (line & 1) {
	if (column & 1) {
	  r2gb = this_ccd;
	  g2b = this_horizontal_interpolation;
	  rg2 = this_vertical_interpolation;
	  r = (2 * (r2gb - g2b) + rg2) / 5;
	  g = (rg2 - r) / 2;
	  b = g2b - 2 * g;
	} else {
	  g2b = this_ccd;
	  r2gb = this_horizontal_interpolation;
	  rgb2 = this_vertical_interpolation;
	  r = (3 * r2gb - g2b - rgb2) / 5;
	  g = 2 * r - r2gb + g2b;
	  b = g2b - 2 * g;
	}
      } else {
	if (column & 1) {
	  rg2 = this_ccd;
	  rgb2 = this_horizontal_interpolation;
	  r2gb = this_vertical_interpolation;
	  b = (3 * rgb2 - r2gb - rg2) / 5;
	  g = (rgb2 - r2gb + rg2 - b) / 2;
	  r = rg2 - 2 * g;
	} else {
	  rgb2 = this_ccd;
	  rg2 = this_horizontal_interpolation;
	  g2b = this_vertical_interpolation;
	  b = (g2b - 2 * (rg2 - rgb2)) / 5;
	  g = (g2b - b) / 2;
	  r = rg2 - 2 * g;
	}
      }
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      BIDIM_ARRAY(red, column, line, columns) = r;
//	  red  [line][column] = r;
      BIDIM_ARRAY(green, column, line, columns) = g;
//      green[line][column] = g;
      BIDIM_ARRAY(blue, column, line, columns) = b;
//      blue [line][column] = b;

    }
  }
}

static void adjust_color_and_saturation(short red[],
				 short green[],
				 short blue[])
{
  int line, column;
  int r_min = SMAX, g_min = SMAX, b_min = SMAX;
  int r_max =    0, g_max =    0, b_max =    0;
  int r_sum =    0, g_sum =    0, b_sum =    0;
  float sqr_saturation = sqrt(saturation);
  for (line = TOP_MARGIN; line < HEIGHT - BOTTOM_MARGIN; line++) {
    for (column = MY_LEFT_MARGIN; column < columns - right_margin; column++) {
      float r =
        BIDIM_ARRAY(red, column, line, columns) *
//	    red  [line][column] *
	    rfactor;
      float g =
        BIDIM_ARRAY(green, column, line, columns) *
//		green[line][column] *
		gfactor;
      float b =
        BIDIM_ARRAY(blue, column, line, columns) *
//		blue [line][column] *
		bfactor;
      if (saturation != 1.0) {
	float *min, *mid, *max, new_intensity;
	float intensity = r * RINTENSITY + g * GINTENSITY + b * BINTENSITY;
	if (r > g) {
	  if (r > b) {
	    max = &r;
	    if (g > b) {
	      min = &b;
	      mid = &g;
	    } else {
	      min = &g;
	      mid = &b;
	    }
	  } else {
	    min = &g;
	    mid = &r;
	    max = &b;
	  }
	} else {
	  if (g > b) {
	    max = &g;
	    if (r > b) {
	      min = &b;
	      mid = &r;
	    } else {
	      min = &r;
	      mid = &b;
	    }
	  } else {
	    min = &r;
	    mid = &g;
	    max = &b;
	  }
	}
	*mid = *min + sqr_saturation * (*mid - *min);
	*max = *min + saturation * (*max - *min);
	new_intensity = r * RINTENSITY + g * GINTENSITY + b * BINTENSITY;
	r *= intensity / new_intensity;
	g *= intensity / new_intensity;
	b *= intensity / new_intensity;
      }
      r += 0.5;
      g += 0.5;
      b += 0.5;
      if (r_min > r) r_min = r;
      if (g_min > g) g_min = g;
      if (b_min > b) b_min = b;
      if (r_max < r) r_max = r;
      if (g_max < g) g_max = g;
      if (b_max < b) b_max = b;
      r_sum += r;
      g_sum += g;
      b_sum += b;
      BIDIM_ARRAY(red, column, line, columns) = r;
//      red  [line][column] = r;
      BIDIM_ARRAY(green, column, line, columns) = g;
//      green[line][column] = g;
      BIDIM_ARRAY(blue, column, line, columns) = b;
//      blue [line][column] = b;
    }
  }
}

static int min3(int x, int y, int z)
{
  return (x < y ? (x < z ? x : z) : (y < z ? y : z));
}

static int max3(int x, int y, int z)
{
  return (x > y ? (x > z ? x : z) : (y > z ? y : z));
}

static void determine_limits(const short red[],
		      const short green[],
		      const short blue[],
		      int *low_i_ptr, int *high_i_ptr)
{
  unsigned int histogram[HISTOGRAM_STEPS + 1];
  int column, line, i, s;
  int low_i = *low_i_ptr, high_i = *high_i_ptr;
  int max_i = 0;
  for (line = TOP_MARGIN; line < HEIGHT - BOTTOM_MARGIN; line++) {
    for (column = MY_LEFT_MARGIN; column < columns - right_margin; column++) {
      i = max3(
		BIDIM_ARRAY(red, column, line, columns),
		BIDIM_ARRAY(green, column, line, columns),
		BIDIM_ARRAY(blue, column, line, columns)
//		red[line][column], green[line][column], blue[line][column]
		);
      if (i > max_i) max_i = i;
    }
  }
  if (low_i == -1) {
    for (i = 0; i <= HISTOGRAM_STEPS; i++) histogram[i] = 0;
    for (line = TOP_MARGIN; line < HEIGHT - BOTTOM_MARGIN; line++) {
      for (column = MY_LEFT_MARGIN; column < columns - right_margin; column++) {
	i = min3(
	  BIDIM_ARRAY(red, column, line, columns),
	  BIDIM_ARRAY(green, column, line, columns),
	  BIDIM_ARRAY(blue, column, line, columns)
//	  red[line][column], green[line][column], blue[line][column]
	  );
	histogram[i * HISTOGRAM_STEPS / max_i]++;
      }
    }
    for (low_i = 0, s = 0;
	 low_i <= HISTOGRAM_STEPS && s < NET_PIXELS * norm_percentage / 100;
	 low_i++) {
      s += histogram[low_i];
    }
    low_i = (low_i * max_i + HISTOGRAM_STEPS / 2) / HISTOGRAM_STEPS;
    *low_i_ptr = low_i;
  }
  if (high_i == -1) {
    for (i = 0; i <= HISTOGRAM_STEPS; i++) histogram[i] = 0;
    for (line = TOP_MARGIN; line < HEIGHT - BOTTOM_MARGIN; line++) {
      for (column = MY_LEFT_MARGIN; column < columns - right_margin; column++) {
	i = max3(
	  BIDIM_ARRAY(red, column, line, columns),
	  BIDIM_ARRAY(green, column, line, columns),
	  BIDIM_ARRAY(blue, column, line, columns)
//	  red[line][column], green[line][column], blue [line][column]
	  );
	histogram[i * HISTOGRAM_STEPS / max_i]++;
      }
    }
    for (high_i = HISTOGRAM_STEPS, s = 0;
	 high_i >= 0 && s < NET_PIXELS * norm_percentage / 100; high_i--) {
      s += histogram[high_i];
    }
    high_i = (high_i * max_i + HISTOGRAM_STEPS / 2) / HISTOGRAM_STEPS;
    *high_i_ptr = high_i;
  }
  if (verbose) printf("%s: determine_limits: low_i = %d, high_i = %d\n", __progname, low_i, high_i);
}

static unsigned char *make_gamma_table(int range)
{
  int i;
  double factor = pow(256.0, 1.0 / gamma_value) / range;
  unsigned char *gamma_table;
  if ((gamma_table = malloc(range * sizeof(unsigned char))) == NULL) {
    if (!quiet) fprintf(stderr, "%s: make_gamma_table: can't allocate memory for gamma table\n", __progname);
    return NULL;
  }
  for (i = 0; i < range; i++) {
    int g = pow((double)i * factor, gamma_value) + 0.5;
#ifndef DC25
    if (verbose) fprintf(stderr, "%s: make_gamma_table: gamma[%4d] = %3d\n", __progname, i, g);
#endif
    if (g > 255) g = 255;
    gamma_table[i] = g;
  }
  return gamma_table;
}

static int lookup_gamma_table(int i, int low_i, int high_i,
		       const unsigned char gamma_table[])
{
  if (i <= low_i)  return   0;
  if (i >= high_i) return 255;
  return gamma_table[i - low_i];
}

static int output_rgb(const short red[],
		const short green[],
		const short blue[],
		int low_i, int high_i,
		struct pixmap *pp)
{
  int r_min = 255, g_min = 255, b_min = 255;
  int r_max =   0, g_max =   0, b_max =   0;
  int r_sum =   0, g_sum =   0, b_sum =   0;
  int column, line;
  unsigned char *gamma_table = make_gamma_table(high_i - low_i);

	if (gamma_table == NULL) {
		if (!quiet) fprintf(stderr, "%s: output_rgb: error: cannot make gamma table\n", __progname);
		return -1;
	}

  for (line = TOP_MARGIN; line < HEIGHT - BOTTOM_MARGIN; line++) {
    for (column = MY_LEFT_MARGIN; column < columns - right_margin; column++) {
      int r =
	lookup_gamma_table(BIDIM_ARRAY(red, column, line, columns), low_i, high_i, gamma_table);
      int g =
	lookup_gamma_table(BIDIM_ARRAY(green, column, line, columns), low_i, high_i, gamma_table);
      int b =
	lookup_gamma_table(BIDIM_ARRAY(blue, column, line, columns), low_i, high_i, gamma_table);
      if (r > 255) r = 255; else if (r < 0) r = 0;
      if (g > 255) g = 255; else if (g < 0) g = 0;
      if (b > 255) b = 255; else if (b < 0) b = 0;

      set_pixel_rgb(pp, column - MY_LEFT_MARGIN, line - TOP_MARGIN, r, g, b);

      if (r_min > r) r_min = r;
      if (g_min > g) g_min = g;
      if (b_min > b) b_min = b;
      if (r_max < r) r_max = r;
      if (g_max < g) g_max = g;
      if (b_max < b) b_max = b;
      r_sum += r;
      g_sum += g;
      b_sum += b;
    }
  }
  free(gamma_table);
	if (verbose) {
		fprintf(stderr, "%s: output_rgb: r: min = %d, max = %d, ave = %d\n", __progname, r_min, r_max, r_sum / NET_PIXELS);
		fprintf(stderr, "%s: output_rgb: g: min = %d, max = %d, ave = %d\n", __progname, g_min, g_max, g_sum / NET_PIXELS);
		fprintf(stderr, "%s: output_rgb: b: min = %d, max = %d, ave = %d\n", __progname, b_min, b_max, b_sum / NET_PIXELS);
	}

	return 0;
}

int comet_to_pixmap(unsigned char *pic, struct pixmap *pp)
{
	unsigned char	*ccd;
	short			*horizontal_interpolation,
					*red,
					*green,
					*blue;
	int				 retval = 0;

	if (pic == NULL) {
		if (!quiet) fprintf(stderr, "%s: cmttoppm: error: no input image\n", __progname);
		return -1;
	}

	if (pic[4] == 0x01) {
		/* Low resolution mode */
		columns = LOW_WIDTH;
		right_margin = MY_LOW_RIGHT_MARGIN;
		camera_header_size = LOW_CAMERA_HEADER;
	}

	ccd = pic + camera_header_size;

  if ((horizontal_interpolation = malloc(sizeof(short) * HEIGHT * columns)) == NULL) {
	if (!quiet) fprintf(stderr, "%s: cmttoppm: error: not enough memory for horizontal_interpolation\n", __progname);
	return -1;
  }

  if ((red = malloc(sizeof(short) * HEIGHT * columns)) == NULL) {
	if (!quiet) fprintf(stderr, "%s: cmttoppm: error: not enough memory for red\n", __progname);
	return -1;
  }

  if ((green = malloc(sizeof(short) * HEIGHT * columns)) == NULL) {
	if (!quiet) fprintf(stderr, "%s: cmttoppm: error: not enough memory for green\n", __progname);
	return -1;
  }

  if ((blue = malloc(sizeof(short) * HEIGHT * columns)) == NULL) {
	if (!quiet) fprintf(stderr, "%s: cmttoppm: error: not enough memory for blue\n", __progname);
	return -1;
  }


  /* Decode raw CCD data to RGB */
  set_initial_interpolation(ccd, horizontal_interpolation);
  interpolate_horizontally(ccd, horizontal_interpolation);
  interpolate_vertically(ccd, horizontal_interpolation, red, green, blue);

  adjust_color_and_saturation(red, green, blue);

  /* Determine lower and upper limit using histogram */
  if (low_i == -1 || high_i == -1) {
    determine_limits(red, green, blue, &low_i, &high_i);
  }

  /* Output pixmap structure */
  retval = output_rgb(red, green, blue, low_i, high_i, pp);

  return retval;
}

