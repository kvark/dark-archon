/*	order.c
 *	Implementations for alphabet reordering routines
 */

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

extern int R1[0x100];
extern int R2[0x100][0x100];
extern byte DC[0x100];


void print_order(FILE *const fd, const int nd, const char str[])	{
	int i,j;
	fprintf(fd,"\n%s matrix:\n",str);
	for(i=0; i!=nd; ++i)
		fprintf(fd,"%d ",DC[i]);
	fputs("\n-------------\n",fd);
	for(i=0; i!=nd; ++i)	{
		for(j=0; j!=nd; ++j)
			fprintf( fd, "%d ", R2[DC[i]][DC[j]] );
		fputc('\n',fd);
	}
}


int cmp_freq(const void *p0, const void *p1)	{
	return	R1[*(byte*)p1] - R1[*(byte*)p0];
}

void optimize_none(int nd)	{
	//dummy
}

void optimize_freq(int nd)	{
	qsort(DC, nd, sizeof(byte), cmp_freq);
}


void optimize_matrix(int nd)	{
	int i,k;
	//the algorithm is not correct! :(
	FILE *const fd = fopen("matrix.txt","w");
	print_order(fd,nd,"Original");
	for(k=nd; --k;)	{
		for(i=0; i+k!=nd; ++i)	{
			const int a = DC[i], b = DC[i+k];
			if(R2[a][b] >= R2[b][a]) continue;
			DC[i] = b; DC[i+k] = a;
			print_order(fd,nd,"Step");
		}
	}
	print_order(fd,nd,"Optimized");
	fclose(fd);
}


static int* cur_sort_array = NULL;

static int cmp_dest(const void *p0, const void *p1)	{
	return	cur_sort_array[*(byte*)p1]-
			cur_sort_array[*(byte*)p0];
}

void topology(byte elem, int nd, byte *const stack, byte *const state, byte (*Dest)[0x100])	{
	int j;
	state[elem] = 1;
	for(j=0; j!=nd; ++j)	{
		const byte d = Dest[elem][j];
		if(state[d]) continue;
		topology(d,nd,stack+1,state,Dest);
	}
	state[elem] = 2;
	stack[0] = elem;
}

void optimize_topo(int nd)	{
	static byte Dest[0x100][0x100];
	byte stack[0x100];
	byte state[0x100];
	int i,j;
	memset(stack,0,0x100);
	memset(state,0,0x100);
	for(i=0; i!=nd; ++i)	{
		const int ci = DC[i];
		for(j=0; j!=nd; ++j)
			Dest[ci][j] = DC[j];
		cur_sort_array = R2[ci];
		qsort(Dest[ci],nd,1,cmp_dest);
	}
	topology(DC[0],nd,stack,state,Dest);	//start elem
	memcpy(DC, stack, nd*sizeof(int));
}


void optimize_bubble(int nd)	{
	int i;
	qsort(DC, nd, sizeof(byte), cmp_freq);
	for(;;)	{
		byte x;
		int b0 = -1, b1 = 0;
		for(i=0; i+1<nd; ++i)	{
			const byte c0 = DC[i], c1 = DC[i+1];
			const int cur = R2[c1][c0] - R2[c0][c1];
			if(cur>b1) b0=i,b1=cur;
		}
		if(!b1) break;
		x = DC[b0];
		DC[b0] = DC[b0+1];
		DC[b0+1] = x;
	}
}


void optimize_greedy(int nd)	{
	int i,p0=0,p1=nd;
	int ins[256]={0},ots[256]={0};
	printf("Greedy!\n");
	//step-1: count inputs and outputs to graph nodes
	for(i=0; i!=256; ++i)	{
		int j;
		for(j=0; j!=256; ++j)	{
			const int val = R2[i][j];
			ins[i] += val;
			ots[j] += val;
		}
	}
	while(p0 != p1)	{
		int bestPos = -1, bestVal = 0;
		//step-2a: find best node
		for(i=p0; i!=p1; ++i)	{
			const byte ch = DC[i];
			const int val = ots[ch] - ins[ch];
			if( ins[ch]*ots[ch] == 0 )	{
				bestPos = i;
				break;
			}
			if(bestPos<0 || val>bestVal)	{
				bestPos = i;
				bestVal = val;
			}
		}
		//step-2b: append it either to the start or to the end
		assert( bestPos>=0 );
		bestVal = DC[bestPos];
		if( ots[bestVal] )
			i = --p1;
		else
			i = p0++;
		DC[bestPos] = DC[i];
		DC[i] = bestVal;
		//step-2c: update counters
		for(i=0; i!=256; ++i)	{
			ins[i] -= R2[i][bestVal];
			ots[i] -= R2[bestVal][i];
		}
		printf("%d ", bestVal);
	}
	printf("\n");
}
