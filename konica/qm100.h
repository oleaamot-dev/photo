#ifndef  _QM100_H
#define  _QM100_H
#include "config.h"
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#if defined(MAX)
#undef MAX
#endif
#if defined(MIN)
#undef MIN
#endif
#include <sys/param.h>
#endif
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "defs.h"
#include "configDialog.h"
#include "close.h"
#include "dump.h"
#include "erasePic.h"
#include "error.h"
#include "formatCF.h"
#include "getPicInfo.h"
#include "getStatus.h"
#include "lowlevel.h"
#include "open.h"
#include "savePic.h"
#include "saveThumb.h"
#include "setSpeed.h"
#include "takePic.h"
#include "transmission.h"
#include "rcutil.h"
#ifdef DEFINE_GLOBALS
#define  XTRN
#else
#define  XTRN extern
#endif
XTRN QM100_CONFIGDATA qm100_configData;
XTRN FILE    *qm100_trace;
XTRN char     qm100_errmsg[128];
XTRN int      qm100_main;
XTRN int      qm100_sendPacing;
XTRN int      qm100_pictureCount;
XTRN int      qm100_recovery;
XTRN int      qm100_showBytes;
XTRN int      qm100_showStatus;
XTRN int      qm100_transmitSpeed;
XTRN int      qm100_escapeCode;
XTRN jmp_buf  qm100_jmpbuf;
XTRN struct   termios newt;
XTRN struct   termios oldt;
XTRN double   qm100_percent;
XTRN double   qm100_percentIncr;
#endif
