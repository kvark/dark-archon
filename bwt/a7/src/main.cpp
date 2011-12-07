#include <stdio.h>
#include <string.h>

#include "common.h"
#include "archon.h"

static const char sUsage[] = "Usage: archon [e|d] <in> <out>\n";


int main(const int argc, const char *const argv[])	{
	FILE *fx; bool mode;
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
	fseek(fx,0,SEEK_SET);
	if(mode)	{
		Archon ar(N);
		printf("Reading raw...\n");
		ar.en_read(fx,N);
		fclose(fx);
		printf("Encoding BWT...\n");
		ar.en_compute();
		printf("Writing BWT...\n");
		fx = fopen(argv[3],"wb");
		if(!fx)
			return -3;
		//fwrite( &N, sizeof(int), 1, fx );
		ar.en_write(fx);
	}else	{
		N -= sizeof(int);
		if(N<=0)
			return -2;
		//fread( &N, sizeof(int), 1, fx );
		Archon ar(N);
		printf("Reading BWT...\n");
		ar.de_read(fx,N);
		fclose(fx);
		printf("Decoding BWT...\n");
		ar.de_compute();
		printf("Writing raw...\n");
		fx = fopen(argv[3],"wb");
		if(!fx)
			return -3;
		ar.de_write(fx);
	}
	fclose(fx);
	printf("Done.\n");
	return 0;
}
