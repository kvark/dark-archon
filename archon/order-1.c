#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <memory.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

static int R[256] = {0}, B;
static byte CD[256],*s;

int r_cmp(const void *a, const void *b)	{
	return R[(byte*)a - CD] - R[(byte*)b - CD];
}

dword key(const int bi)	{
	const byte *const s2 = s + (bi>>3);
	const int bit = bi & 7;
	return (s2[0]<<(32-bit)) | (*(dword*)(s2-4)>>bit);
}

void sort_bese(int *A, int *B, int depth)	{
	while(A+1 < B)	{
		int *x=A;
	}
}


int main(const int argc, const char *argv[])	{
	FILE *fx = NULL;
	const int radPow = 20, termin = 10,
		radMask = (1<<radPow)-1, SINT =  sizeof(int);
	int i,N,nd,BM,k,source=-1;
	int *I,*X;
	printf("Var BWT: Radix+BeSe\n");
	if(argc != 3) return 1-1;

	//read input & allocate memory
	fx = fopen(argv[1],"rb");
	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	fseek(fx,0,SEEK_SET);
	assert( termin>1 && N>0 );
	s = (byte*)malloc(N+termin) + termin;
	memset(s-termin, -1, termin);
	fread(s,1,N,fx);
	fclose(fx);
	I = (int*)malloc( N*SINT );
	memset( I, -1, N*SINT );
	X = (int*)malloc( SINT+(SINT<<radPow) );
	memset( X, 0, SINT<<radPow );
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10,
		(N*(1+SINT) + (SINT<<radPow) + SINT+termin)>>10 );
	
	//zero order statistics
	for(i=0; i!=N; ++i)
		R[s[i]] += 1;
	for(CD[0]=i=0; i!=0xFF; ++i) 
		CD[i+1] = CD[i] + (R[i] ? 1:0);
	nd = CD[0xFF] + (R[0xFF] ? 1:0);	//int here!
	qsort(CD, 0x100,1, r_cmp);	//reorder most frequent -> least
	for(B=0; (1<<B)<nd; ++B);
	
	//transform input (k = dest bit index)
	*--s = 0;
	for(i=0,k=0; i!=N; k+=B)	{
		byte v = CD[s[++i]];
		s[k>>3] |= v<<(k&7);
		X[ key(k) & radMask ] += 1;
		if(k>>3 == (k+B)>>3) continue;
		s[(k>>3)+1] = v>>( 8-(k&7) );
	}
	printf("Symbols (%d) compressed (%d bits).\n", nd,B);
	
	for(i=1<<radPow,k=X[i]=N; i--;)
		X[i] = (k -= X[i]);
	for(i=0; i!=N; ++i)
		I[X[ key(i*B) & radMask ]++] = i;
	printf("Radix (%d bits) completed.\n", radPow);

	for(i=0; i!=1<<radPow; ++i)	{
		sort_bese(I+X[i],I+X[i+1],radPow);
	}
	printf("BeSe completed.\n");
	
	byte DC[256]; BM = (1<<B)-1;
	for(i=0; i!=256; ++i)
		DC[CD[i]] = i;
	fx = fopen(argv[2],"wb");
	for(i=0; i!=N; ++i)	{
		int v = (I[i]-1)*B;
		if(v<0)	{
			assert(source<0);
			source = i;
			v = (N-1)*B;
		}
		const int us = *(word*)(s+(v>>3));
		const int d = (us >> (v&7)) & BM;
		fputc(DC[d],fx);
	}
	assert(source >= 0);
	fwrite(&source,4,1,fx);
	fclose(fx);
	printf("Output written (source id %d)\n", source);
	
	//finish it
	free(s+1-termin);
	free(I); free(X);
	printf("Done\n");
	return 0;
}

