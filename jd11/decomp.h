#ifndef _DECOMP_H
#define _DECOMP_H
struct compstate {
	unsigned char	curmask;
	unsigned char	bytebuf;
	unsigned char	*byteptr;
};
extern void picture_decomp_v1(char *compressed,char *uncompressed,int width,int height);
extern void picture_decomp_v2(char *compressed,char *uncompressed,int width,int height);  
#endif
