#include "qm100.h"
/*---------------------------------------------------------------------*
 *                                                                     *
 * error - deal with camera error.                                     *
 *                                                                     *
 *       1. Issue message to console                                   *
 *       2. Reset the serial port                                      *
 *       3. If recovery environment exists, jump to it,                *
 *          otherwise exit to the system.                              *
 *                                                                     *
 *---------------------------------------------------------------------*/
void qm100_error(int serialdev, char *operation, int error)
{
   if (error)
      sprintf(qm100_errmsg, "%s: %s", operation, strerror(error));
   else
      strcpy(qm100_errmsg, operation);
   if (serialdev > 0)
      {
      int c;
      usleep(10);
      qm100_writeByte(serialdev, SIO_NAK);
      usleep(10);
      qm100_resetUart(serialdev);
      while ((c=qm100_readTimedByte(serialdev)))
             {
             qm100_readByte(serialdev);
             qm100_writeByte(serialdev, SIO_NAK);
             }
      }
   if (qm100_trace)
      fprintf(qm100_trace, "%s\n", qm100_errmsg);
   if (qm100_recovery)
      longjmp(qm100_jmpbuf, 1);
   fprintf(stderr, "%s - terminating\n", qm100_errmsg);
   exit(1);
}
