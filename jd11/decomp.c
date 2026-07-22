#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "decomp.h"

/* FLOAT_QUERY: 0.860000 1.030000 1.150000. */

/********************************************************/
/* The bit stage                                        */
/********************************************************/
static inline int getbit(struct compstate *cs) {
	int	ret;
	if (cs->curmask == 0x80)
		cs->bytebuf = *cs->byteptr++;
	ret = cs->curmask & cs->bytebuf;
	cs->curmask >>=1;
	if (!cs->curmask) cs->curmask = 0x80;
	return !!ret;
}

/********************************************************/
/* The huffman compressor stage                         */
/********************************************************/

struct chain { int 	left,val,right; };

static const int df[] = {
	-180, 180, 1000, -90, 1000, 90, 1000, -45, 1000, 45, 1000, -20, 1000,
	 20, 1000, -11, 1000, 11, 1000, -6, 1000, 2, 1000, 6, -2, 1000, 1000
};

static struct chain	cl[200];
static int		stackstart;

int decomp_1byte(struct compstate *cs) {
	int	xcs = stackstart;
	int	xbit;

	while ((cl[xcs].left>=0) && (cl[xcs].right>=0)) {
		xbit = getbit(cs); 
		if (xbit)
			xcs = cl[xcs].left;
		else
			xcs = cl[xcs].right;
	}
	return cl[xcs].val;
}

void build_huffmann_tree() {
	int	xstack[200];
	int	i,curcl=0,curstack=0;


	for (i=0;i<sizeof(df)/sizeof(df[0]);i++) {
		if (df[i]!=1000) {
			cl[curcl].left = -1;
			cl[curcl].right = -1;
			cl[curcl].val = df[i];
		} else {
			cl[curcl].right	= xstack[--curstack];
			cl[curcl].left	= xstack[--curstack];
		}
		xstack[curstack++] = curcl++;
	}
	stackstart = xstack[0];
}

#define F1	0.5
#define F2	0.0
#define F3	0.5
#define F4	0.0

void
picture_decomp_v1(char *compressed,char *uncompressed,int width,int height) {
	struct	compstate cs;
	unsigned char xbyte;
	int	i=0,curbyte=0,diff,lastval;
	int	*line,*lastline;

	cs.curmask = 0x80; cs.bytebuf = 0; cs.byteptr = compressed;

	build_huffmann_tree();

	line	= (int*)malloc(sizeof(int)*width);
	lastline= (int*)malloc(sizeof(int)*width);

	curbyte=0;
	memset(line,0,sizeof(line));
	memset(lastline,0,sizeof(line));
	for (i=0;i<width;i++) {
		diff = decomp_1byte(&cs);
		curbyte+=diff;
		xbyte = curbyte;
		if (curbyte>255) xbyte = 255;
		if (curbyte<0) xbyte = 0;

		*uncompressed++=xbyte;

		line[i] = curbyte;
	}
	height--;
	while (height--) {
		lastval = line[0]; /* just before the copy */
		memcpy(lastline,line,width*sizeof(int));
		memset(line,0,width*sizeof(int));
		for (i=0;i<width;i++) {
			diff = decomp_1byte(&cs);
			line[i]=diff+lastval;
			if (i<width-2) {
				lastval = (int)((lastline[i+2]*F4+lastline[i]*F2+lastline[i+1]*F1+line[i]*F3));
			} else {
				if (i==width-2)
					lastval = (int)(lastline[i]*F2+lastline[i+1]*F1+line[i]*F3);
				else
					lastval = line[i];
			}

			xbyte = line[i];
			if (line[i]>255) xbyte = 255;
			if (line[i]<0)	 xbyte = 0;
			*uncompressed++=xbyte;
		}
	}
}

/* Just blow up the picture from 6 bit uncompressed to 8 bit uncompressed */
void
picture_decomp_v2(char *compressed,char *uncompressed,int width,int height) {
	struct	compstate cs;
	int	i,j;
	unsigned char xbyte;

	cs.curmask = 0x80; cs.bytebuf = 0; cs.byteptr = compressed;
	for (i=width*height;i--;) {
		unsigned char xmask = 0x80;
		xbyte = 0;
		for (j=6;j--;) {
			if (getbit(&cs))
				xbyte|=xmask;
			xmask>>=1;
		}
		*uncompressed++=xbyte;
	}
}
