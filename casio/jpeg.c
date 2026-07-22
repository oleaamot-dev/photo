#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "../src/gphoto.h"
#include "casio_qv_defines.h"
#include "getuint.h"
#ifdef DOS
#include "cam2jpg.h"
#include "jpegtabf.h"
#else
#include "cam2jpgtab.h"
#include "jpegtab_f.h"
#endif


int
casio_write_file(buf, len, outfp)
     u_char	*buf;
     int	len;
     FILE	*outfp;
{
  int i, l;

  i = 0;
  while( len > i) {
    l = ( (len - i) < BUFSIZ) ? (len -i) : BUFSIZ;
    if(fwrite(&buf[i], sizeof(u_char), l, outfp) != l){
      perror("casio_write_file");
      return(-1);
    };
    i = i + l;
  }
  return(i);
}

#ifdef USEWORKFILE
int
casio_write_file_file(filename, len, skip, outfp)
     char	*filename;
     long	len;
     long	skip;
     FILE	*outfp;
{
  long i, l;
  FILE *fp;
  u_char buf[BUFSIZ];

  fp = fopen(filename, RMODE);
  if(fp == NULL){
    fprintf(stderr, "can't read workfile(%s).\n", filename);
    return(-1);
  }

  for(i = 0 ; i<skip; i++)
    (void) fgetc(fp);

  len = len - skip;
  i = 0;
  while( len > i) {
    l = ( (len - i) < BUFSIZ) ? (len -i) : BUFSIZ;
    fread(buf, sizeof(u_char), l, fp);
    if(fwrite(buf, sizeof(u_char), l, outfp) != l){
      perror("casio_write_file_file");
      fclose(fp);
      return(-1);
    }
    i = i + l;
  }
  fclose(fp);
  return(i);
}
#endif

int
write_jpeg(buf, outfp)
     u_char	*buf;
     FILE	*outfp;
{
  int i = 0;
  int areaNum;
  int ysize;
  int usize;
  int vsize;
  
  areaNum =  get_u_short(buf);	/* areaNum == 0x03 */
  ysize = get_u_short(buf + 2);
  usize = get_u_short(buf + 4);
  vsize = get_u_short(buf + 6);
  i = i + 8;
  
  if(casio_write_file(soi, sizeof(soi), outfp) == -1) return(-1);
  if(casio_write_file(app0, sizeof(app0), outfp) == -1) return(-1);
  if(casio_write_file(dqt0, sizeof(dqt0), outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], 64, outfp) == -1) return(-1);
  i = i + 64;

  if(casio_write_file(dqt1, sizeof(dqt1), outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], 64, outfp) == -1) return(-1);
  i = i + 64;

  if(casio_write_file(sof, sizeof(sof), outfp) == -1) return(-1);
  if(casio_write_file(dht, sizeof(dht), outfp) == -1) return(-1);

  if(casio_write_file(sos_y, sizeof(sos_y), outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], ysize, outfp) == -1) return(-1);
  i = i + ysize;
  
  if(casio_write_file(sos_u, sizeof(sos_u), outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], usize, outfp) == -1) return(-1);
  i = i + usize;

  if(casio_write_file(sos_v, sizeof(sos_v), outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], vsize, outfp) == -1) return(-1);
  i = i + vsize;

  if(casio_write_file(eoi, sizeof(eoi), outfp) == -1) return(-1);

  return(i);
  
}

void
record_jpeg(u_char buf[], struct Image *cameraImage) {
  int i = 0;
  int areaNum;
  int ysize;
  int usize;
  int vsize;
  int imageIndex = 0;
  
  areaNum =  get_u_short(buf);	/* areaNum == 0x03 */
  ysize = get_u_short(buf + 2);
  usize = get_u_short(buf + 4);
  vsize = get_u_short(buf + 6);
  i = i + 8;

  cameraImage->image_size = 128 + ysize + usize + vsize +
  				sizeof(soi) + sizeof(app0) + sizeof(dqt0) +
				sizeof(dqt1) + sizeof(sof) +  sizeof(dht) +
				sizeof(sos_y) + sizeof(sos_u) + sizeof(sos_v) +
				sizeof(eoi);

  cameraImage->image = (void *)malloc(cameraImage->image_size);
  if (cameraImage->image == NULL) {
    cameraImage->image_size = 0;
    return;
  }
  
  memcpy(&cameraImage->image[imageIndex], soi, sizeof(soi));
  imageIndex += sizeof(soi);
  memcpy(&cameraImage->image[imageIndex], app0, sizeof(app0));
  imageIndex += sizeof(app0);
  memcpy(&cameraImage->image[imageIndex], dqt0, sizeof(dqt0));
  imageIndex += sizeof(dqt0);
  memcpy(&cameraImage->image[imageIndex], &buf[i], 64);
  imageIndex += 64;
  i = i + 64;

  memcpy(&cameraImage->image[imageIndex], dqt1, sizeof(dqt1));
  imageIndex += sizeof(dqt1);
  memcpy(&cameraImage->image[imageIndex], &buf[i], 64);
  imageIndex += 64;
  i = i + 64;

  memcpy(&cameraImage->image[imageIndex], sof, sizeof(sof));
  imageIndex += sizeof(sof);
  memcpy(&cameraImage->image[imageIndex], dht, sizeof(dht));
  imageIndex += sizeof(dht);

  memcpy(&cameraImage->image[imageIndex], sos_y, sizeof(sos_y));
  imageIndex += sizeof(sos_y);
  memcpy(&cameraImage->image[imageIndex], &buf[i], ysize);
  imageIndex += ysize;
  i = i + ysize;
  
  memcpy(&cameraImage->image[imageIndex], sos_u, sizeof(sos_u));
  imageIndex += sizeof(sos_u);
  memcpy(&cameraImage->image[imageIndex], &buf[i], usize);
  imageIndex += usize;
  i = i + usize;

  memcpy(&cameraImage->image[imageIndex], sos_v, sizeof(sos_v));
  imageIndex += sizeof(sos_v);
  memcpy(&cameraImage->image[imageIndex], &buf[i], vsize);
  imageIndex += vsize;
  i = i + vsize;

  memcpy(&cameraImage->image[imageIndex], eoi, sizeof(eoi));
  imageIndex += sizeof(eoi);

  return;
  
}

#ifdef USEWORKFILE
int
write_jpeg_fine(filename, outfp)
     char	*filename;
     FILE	*outfp;
{
  int i = 0;
  int size;
  u_char c = 0x01;
  FILE *fp;
  u_char buf[136];
  
  fp = fopen(filename, RMODE);
  if(fp == NULL){
    fprintf(stderr, "can't read workfile(%s).\n", filename);
    return(-1);
  }
  fread(buf, sizeof(u_char), 136, fp);
  fclose(fp);

  size = get_u_int(buf + 4);

  i = i + 8;
  if(casio_write_file(soi, sizeof(soi), outfp) == -1) return(-1);
  if(casio_write_file(app_f, sizeof(app_f), outfp) == -1) return(-1);
  if(casio_write_file(dqt_f, sizeof(dqt_f), outfp) == -1) return(-1);

  if(casio_write_file(&buf[i], 64, outfp) == -1) return(-1);
  i = i + 64;
  if(casio_write_file(&c, 1, outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], 64, outfp) == -1) return(-1);
  i = i + 64;
  if(casio_write_file(sof_f, sizeof(sof_f), outfp) == -1) return(-1);

  if(casio_write_file(dht_f, sizeof(dht_f), outfp) == -1) return(-1);
  
  if(casio_write_file(sos_f, sizeof(sos_f), outfp) == -1) return(-1);

  /* skip 136 byte */
  if(casio_write_file_file(filename, size, 136, outfp) == -1) return(-1);

  if(casio_write_file(eoi, sizeof(eoi), outfp) == -1) return(-1);

  return(i);
  
}
#else
int
write_jpeg_fine(buf, outfp)
     u_char	*buf;
     FILE	*outfp;
{
  int i = 0;
  int size;
  u_char c = 0x01;
  
  size = get_u_int(buf + 4);
  i = i + 8;
  if(casio_write_file(soi, sizeof(soi), outfp) == -1) return(-1);
  if(casio_write_file(app_f, sizeof(app_f), outfp) == -1) return(-1);
  if(casio_write_file(dqt_f, sizeof(dqt_f), outfp) == -1) return(-1);

  if(casio_write_file(&buf[i], 64, outfp) == -1) return(-1);
  i = i + 64;
  if(casio_write_file(&c, 1, outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], 64, outfp) == -1) return(-1);
  i = i + 64;
  if(casio_write_file(sof_f, sizeof(sof_f), outfp) == -1) return(-1);

  if(casio_write_file(dht_f, sizeof(dht_f), outfp) == -1) return(-1);
  
  if(casio_write_file(sos_f, sizeof(sos_f), outfp) == -1) return(-1);
  if(casio_write_file(&buf[i], size, outfp) == -1) return(-1);

  if(casio_write_file(eoi, sizeof(eoi), outfp) == -1) return(-1);

  return(i);
  
}

#endif
