/*	test.c
 *	Separate alphabet re-ordering benchmark utility
 */ 

#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int log2i(int x)	{
	return x>1 ? 1+log2i(x>>1) : 0;
}
static int complexity(int x)	{
	return x * log2i(x);
}

struct Estimation	{
	int n_flat, n_log;
};

void sort(struct Estimation *const e, int num)	{
	e->n_flat += num;
	e->n_log += complexity(num);
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
}

// IT-1-exteme:
// choose smallest symbol group B, sort it
// mark AB for each A as sorted

void fit1x(struct Estimation *const e)	{
	int i, ra[1<<8] = {0}, rb[1<<16] = {0};
	byte a=0,b=0;
	fread(&a,1,1,fi);
	while( ++ra[a] && fread(&b,1,1,fi) )	{
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
}

// IT-1-y:
// choose the group B, which yields
// the highest number of elements per a sorted one
// mark AB for each A as sorted

void fit1y(struct Estimation *const e)	{
	int i, rx[0x10000] = {0};
	byte a=0,b=0;
	fread(&a,1,1,fi);
	while( fread(&b,1,1,fi) )	{
		rx[(a<<8)+b] += 1;
		a = b;
	}
	for(;;)	{
		int ms = 0, mval = 0;
		for(i=0; i!=0x100; ++i)	{
			int j, nComplexity = 1, nBenefit = 0, cv;
			for(j=0; j!=0x100; ++j)	{
				nComplexity += complexity( rx[(i<<8)+j] );
				nBenefit += rx[(j<<8)+i];
			}
			cv = (nBenefit<<4) / nComplexity;
			if(cv > mval)	{
				ms = i; mval = cv;
			}
		}
		if(!mval)
			break;
		//printf("\tSymbol %02X, score %d\n", ms, mval);
		for(i=0; i<(1<<8); ++i)	{
			int *const x = rx + (ms<<8)+i;
			sort(e,*x);
			*x = 0;
		}
		for(i=0; i<(1<<8); ++i)	{
			rx[(i<<8)+ms] = 0;
		}
	}
}

// IT-12-extreme:
// choose smallest pair-symbol group BC, sort it
// mark ABC for each symbol A as sorted

void fit12x(struct Estimation *const e)	{
	int i, r[1<<16] = {0};
	byte a=0,b=0,c=0;
	int *const rx = malloc(sizeof(int)<<24);

	memset(rx,0,sizeof(int)<<24);
	fread(&a,1,1,fi);
	fread(&b,1,1,fi);
	while( ++r[(a<<8)+b] && fread(&c,1,1,fi) )	{
		rx[(a<<16)+(b<<8)+c] += 1;
		a = b; b = c;
	}
	for(i=0; i<0x10000; ++i)	{
		if(!r[i])
			r[i] = -1;
	}
	for(;;)	{
		int min = 0;
		for(; min<0x10000 && r[min]<0; ++min);
		if(min == 0x10000)
			break;
		for(i=min+1; r[min] && i<0x10000; ++i)	{
			if(r[i]>=0 && r[i]<r[min])
				min = i;
		}
		sort(e,r[min]);
		//printf("\tSymbol %c%c: %d\n", min>>8, min, r[min]);
		//printf("\tSymbol %04X: %d\n", min, r[min]);
		r[min] = -1;
		for(i=0; i<0x100; ++i)	{
			int *const x = r + (i<<8)+(min>>8);
			if(*x>0)	{
				*x -= rx[(i<<16)+min];
				assert( *x>=0 );
			}
		}
	}
	free(rx);
}


// IT-2-exteme-1st:
// use the order from IT-1x
// in the IT-2 scheme

void fit2x1(struct Estimation *const e)	{
	int i, r[0x10000] = {0};
	byte a=0,b=0,c=0, order[0x100] = {0};
	const int startPos = ftell(fi);
	// find out the IT-1x order
	memset(order,0xFF,sizeof(order));
	fread(&a,1,1,fi);
	while( fread(&b,1,1,fi) )	{
		r[(a<<8)+b] += 1;
		order[a] = 0;
		a = b;
	}
	for(;;)	{
		int ms=-1, mv=1<<30;
		for(i=0; i!=0x100; ++i)	{
			int j, cur = 0;
			if(order[i])
				continue;
			for(j=0; j!=0x100; ++j)	{
				cur += r[(i<<8)+j];
			}
			if(cur>0 && cur<mv)	{
				ms = i; mv = cur;
			}
		}
		if(ms<0)
			break;
		order[ms] = ++c;
		for(i=0; i!=0x100; ++i)	{
			r[(ms<<8)+i] = r[(i<<8)+ms] = 0;
		}
	}
	fseek(fi,startPos,SEEK_SET);
	// apply the order to IT-2
	memset(r,0,sizeof(r));
	fread(&a,1,1,fi);
	fread(&b,1,1,fi);
	while( fread(&c,1,1,fi) )	{
		r[(a<<8)+b] += (order[c]>order[b] && order[a]>=order[b]);
		a = b; b = c;
	}
	for(i=0; i!=0x10000; ++i)	{
		sort(e,r[i]);
	}
}


// IT-1-search:
// find local minimums for a number of random configurations

int checkOrder(const byte arr[])	{
	static byte temp[0x100];
	int i;
	memset(temp,0,sizeof(temp));
	for(i=0; i!=0x100; ++i)	{
		const byte x = arr[i];
		if(temp[x])
			return -1;
		temp[x] = 1;
	}
	return 0;
}

void fit1s(struct Estimation *const e)	{
	const int sBubble = 0;
	int i, r[0x10000] = {0}, rd[0x10000], bestScore = 0;
	const int nInitial = 100, nLocal = 100;
	byte bestOrder[0x100], order[0x100];	//rank->symbol
	byte a=0,b=0;
	// code
	srand(0);
	fread(&a,1,1,fi);
	while( fread(&b,1,1,fi) )	{
		++r[(a<<8)+b];
		a = b;
	}
	for(i=0; i!=0x10000; ++i)	{
		const int j = ((i&0xFF)<<8) + (i>>8);
		rd[i] = complexity(r[i]) - complexity(r[j]);
		bestScore += complexity(r[i]) * ((i<<8)<(i&0xFF));
	}
	for(i=0; i!=0x100; ++i)
		bestOrder[i] = i;
	for(i=0; i!=nInitial; ++i)	{
		int j, nLeft, curScore = 0;
		// choose random order
		memcpy( order, bestOrder, sizeof(order) );
		for(j=1; j!=0x100; ++j)	{
			const int target = rand() % (j+1);
			const byte x = order[j];
			order[j] = order[target];
			order[target] = x;
		}
		// find current score
		for(j=0; j!=0x10000; ++j)	{
			a = order[j>>8]; b = order[j&0xFF];
			curScore += complexity(r[(a<<8)+b]) * ((j>>8) < (j&0xFF));
		}
		// find local minimum (bubble method)
		for(nLeft=nLocal; sBubble && --nLeft;)	{
			int add = 0;
			j = rand()%255;
			a = order[j+0]; b = order[j+1];
			add = rd[(b<<8)+a];;
			if(add > 0)	{
				nLeft = nLocal;
				curScore += add;
				order[j+0] = b;
				order[j+1] = a;
			}
		}
		for(nLeft=nLocal; !sBubble && --nLeft;)	{
			int add = 0;
			byte x,q,e;
			a = rand()&0xFF; b = rand()&0xFF;
			if(a>b)	{
				x=a; a=b; b=a;
			}
			q = order[a]; e = order[b];
			add = rd[(e<<8)+q];
			// find the new score
			for(j=a+1; j<b; ++j)	{
				x = order[j];
				add += rd[(x<<8)+q] + rd[(e<<8)+x];
			}
			if(add > 0)	{
				nLeft = nLocal;
				curScore += add;
				order[b] = q;
				order[a] = e;
			}
		}
		assert( !checkOrder(order) );
		if(curScore > bestScore)	{
			memcpy( bestOrder, order, sizeof(order) );
			bestScore = curScore;
		}
	}
	// check order
	assert( !checkOrder(bestOrder) );
	// sort, finally
	for(i=0; i!=0x10000; ++i)	{
		int rx;
		a = bestOrder[i>>8]; b = bestOrder[i&0xFF];
		rx = r[(a<<8)+b];
		if((i>>8) >= (i&0xFF))
			sort(e,rx);
		else
			bestScore -= complexity(rx);
	}
	assert(!bestScore);
}



//------------------------------------------//
//					MAIN					//

int main(const int argc, const char *argv[])	{
	struct EstiFunc	{
		char name[12];
		f_estimate fun;
	}const farray[] = {
		{"IT-1",	fit1},
		{"IT-1x",	fit1x},
		{"IT-1y",	fit1y},
		{"IT-1s",	fit1s},
		{"IT-12x",	fit12x},
		{"IT-2",	fit2},
		{"IT-2x1",	fit2x1},
		{"",NULL}
	};
	const struct EstiFunc *pe = NULL;
	int total = 0;
	float kn0, kn1;

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
	kn0 = 1.f / total;
	kn1 = 1.f / (total * log2i(total));

	for(pe=farray; pe->fun; ++pe)	{
		struct Estimation est = {0,0};
		fseek(fi,0,SEEK_SET);
		printf("Estimating %s...\n", pe->name);
		pe->fun(&est);
		printf("\tFlat: %.2f, Log: %.2f\n", est.n_flat * kn0, est.n_log * kn1);
	}
	fclose(fi);
	return 0;
}