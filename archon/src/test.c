/*	test.c
 *	Separate alphabet re-ordering benchmark utility
 */ 

#include "common.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int log2(int x)	{
	return x>1 ? 1+log2(x>>1) : 0;
}

struct Estimation	{
	int n_flat, n_log;
	char name[10];
};

void sort(struct Estimation *const e, int num)	{
	e->n_flat += num;
	e->n_log += num * log2(num);
}

FILE *fi = NULL;

typedef void (*f_estimate)(struct Estimation *const);

// IT-1: a>=b

void fit1(struct Estimation *const e)	{
	int i, r[1<<8] = {0};
	byte a=0,b=0;
	fread(&a,1,1,fi);
	while( fread(&b,1,1,fi) )	{
		r[a] += (a>=b);
		a = b;
	}
	for(i=0; i<(1<<8); ++i)	{
		sort(e,r[i]);
	}
	strcpy(e->name,"IT-1");
}

// IT-2: a>=b<c

void fit2(struct Estimation *const e)	{
	int i, r[1<<16] = {0};
	byte a=0,b=0,c=0;
	fread(&a,1,1,fi);
	fread(&b,1,1,fi);
	while( fread(&c,1,1,fi) )	{
		r[(a<<8)+b] += (c>b && a>=b);
		a = b; b = c;
	}
	for(i=0; i<(1<<16); ++i)	{
		sort(e,r[i]);
	}
	strcpy(e->name,"IT-2");
}

// IT-1-exteme:
// choose smallest symbol group B, sort it
// mark AB for each A as sorted

void fit1x(struct Estimation *const e)	{
	int i, ra[1<<8] = {0}, rb[1<<16] = {0};
	byte a=0,b=0;
	fread(&a,1,1,fi);
	while( fread(&b,1,1,fi) )	{
		ra[a] += 1;
		rb[(a<<8)+b] += 1;
		a = b;
	}
	for(;;)	{
		int min = 0;
		for(i=1; i<(1<<8); ++i)	{
			if(!ra[min] || (ra[i] > 0 && ra[i] < ra[min]))
				min = i;
		}
		if(!ra[min])
			break;
		ra[min] = 0;
		for(i=0; i<(1<<8); ++i)	{
			int *const x = rb + (min<<8)+i;
			sort(e,*x);
			*x = 0;
		}
		for(i=0; i<(1<<8); ++i)	{
			int *const x = rb + (i<<8)+min;
			if(ra[i]>0)
				ra[i] -= *x;
			*x = 0;
		}
	}
	strcpy(e->name,"IT-1x");
}

// IT-2-exteme:
// choose smallest pair-symbol group BC, sort it
// mark ABC for each symbol A as sorted

static int *r_cmp = NULL;
int cmp_word(const void *pa, const void *pb)	{
	return r_cmp[*(word*)pa] - r_cmp[*(word*)pb];
}

void fit2x(struct Estimation *const e)	{
	int i, r[1<<16] = {0};
	byte a=0,b=0,c=0;
	int *const rx = malloc(sizeof(int)<<24);
	word suf[1<<16];

	memset(rx,0,sizeof(int)<<24);
	fread(&a,1,1,fi);
	fread(&b,1,1,fi);
	while( fread(&c,1,1,fi) )	{
		r[(a<<8)+b] += 1;
		rx[(a<<16)+(b<<8)+c] += 1;
		a = b; b = c;
	}
	r_cmp = r;
	for(i=0; i<(1<<16); ++i)
		suf[i] = i;
	for(i=0;;++i)	{
		word s = 0;
		int j;
		qsort(suf+i, (1<<16)-i, sizeof(word), cmp_word);
		while( i<(1<<16) && r[suf[i]]<=0 )
			++i;
		if(i == (1<<16))
			break;
		s = suf[i];
		sort(e,r[s]);
		r[s] = 0;
		for(j=0; j<(1<<8); ++j)	{
			int *const x = r + (j<<8)+(s>>8);
			if(*x>0)
				*x -= rx[(j<<16)+s];
		}
	}
	free(rx);
	strcpy(e->name,"IT-2x");
}


//------------------------------------------//
//					MAIN					//

int main(const int argc, const char *argv[])	{
	const f_estimate farray[] = {
		fit1,fit1x, fit2,fit2x, NULL
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
	if(total<=0)
		return -3;

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