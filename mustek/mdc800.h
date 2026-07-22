#ifndef DEFINE_MDC800_H
#define DEFINE_MDC800_H

#define null 0
#include "../src/gphoto.h"

/* Definition of API in mdc800.c */

int mdc800_initialize ();
int mdc800_close ();

char* mdc800_summary ();
char* mdc800_description();
struct Image* mdc800_get_picture (int,int);
int mdc800_delete_image (int);
int mdc800_number_of_pictures ();
int mdc800_take_picture ();
struct Image* mdc800_get_preview ();

#endif
