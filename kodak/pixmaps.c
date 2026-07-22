/*
 *	File: pixmaps.c
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
 *	Library to handle 8 bit greyscale and 24 bits color pictures.
 *
 *	$Id: pixmaps.c,v 1.3 2000/10/28 10:48:53 ole Exp $
 */

#include "dc20.h"
#include "pixmaps.h"
#include "main.h"

#define PGM_EXT		"pgm"
#define PPM_EXT		"ppm"

#ifdef USE_JPEG
#define JPEG_INTERNAL_OPTIONS
#include <jpeglib.h>
#define JPEG_EXT	"jpg"
#endif /* USE_JPEG */

#ifdef USE_TIFF
#define TIFF_EXT	"tif"
#endif /* USE_TIFF */

#ifdef USE_PNG
#include <png.h>
#define PNG_EXT		"png"
#endif /* USE_PNG */

#define RED		0.30
#define GREEN	0.59
#define BLUE	0.11

#ifdef USE_JPEG
#define RED_OFFSET		(RGB_RED)
#define GREEN_OFFSET	(RGB_GREEN)
#define BLUE_OFFSET		(RGB_BLUE)
#else
#define RED_OFFSET		0
#define GREEN_OFFSET	1
#define BLUE_OFFSET		2
#endif /* USE_JPEG */

#if RED_OFFSET == 0 && GREEN_OFFSET == 1 && BLUE_OFFSET == 2
#define CANONICAL_WAY
#endif

#define GET_COMP(pp, x, y, c)	(pp->planes[((x) + (y)*pp->width)*pp->components + (c)])

#define GET_R(pp, x, y)	(GET_COMP(pp, x, y, RED_OFFSET))
#define GET_G(pp, x, y)	(GET_COMP(pp, x, y, GREEN_OFFSET))
#define GET_B(pp, x, y)	(GET_COMP(pp, x, y, BLUE_OFFSET))

struct pixmap *alloc_pixmap(int x, int y, int d)
{
	struct pixmap	*result = NULL;

	if (d == 1 || d == 3) {
		if (x > 0) {
			if (y > 0) {
				if ((result = malloc(sizeof(struct pixmap))) != NULL) {
					result->width = x;
					result->height = y;
					result->components = d;
					if (!(result->planes = malloc(x*y*d))) {
						if (!quiet) fprintf(stderr, "%s: alloc_pixmap: error: not enough memory for bitplanes\n", __progname);
						free(result);
						result = NULL;
					}
				} else
					if (!quiet) fprintf(stderr, "%s: alloc_pixmap: error: not enough memory for pixmap\n", __progname);
			} else
				if (!quiet) fprintf(stderr, "%s: alloc_pixmap: error: y is out of range\n", __progname);
		} else
			if (!quiet) fprintf(stderr, "%s: alloc_pixmap: error: x is out of range\n", __progname);
	} else
		if (!quiet) fprintf(stderr, "%s: alloc_pixmap: error: cannot handle %d components\n", __progname, d);

	return result;
}

void free_pixmap(struct pixmap *p)
{
	if (p) {
		free(p->planes);
		free(p);
	}
}

int save_pixmap_pxm(struct pixmap *p, FILE *fp)
{
	int	 result = 0;
#ifndef CANONICAL_WAY
	int	 row,
		 column,
		 component;
#endif /* CANONICAL_WAY */

	if (p) {
		fprintf(fp, "P%c\n%d %d\n255\n", (p->components == 1) ? '5' : '6', p->width, p->height);
#ifdef CANONICAL_WAY
		fwrite(p->planes, p->height*p->width*p->components, 1, fp);
#else
		for (row = 0; row < p->height; row++) {
			for (column = 0; column < p->width; column++) {
				for (component = 0; component < p->components; component++) {
					putc(GET_COMP(p, column, row, component), fp);
				}
			}
		}
#endif /* CANONICAL_WAY */
	}

	return result;
}

int set_pixel(struct pixmap *p, int x, int y, unsigned char v)
{
	int	 result = 0,
		 component;

	if (p) {
		if (x >= 0 && x < p->width) {
			if (y >= 0 && y < p->height) {
				for (component = 0; component < p->components; component++)
					GET_COMP(p, x, y, component) = v;
			} else {
				if (!quiet) fprintf(stderr, "%s: set_pixel: error: y out of range\n", __progname);
				result = -1;
			}
		} else {
			if (!quiet) fprintf(stderr, "%s: set_pixel: error: x out of range\n", __progname);
			result = -1;
		}
	}

	return result;
}

int set_pixel_rgb(struct pixmap *p, int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	int	 result = 0;

	if (p) {
		if (x >= 0 && x < p->width) {
			if (y >= 0 && y < p->height) {
				if (p->components == 1) {
					GET_R(p, x, y) = RED * r + GREEN * g + BLUE * b;
				} else {
					GET_R(p, x, y) = r;
					GET_G(p, x, y) = g;
					GET_B(p, x, y) = b;
				}
			} else {
				if (!quiet) fprintf(stderr, "%s: set_pixel_rgb: error: y out of range\n", __progname);
				result = -1;
			}
		} else {
			if (!quiet) fprintf(stderr, "%s: set_pixel_rgb: error: x out of range\n", __progname);
			result = -1;
		}
	}

	return result;
}

int zoom_x(struct pixmap *source, struct pixmap *dest)
{
	int				 result = 0,
					 dest_col,
					 row,
					 component,
					 src_index;
	float			 ratio,
					 src_ptr,
					 delta;
	unsigned char	 src_component;

	if (source && dest) {
		/*
		 *	We could think of resizing a pixmap and changing the number of
		 *	components at the same time. Maybe this will be implemented later.
		 */
		if (source->height == dest->height && source->components == dest->components) {
			if (source->width < dest->width) {
				ratio = ((float) source->width / (float) dest->width);

				for (src_ptr = 0, dest_col = 0; dest_col < dest->width; src_ptr += ratio, dest_col++) {
					/*
					 *	dest[dest_col] = source[(int)src_ptr] +
					 *	  (source[((int)src_ptr) + 1] - source[(int)src_ptr])
					 *	  * (src_ptr - (int)src_ptr);
					 */
					src_index = (int)src_ptr;
					delta = src_ptr - src_index;

					for (row = 0; row < source->height; row++) {
						for (component = 0; component < source->components; component++) {
							src_component = GET_COMP(source, src_index, row, component);

							GET_COMP(dest, dest_col, row, component) =
								src_component +
								( GET_COMP(source, src_index + 1, row,
											component) -
								  src_component
								) * delta;
						}
					}
				}
			} else {
				if (!quiet) fprintf(stderr, "%s: zoom_x: error: can only zoom out\n", __progname);
				result = -1;
			}
		} else {
			if (!quiet) fprintf(stderr, "%s: zoom_x: error: incompatible pixmaps\n", __progname);
			result = -1;
		}
	}

	return result;
}

int zoom_y(struct pixmap *source, struct pixmap *dest)
{
	int				 result = 0,
					 dest_row,
					 column,
					 component,
					 src_index;
	float			 ratio,
					 src_ptr,
					 delta;
	unsigned char	 src_component;

	if (source && dest) {
		/*
		 *	We could think of resizing a pixmap and changing the number of
		 *	components at the same time. Maybe this will be implemented later.
		 */
		if (source->width == dest->width && source->components == dest->components) {
			if (source->height < dest->height) {
				ratio = ((float) source->height / (float) dest->height);

				for (src_ptr = 0, dest_row = 0; dest_row < dest->height; src_ptr += ratio, dest_row++) {
					/*
					 *	dest[dest_row] = source[(int)src_ptr] +
					 *	  (source[((int)src_ptr) + 1] - source[(int)src_ptr])
					 *	  * (src_ptr - (int)src_ptr);
					 */
					src_index = (int)src_ptr;
					delta = src_ptr - src_index;

					for (column = 0; column < source->width; column++) {
						for (component = 0; component < source->components; component++) {
							src_component = GET_COMP(source, column, src_index, component);

							GET_COMP(dest, column, dest_row, component) =
								src_component +
								( GET_COMP(source, column, src_index + 1,
											component) -
								  src_component
								) * delta;
						}
					}
				}
			} else {
				if (!quiet) fprintf(stderr, "%s: zoom_y: error: can only zoom out\n", __progname);
				result = -1;
			}
		} else {
			if (!quiet) fprintf(stderr, "%s: zoom_y: error: incompatible pixmaps\n", __progname);
			result = -1;
		}
	}

	return result;
}

#ifdef USE_JPEG
int save_pixmap_jpeg(struct pixmap *p, int q, FILE *fp)
{
	struct jpeg_compress_struct	 cinfo;
	struct jpeg_error_mgr		 jerr;
	JSAMPROW					 row_pointer[1];
	int							 result = 0,
								 row_stride;

	if (p) {
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);

		jpeg_stdio_dest(&cinfo, fp);
	
		cinfo.image_width = p->width;
		cinfo.image_height = p->height;
		cinfo.input_components = p->components;
		cinfo.in_color_space = (p->components == 1) ? JCS_GRAYSCALE : JCS_RGB;
	
		jpeg_set_defaults(&cinfo);
	
		jpeg_set_quality(&cinfo, q, TRUE);
	
		jpeg_start_compress(&cinfo, TRUE);
	
		row_stride = p->width * p->components;
	
		while (cinfo.next_scanline < cinfo.image_height) {
			row_pointer[0] = & p->planes[cinfo.next_scanline * row_stride];
			(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
	
		jpeg_finish_compress(&cinfo);
	
		jpeg_destroy_compress(&cinfo);
	}

	return result;
}
#endif /* USE_JPEG */

struct pixmap *rotate_right(struct pixmap *p)
{
	struct pixmap	*result = NULL;
	int				 x,
					 y,
					 c;

	if ((result = alloc_pixmap(p->height, p->width, p->components)) != NULL) {
		for (x = 0; x < p->width; x++) {
			for (y = 0; y < p->height; y++) {
				for (c = 0; c < p->components; c++) {
					GET_COMP(result, p->height - y - 1, x, c) = GET_COMP(p, x, y, c);
				}
			}
		}
	}

	return result;
}

struct pixmap *rotate_left(struct pixmap *p)
{
	struct pixmap	*result = NULL;
	int				 x,
					 y,
					 c;

	if ((result = alloc_pixmap(p->height, p->width, p->components)) != NULL) {
		for (x = 0; x < p->width; x++) {
			for (y = 0; y < p->height; y++) {
				for (c = 0; c < p->components; c++) {
					GET_COMP(result, y, p->width - x - 1, c) = GET_COMP(p, x, y, c);
				}
			}
		}
	}

	return result;
}

#ifdef USE_TIFF
int save_pixmap_tiff(struct pixmap *p, int c, int q, int pr, char *name)
{
	TIFF	*out;
	int		 result = 0,
			 row_stride,
			 y;

	if (p) {
		if ((out = TIFFOpen(name, "w")) != NULL) {
			TIFFSetField(out, TIFFTAG_IMAGEWIDTH, p->width);
			TIFFSetField(out, TIFFTAG_IMAGELENGTH, p->height);
			TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
			TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, p->components);
			TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
			TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(out, TIFFTAG_PHOTOMETRIC, (p->components == 1) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
			TIFFSetField(out, TIFFTAG_COMPRESSION, c);
			switch (c) {
				case COMPRESSION_JPEG:
					TIFFSetField(out, TIFFTAG_JPEGQUALITY, q);
					TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RAW);
					break;
				case COMPRESSION_LZW:
				case COMPRESSION_DEFLATE:
					if (pr != 0)
						TIFFSetField(out, TIFFTAG_PREDICTOR, pr);
				default:
					break;
			}
			row_stride = p->width * p->components;
			TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, -1));
			for (y = 0; y < p->height; y++) {
				if (TIFFWriteScanline(out, &GET_COMP(p, 0, y, 0), y, 0) < 0) {
					if (!quiet) fprintf(stderr, "%s: save_pixmap_tiff: error: TIFFWriteScanline\n", __progname);
					result = -1;
					break;
				}
			}
			TIFFClose(out);
		} else
			result = -1;
	}

	return result;
}
#endif /* USE_TIFF */

#ifdef USE_PNG
int save_pixmap_png(struct pixmap *p, FILE *fp)
{
	int			 result = 0,
				 y;
	png_structp	 png_ptr;
	png_infop	 info_ptr;

	if (p) {
		if ((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) != NULL) {
			if ((info_ptr = png_create_info_struct(png_ptr)) != NULL) {
				if (setjmp(png_ptr->jmpbuf) == 0) {
					png_init_io(png_ptr, fp);
					png_set_IHDR(png_ptr, info_ptr, p->width, p->height, 8, (p->components == 1) ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
					png_write_info(png_ptr, info_ptr);
					for (y = 0; y < p->height; y++) {
						png_write_row(png_ptr, (png_bytep)&GET_COMP(p, 0, y, 0));
					}
					png_write_end(png_ptr, info_ptr);
					png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
				} else
					result = -1;
			} else
				result = -1;

			png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		} else
			result = -1;
	}

	return result;
}
#endif /* USE_PNG */

int save_pixmap(struct pixmap *p, char *name, int orientation, int format)
{
	struct pixmap	*to_be_saved = p,
					*p2 = NULL;
	char			 fname[1024];
	FILE			*ofp = NULL;
	int				 retval;

	/*
	 *	Rotate image
	 */

	switch (orientation) {
		case ROT_LEFT:
			p2 = rotate_left(p);
			to_be_saved = p2;
			break;
		case ROT_HEADDOWN:
			p2 = rotate_right(p);
			to_be_saved = rotate_right(p2);
			free_pixmap(p2);
			p2 = to_be_saved;
			break;
		case ROT_RIGHT:
			p2 = rotate_right(p);
			to_be_saved = p2;
		case ROT_STRAIGHT:
		default:
			break;
	}

	/*
	 *	Build the image name
	 */

	strcpy(fname, name);
	strcat(fname, ".");
	switch (format & SAVE_FORMATS) {
#ifdef USE_JPEG
		case SAVE_JPEG:
			strcat(fname, JPEG_EXT);
			break;
#endif /* USE_JPEG */
#ifdef USE_TIFF
		case SAVE_TIFF:
			strcat(fname, TIFF_EXT);
			break;
#endif /* USE_TIFF */
#ifdef USE_PNG
		case SAVE_PNG:
			strcat(fname, PNG_EXT);
			break;
#endif /* USE_PNG */
		default:
			strcat(fname, (to_be_saved->components == 3) ? PPM_EXT : PGM_EXT );
			break;
	}

	/*
	 *	Open the output file, if necessary
	 */

	switch (format & SAVE_FORMATS) {
#ifdef USE_JPEG
		case SAVE_JPEG:
#endif /* USE_JPEG */
#ifdef USE_PNG
		case SAVE_PNG:
#endif /* USE_PNG */
		default:
			if ((ofp = fopen(fname, "wb")) == NULL) {
				if (!quiet) {
					perror("fopen");
					fprintf(stderr, "%s: save_pixmap: cannot open %s for output\n", __progname, fname);
				}
				if (p2) free_pixmap(p2);
				return -1;
			}
#ifdef USE_TIFF
		case SAVE_TIFF:
#endif /* USE_TIFF */
			break;
	}

	/*
	 *	Save the image
	 */

	switch (format & SAVE_FORMATS) {
#ifdef USE_JPEG
		case SAVE_JPEG:
			retval = save_pixmap_jpeg(to_be_saved, quality, ofp);
			break;
#endif /* USE_JPEG */
#ifdef USE_TIFF
		case SAVE_TIFF:
			retval = save_pixmap_tiff(to_be_saved, tiff_compression, quality, tiff_predictor, fname);
			break;
#endif /* USE_TIFF */
#ifdef USE_PNG
		case SAVE_PNG:
			retval = save_pixmap_png(to_be_saved, ofp);
			break;
#endif /* USE_PNG */
		default:
			retval = save_pixmap_pxm(to_be_saved, ofp);
			break;
	}

	if (ofp) fclose(ofp);

	if (p2) free_pixmap(p2);

	return retval;
}
