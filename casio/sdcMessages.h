#ifndef _SDCMESSAGES_H
#define _SDCMESSAGES_H

#define MALLOC_ERROR "Unable to allocate memory in function %s\n"
#define CANT_OPEN_DEVICE "Unable to connect to port %s\n"
#define CANT_READ_DEVICE "Unable to read port in %s\n"
#define CANT_SET_EXCLUSIVE "Can't set exclusive mode on port.\n"
#define CANT_SET_HOLD "Can't set hold mode on port\n";
#define CANT_GET_ATTRIBUTE "In %s  can't get port attribute\n"
#define CANT_SET_ATTRIBUTE "In %s  can't set port attribute\n"
#define CANT_SET_RTS "Can't set RTS %s\n"
#define CANT_SET_DTR "Can't set CTS DTR %s\n"
#define READ_TIMEOUT "Timeout occurred while trying to read port\n"
#define PORT_IS_CLOSED "Function %s called an a port that is not open\n"

#endif /* _SDCMESSAGES_H */
