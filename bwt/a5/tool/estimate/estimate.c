#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <time.h>

typedef unsigned char byte;
#define	MAX_ORDER	10

int main(const int argc, const char *const argv[])	{
	byte a,b;
	int fc[1<<MAX_ORDER];
	int i=10, mask;
	int order=1, n=0;
	FILE *fi = NULL;
	
	printf("a5: Group size estimator\n");
	printf("\tArchon project (C) Dzmitry Malyshau\n");

	if(argc>=2)	{
		sscanf(argv[1],"%d",&order);
		assert(order>=0 && order<=MAX_ORDER);
	}
	
	if(argc>=4)	{
		if(!strcmp(argv[2],"-n"))	{
			sscanf(argv[3],"%d",&i);
			assert(i>0 && i<30);
		}else
		if(!strcmp(argv[2],"-f"))	{
			fi = fopen(argv[3],"rb");
		}else	{
			printf("Option (%s) is not recognized!\n", argv[2]);
		}
	}else	{
		printf("Command: estimate <it-order=%d> "
			"[-n <data_pow2_size>(%d) | -f <file_path>]\n",
			order, i);
	}

	if(fi)	{
		printf("Using file: %s\n", argv[3]);
	}else	{
		srand( (unsigned)time(NULL) );
		n = 1<<i;
		printf("Using random size: %d\n", n);
	}

	memset(fc, 0, sizeof(int)<<order);
	a=b=0xFF; mask=0;
	for(i=0; ;)	{
		const int sym = (fi ? fgetc(fi) : rand());
		const byte c = (byte)sym;
		int bit;
		if(sym<0) break;
		if(c!=b) a=b;
		b = c;
		bit = (a>c ? 1:0);
		mask = (mask + mask + bit) & ((1<<order)-1);
		assert(!(mask>>order));
		fc[mask] += 1;
		if(++i>=n && !fi)
			break;
	}

	if(fi)	{
		fclose(fi);
		fi = NULL;
		n = i;
	}
	
	for(i=0; !(i>>order); ++i)	{
		int j;
		for(j=0; j!=order; ++j)	{
			int code = (i>>(order-1-j))&1;
			putchar(code+'0');
		}
		printf(":\t%.3f\n", fc[i]*1.f/n);
	}
	
	return 0;
}
