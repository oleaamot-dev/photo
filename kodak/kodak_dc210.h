
extern struct Image *kodak_dc210_get_picture (int picNum, int thumbnail);
extern unsigned char kodak_dc210_checksum(char packet[],int length);
extern unsigned char kodak_dc210_read_byte ( int serialdev );

extern int kodak_dc210_write_byte ( int serialdev, char b );
extern int kodak_dc210_read ( int serialdev, unsigned char *buf, int nbytes );

