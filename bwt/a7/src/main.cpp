#include <stdio.h>
#include <string.h>
#include <time.h>

#include "archon.h"

static const char sUsage[] = "Usage: archon [e|d] <in> <out>\n";


int main(const int argc, const char *const argv[])	{
	FILE *fx; bool mode; time_t t0;
	printf("Archon-7 prototype\n");
	if(argc != 4)	{
		printf("%s",sUsage);
		return -1;
	}
	if(!strcmp(argv[1],"e")) mode = true; else
	if(!strcmp(argv[1],"d")) mode = false; else	{
		printf("%s",sUsage);
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
		ar.enRead(fx,N);
		fclose(fx);
		printf("Encoding SA...\n");
		t0 = clock();
		ar.enCompute();
		t0 = clock()-t0;
#		ifndef NO_VALIDATE
		printf("Validating...");
		const bool rez = ar.validate();
		printf("%s\n", rez?"OK":"Fail");
#		endif
#		ifndef NO_WRITE
		printf("Writing BWT...\n");
		fx = fopen(argv[3],"wb");
		if(!fx)
			return -3;
		ar.enWrite(fx);
#		endif
	}else	{
		N -= sizeof(int);
		if(N<=0)
			return -2;
		printf("Reading BWT...\n");
		ar.deRead(fx,N);
		fclose(fx);
		printf("Decoding SA...\n");
		t0 = clock();
		ar.deCompute();
		t0 = clock()-t0;
		printf("Writing raw...\n");
		fx = fopen(argv[3],"wb");
		if(!fx)
			return -3;
		ar.deWrite(fx);
	}
	fclose(fx);
	printf("SA time: %.2f sec\n", t0*1.f/CLOCKS_PER_SEC);
	printf("Done.\n");
	return 0;
}
