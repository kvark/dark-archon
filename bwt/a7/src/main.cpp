#include <stdio.h>

#include "common.h"
#include "archon.h"

int main(const int argc, const char *const argv[])	{
	FILE *fx;
	printf("Archon-7 prototype\n");
	if(argc != 3)
		return -1;
	printf("Reading...\n");
	fx = fopen(argv[1],"rb");
	fseek(fx,0,SEEK_END);
	if(!fx)
		return -2;
	const int N = ftell(fx);
	fseek(fx,0,SEEK_SET);
	Archon ar(N);
	ar.read(fx);
	fclose(fx);
	printf("Computing...\n");
	ar.compute();
	printf("Writing...\n");
	fx = fopen(argv[2],"wb");
	if(!fx)
		return -3;
	ar.write(fx);
	fclose(fx);
	printf("Done.\n");
	return 0;
}
