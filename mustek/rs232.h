#ifndef _RS232_H
#define _RS232_H

int mdc800_rs232_sendCommand (char* , char * , int );
int mdc800_rs232_waitForCommit (char commandid);
int mdc800_rs232_receive (char * , int );
int mdc800_rs232_download (char *, int);

#endif
