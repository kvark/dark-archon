#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <memory.h>

typedef unsigned char byte;

int main(const int argc, const char *argv[])	{
	FILE *fx = stdin;
	int i,N,nd,ord,bits,key,off;
	byte *s; int *I,*X;
	int R[256] = {0};
	const int radPow = 21;
	byte CD[256];
	printf("Var-len radix (Pow2)\n");

	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	fseek(fx,0,SEEK_SET);
	s = (byte*)malloc(N);
	fread(s,1,N,fx);
	fclose(fx);
	I = (int*)malloc( N*sizeof(int) );
	memset( I, -1, N*sizeof(int) );
	X = (int*)malloc( sizeof(int)<<radPow );
	memset( X, 0, sizeof(int)<<radPow );
	printf("Loaded: %d bytes\n",N);
	
	for(i=0; i!=N; ++i)
		R[s[i]] += 1;
	CD[0] = 0;
	for(i=1; i!=256; ++i)	{
		byte d = CD[i-1] + (R[i-1] != 0); 
		CD[i] = d;
	}
	nd = CD[255];
	for(bits=0; (1<<bits)<nd; ++bits);
	ord = radPow / bits;
	assert(ord>0);
	off = bits * (ord-1);
	printf("Symbols: %d, Order: %d\n",nd,ord);
	
	for(key=0,i=0; i!=N; ++i)	{
		key = (key >> bits) + (CD[s[i]] << off);
		X[key] += 1;
	}
	for(key=N,i=1<<(bits*ord); i--;)
		key = X[i] = key-X[i];
	for(key=0,i=0; i!=N; ++i)	{
		key = (key >> bits) + (CD[s[i]] << off);
		I[ X[key]++ ] = i;
	}
	for(i=0; i!=N; ++i)
		printf("%d,",I[i]);
	
	free(s); free(I); free(X);
	printf("\nDone\n");
	return 0;
}
