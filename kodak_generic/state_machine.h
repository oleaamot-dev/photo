#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/*
 * Copyright (c) 1999, Randy Scott <scottr@wwa.com>.  All rights reserved.
 */

#include "kodak_generic.h"

typedef enum
{
   STATE_INVALID,/* used internally */
   STATE_AGAIN,  /* used internally */
   STATE_REPEAT, /* repeat the state */
   STATE_NEXT,   /* advance to next state */
   STATE_ABORT,  /* abort the state machine */
   STATE_0,      /* Go to state 0 */
   STATE_1,      /* ... */
   STATE_2,
   STATE_3,
   STATE_4,
   STATE_5,
   STATE_6,
} NEXT_STATE_TYPE;

typedef enum
{
   ERROR_UNKNOWN,
   ERROR_WRITE,
   ERROR_READ,
   ERROR_TIMEOUT
} ERROR_TYPE;

typedef struct
{
   int descriptor;
   int bytes_write;
   int bytes_read;
   unsigned char *(*write_data)(int);
   NEXT_STATE_TYPE (*read_data)(int, unsigned char *);
   NEXT_STATE_TYPE (*error_handler)(int, ERROR_TYPE);
} STATE_MACHINE_LINE;

typedef struct _STATE_MACHINE_INSTANCE
{
   unsigned char *device;
   int initial_baud;
   void (*driver_init)(struct _STATE_MACHINE_INSTANCE *);

   int fd;
   int baud;
   int is_usb;

   int current;
   int num_states;
   STATE_MACHINE_LINE *states;

   int num_tx;
   int num_rx;

   unsigned char *rx_buffer;
} STATE_MACHINE_INSTANCE;

typedef struct
{
   char *device;
   int baud_rate;
   void (*driver_init)(STATE_MACHINE_INSTANCE *);
} STATE_MACHINE_TEMPLATE;

typedef struct
{
   int num_states;
   STATE_MACHINE_LINE *states;
} STATE_MACHINE_PROGRAM;

STATE_MACHINE_INSTANCE *state_machine_construct (STATE_MACHINE_TEMPLATE *);
void state_machine_set_baud (STATE_MACHINE_INSTANCE *, int);
void state_machine_program (STATE_MACHINE_INSTANCE *, STATE_MACHINE_PROGRAM *);
BOOLEAN state_machine_run (STATE_MACHINE_INSTANCE *);
void state_machine_assert_break (STATE_MACHINE_INSTANCE *);
void state_machine_reinitialize (STATE_MACHINE_INSTANCE *);

#endif /* STATE_MACHINE_H */
