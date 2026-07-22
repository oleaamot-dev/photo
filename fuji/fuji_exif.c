/*
  Thumbnail extraction from EXIF file for GPHOTO package
  Copyright (C) 1998 Matthew G. Martin

  This routine works for DS-7 thumbnail files.  Don't know about any others.
    
    Most of this code was taken from
    GDS7 v0.1 interactive digital image transfer software for DS-7 camera
    Copyright (C) 1998 Matthew G. Martin

    Which was directly derived from get_ds7, a Perl Language library
    Copyright (C) 1997 Mamoru Ohno


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "gphoto_fuji.h"
#include "exif.h"

/***********************************************************************
   EXIF handling functions
 **********************************************************************/

/* 
   New conversion function, all in memory 
*/
unsigned char *fuji_exif_convert(exifparser *exifdat){
  char *tmpstr[32];
  unsigned char *imagedata,*exifimg,*newimg,*curptr;
  unsigned int exiflen,offset,entry;
    long dataptr,dsize,tag,datvec,size,tmp;
  int i,j;

  if (exif_header_parse(exifdat)<0) return(NULL);
  /* Check out the exif data */
  if (stat_exif(exifdat)) return(NULL); /* Couldn't parse exif data,quit */

  newimg=malloc(exifdat->exiflen);
  if (newimg==NULL){
    fprintf(stderr,"fuji_exif_convert: could not malloc\n");
    return(NULL);
  };

  /* Copy header*/
  memcpy(newimg,exifdat->data,8);
  curptr=newimg+8;

  offset=lilend(exifdat->data+4,4);

  if (fuji_debug) {
    printf("Offset is %d bytes\n",offset);
    dump_exif(exifdat);
  };

  /* Skip to TIFF image data */
  if(exifdat->ifdcnt<2) {
    if (fuji_debug) {
      fprintf(stderr,"Too few ifds, doesn't look right. Giving up\n");
    };
    return(NULL); /* Things don't look right...*/
  };

  if (fuji_debug) printf("New Offset is %d bytes\n",offset);

  /* Jump to thumbnail image data */
  exifimg=exifdat->ifds[1];

  /* Copy number of entries */
  memcpy(curptr,exifimg,2);
  curptr+=2;

  entry=lilend(exifimg,2);

  if (fuji_debug) printf("Entry is %d \n",entry);

  /* See if thumb is a JPEG */
  tmp=getintval(exifimg,EXIF_JPEGInterchangeFormat); /*imagedata start*/
  if (tmp>0) { /* jpeg image */
    if (fuji_debug) fprintf(stderr,"Found jpeg thumb data\n");
    dsize=getintval(exifimg,EXIF_JPEGInterchangeFormatLength);
    if (dsize==-1){
      fprintf(stderr,"No Jpeg size tag for thumbnail, skipping\n");
      return(NULL);
    };
    imagedata=exifdat->data+tmp;
    memcpy(newimg,imagedata,dsize);
    return(newimg);
  };

  /* Try a TIFF */
  tmp=getintval(exifimg,EXIF_StripOffsets); /*imagedata start*/
  if (tmp==-1) {
    fprintf(stderr,"fuji_exif: Tiff or jpeg data not found, skipping\n");
    return(NULL);
  };
  imagedata=exifdat->data+tmp;

  dataptr=getintval(exifimg,EXIF_StripByteCounts);        /* imagedata size */
  if (dataptr==-1) {
    printf("Split two\n");
    return(NULL);
  };

  if (fuji_debug) printf("Imagedata size is %ld bytes\n",dataptr);

  for (i=0;i<entry;i++){
    dsize=datsize(exifimg,i);
    tag=tagnum(exifimg,i);

    /*
      if (fuji_debug) printf("Datsize %d (tag=%ld) is %ld\n",i,tag,dsize);
    */

    if (tag==EXIF_StripOffsets) {
      setval(exifimg,i,12*entry+14); /* set to end of directory */
      memcpy(curptr,exifimg+12*i+2,12);
      curptr+=12;
    }
    else {
      if (dsize<5){
	/* Just copy the field if small */
        memcpy(curptr,exifimg+12*i+2,12);
	curptr+=12;
      }
      else{
	datvec=theval(exifimg,i);
	setval(exifimg,i,dataptr+12*entry+14);
	for (j=0;j<dsize;j++) imagedata[dataptr++]=exifdat->data[datvec+j];
        memcpy(curptr,exifimg+12*i+2,12);
	curptr+=12;
      };
    };
  };
  memcpy(curptr,exifimg+12*entry+10,4); /* Write 4 zero bytes */
  curptr+=4;
  memcpy(curptr,imagedata,dataptr);/* ? */
  curptr+=dataptr;
  return newimg;
};
