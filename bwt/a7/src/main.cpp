#include <stdio.h>
#include <string.h>
#include <time.h>

#include "archon.h"

static const char sUsage[] = "Usage: archon [e|d] <in> <out>\n";


int main(const int argc, const char *const argv[])	{
	FILE *fx; bool mode; time_t t0;
	printf("Archon-7 prototype\n");
	if(argc != 4)	{
		printf(sUsage);
		return -1;
	}
	if(!strcmp(argv[1],"e")) mode = true; else
	if(!strcmp(argv[1],"d")) mode = false; else	{
		printf(sUsage);
		return -1;
	}
	printf("Initializing...\n");
	fx = fopen(argv[2],"rb");
	if(!fx)
		return -2;
	fseek(fx,0,SEEK_END);
	int N = ftell(fx);
	if(!N)
		return -3;
	Archon ar(N);
	const unsigned mem = ar.countMemory();
	printf("Allocated %dmb or %.1fn\n", mem>>20, mem*1.f/N );
	fseek(fx,0,SEEK_SET);
	if(mode)	{
		printf("Reading raw...\n");
		ar.en_read(fx,N);
		fclose(fx);
		printf("Encoding SA...\n");
		t0 = clock();
		ar.en_compute();
		t0 = clock()-t0;
		printf("Writing BWT...\n");
		fx = fopen(argv[3],"wb");
		if(!fx)
			return -3;
		ar.en_write(fx);
	}else	{
		N -= sizeof(int);
		if(N<=0)
			return -2;
		printf("Reading BWT...\n");
		ar.de_read(fx,N);
		fclose(fx);
		printf("Decoding SA...\n");
		t0 = clock();
		ar.de_compute();
		t0 = clock()-t0;
		printf("Writing raw...\n");
		fx = fopen(argv[3],"wb");
		if(!fx)
			return -3;
		ar.de_write(fx);
	}
	fclose(fx);
	printf("SA time: %.2f sec\n", t0*1.f/CLOCKS_PER_SEC);
	printf("Done.\n");
	return 0;
}
