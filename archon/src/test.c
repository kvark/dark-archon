/*	test.c
 *	Separate alphabet re-ordering benchmark utility
 */ 

#include "common.h"
#include <stdio.h>
#include <string.h>


int log2(int x)	{
	return x>1 ? 1+log2(x>>1) : 0;
}

struct Estimation	{
	int n_flat, n_log;
	char name[10];
};

FILE *fi = NULL;

typedef void (*f_estimate)(struct Estimation *const);


void fit1(struct Estimation *const e)	{
	int i, r[256] = {0};
	byte a=0,b=0;
	fread(&a,1,1,fi);
	while( fread(&b,1,1,fi) )	{
		r[a] += (b>a);
		a = b;
	}
	for(i=0; i<256; ++i)	{
		e->n_flat += r[i];
		e->n_log += r[i] * log2(r[i]);
	}
	strcpy(e->name,"IT-1");
}

void fit2(struct Estimation *const e)	{
	strcpy(e->name,"IT-2");
}


//------------------------------------------//
//					MAIN					//

int main(const int argc, const char *argv[])	{
	const f_estimate farray[] = {
		fit1,fit2,NULL
	};
	const f_estimate *pfun = NULL;
	int total = 0;

	printf("Suffix sort estimation utility.\n");
	printf("\tArchon project. (C) Dzmitry Malyshau.\n");
	if(argc<2)
		return -1;
	fi = fopen(argv[1],"rb");
	if(!fi)
		return -2;
	fseek(fi,0,SEEK_END);
	total = ftell(fi);

	for(pfun=farray; *pfun; ++pfun)	{
		struct Estimation est = {0,0,""};
		fseek(fi,0,SEEK_SET);
		(*pfun)(&est);
		printf("Estimating %s...\n", est.name);
		printf("\tFlat: %.2f, Log: %.2f\n",
			est.n_flat * 1.f / total, est.n_log * 1.f / total);
	}
	fclose(fi);
	return 0;
}