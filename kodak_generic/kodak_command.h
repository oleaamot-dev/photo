#ifndef KODAK_COMMAND_H
#define KODAK_COMMAND_H

/*
 *      Copyright (C) 1999 Randy Scott <scottr@wwa.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published 
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdarg.h>

#include "kodak_generic.h"

#define CMD_SET_HPBS         0x2A
#define CMD_BAUD_RATE        0x41
#define CMD_SEND_LAST_PIC    0x4C
#define CMD_TAKE_PICTURE     0x7C
#define CMD_STATUS           0x7F
#define CMD_READ_PIC_INFO    0x91
#define CMD_READ_THUMBNAIL   0x93
#define CMD_OPEN_CARD        0x96
#define CMD_CLOSE_CARD       0x97
#define CMD_READ_DIRECTORY   0x99
#define CMD_READ_FILE        0x9A
#define CMD_DELETE_FILE      0x9D

#define COMMAND_RESPONSE_COMPLETE   0x00
#define COMMAND_RESPONSE_ACK        0xD1
#define COMMAND_RESPONSE_CORRECT    0xD2
#define COMMAND_RESPONSE_NAK        0xE1
#define COMMAND_RESPONSE_EXC_ERROR  0xE2
#define COMMAND_RESPONSE_ILL_PACKET 0xE3
#define COMMAND_RESPONSE_CANCEL     0xE4
#define COMMAND_RESPONSE_BUSY       0xF0


int kodak_command_create (KODAK_CAMERA_TYPE, unsigned char, ...);
int kodak_command_vcreate (KODAK_CAMERA_TYPE, unsigned char, va_list);
unsigned char * kodak_command_get (int);

#endif /* KODAK_COMMAND_H */
