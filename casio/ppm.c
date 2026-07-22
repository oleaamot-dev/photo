#include <config.h>
#include "../src/gphoto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define NORM(x) { if(x<0) x=0; else if (x>255) x=255;}


int
write_ppm(buf, outfp, PPM_WIDTH, PPM_HEIGHT, rateW, rateH, withheader, order)
     u_char	*buf;
     FILE	*outfp;
     int PPM_WIDTH;
     int PPM_HEIGHT;
     int rateW;
     int rateH;
     int withheader;
     int order;
{
  int x, y;
  int Sy;
  long cr, cb;
  long L;
  long r,g,b;
  u_char *Y;
  u_char *Cr;
  u_char *Cb;
  int i;
  i = 0;

  if(withheader)
    fprintf(outfp, "P6\n%d %d\n255\n", PPM_WIDTH, PPM_HEIGHT);

  Y = buf;

  Cb = Y + (PPM_HEIGHT * PPM_WIDTH);
  Cr = Cb + (PPM_HEIGHT / rateH) * (PPM_WIDTH / rateW);

  for( Sy = 0 ; Sy < PPM_HEIGHT; Sy++){
    if(order)
      y = PPM_HEIGHT - Sy -1;
    else
      y = Sy;
    for( x = 0 ; x < PPM_WIDTH ; x++){
      L = Y[y * PPM_WIDTH + x] *  100000; 
      cb = Cb[(y / rateH) * PPM_WIDTH /rateW + (x /rateW)];
      if(cb > 127) cb = cb - 256;
      cr = Cr[(y / rateH) * PPM_WIDTH /rateW + (x /rateW)];
      if(cr > 127) cr = cr - 256;

      r = L + 140200 * cr;
      g = L - 34414 * cb - 71414 * cr;
      b = L + 177200 * cb;
      
      r = r / 100000;
      g = g / 100000;
      b = b / 100000;

      NORM(r);
      NORM(g);
      NORM(b);
      if(order){
	if(fputc(b, outfp) == EOF) { perror("write_ppm"); return(-1);}
	i++;
	if(fputc(g, outfp) == EOF) { perror("write_ppm"); return(-1);}
	i++;
	if(fputc(r, outfp) == EOF) { perror("write_ppm"); return(-1);}
	i++;
      } else {
	if(fputc(r, outfp) == EOF) { perror("write_ppm"); return(-1);}
	i++;
	if(fputc(g, outfp) == EOF) { perror("write_ppm"); return(-1);}
	i++;
	if(fputc(b, outfp) == EOF) { perror("write_ppm"); return(-1);}
	i++;
      }
    }
  }
  return(i);
}

int
record_ppm(u_char buf[], int PPM_WIDTH, int PPM_HEIGHT,
	   int rateW, int rateH, int withheader, int order,
	   struct Image *cameraImage) {
  int x, y;
  int Sy;
  long cr, cb;
  long L;
  long r,g,b;
  u_char *Y;
  u_char *Cr;
  u_char *Cb;
  int i;
  int imageIndex = 0;
  char str[256];

  i = 0;

  cameraImage->image_size = 3 * PPM_WIDTH * PPM_HEIGHT;
  if(withheader) {
     sprintf(str, "P6\n%d %d\n255\n", PPM_WIDTH, PPM_HEIGHT);
     cameraImage->image_size += strlen(str);
  }

  cameraImage->image = (void *)malloc(cameraImage->image_size);
  if (cameraImage->image == NULL) {
    cameraImage->image_size = 0;
    return(0);
  }

  if(withheader) {
       memcpy(&cameraImage->image[imageIndex], str, strlen(str));
       imageIndex += strlen(str);
  }

  Y = buf;

  Cb = Y + (PPM_HEIGHT * PPM_WIDTH);
  Cr = Cb + (PPM_HEIGHT / rateH) * (PPM_WIDTH / rateW);

  for( Sy = 0 ; Sy < PPM_HEIGHT; Sy++){
    if(order)
      y = PPM_HEIGHT - Sy -1;
    else
      y = Sy;

    for( x = 0 ; x < PPM_WIDTH ; x++){
      L = Y[y * PPM_WIDTH + x] *  100000; 
      cb = Cb[(y / rateH) * PPM_WIDTH /rateW + (x /rateW)];
      if(cb > 127) cb = cb - 256;
      cr = Cr[(y / rateH) * PPM_WIDTH /rateW + (x /rateW)];
      if(cr > 127) cr = cr - 256;

      r = L + 140200 * cr;
      g = L - 34414 * cb - 71414 * cr;
      b = L + 177200 * cb;
      
      r = r / 100000;
      g = g / 100000;
      b = b / 100000;

      NORM(r);
      NORM(g);
      NORM(b);
      if(order){
	cameraImage->image[imageIndex++] = b;
	i++;

	cameraImage->image[imageIndex++] = g;
	i++;

	cameraImage->image[imageIndex++] = r;
	i++;
      } else {
	cameraImage->image[imageIndex++] = r;
	i++;

	cameraImage->image[imageIndex++] = g;
	i++;

	cameraImage->image[imageIndex++] = b;
	i++;
      }
    }
  }

  cameraImage->image_size = imageIndex;
  return(i);
}

