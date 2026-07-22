#ifndef _JPEG_H
#define _JPEG_H

/* for function prototypes */
#ifdef STDC_HEADERS
# define P__(x) x
#else
# define P__(x) ()
#endif

int	write_jpeg P__((u_char *, FILE *));
int	casio_write_file P__((u_char *, int,  FILE *));

#ifdef USEWORKFILE
int	write_jpeg_fine P__((char *, FILE *));
int	write_file_file P__((u_char *, int, int, FILE *));
#else
int	write_jpeg_fine P__((u_char *, FILE *));
#endif

#endif /* _JPEG_H */
