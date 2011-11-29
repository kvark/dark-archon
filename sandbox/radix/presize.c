#include <stdio.h>
#include <malloc.h>

typedef unsigned char byte;

int main(const int argc, const char *argv[])	{
	FILE *fx = stdin;
	int i,N,nd,ord,total,key;
	byte *s; int *I;
	int R[256] = {0}, X[1<<20] = {0};
	const int maxRad = sizeof(X)/sizeof(int);
	byte CD[256],DC[256];
	printf("Var-len radix\n");

	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	fseek(fx,0,SEEK_SET);
	s = (byte*)malloc(N);
	I = (int*)malloc(N*sizeof(N));
	fread(s,1,N,fx);
	fclose(fx);
	printf("Loaded: %d bytes\n",N);
	
	for(i=0; i!=N; ++i)
		R[s[i]] += 1;
	CD[0] = DC[0] = 0;
	for(i=1; i!=256; ++i)	{
		byte d = CD[i-1] + (R[i-1] != 0); 
		CD[i] = d; DC[d] = i;
	}
	nd = CD[255];
	for(i=1,ord=0; i<=maxRad; i*=nd,++ord);
	total = i/(nd*nd);
	printf("Symbols: %d, Order: %d\n",nd,ord);
	
	for(key=0,i=0; i!=N; ++i)	{
		key = key/nd + total*CD[s[i]];
		X[key] += 1;
		I[i] = -1;
	}
	for(key=N,i=total*nd; i--;)
		key = X[i] = key-X[i];
	for(key=0,i=0; i!=N; ++i)	{
		key = key/nd + total*CD[s[i]];
		I[ X[key]++ ] = i;
	}
	for(i=0; i!=N; ++i)
		printf("%d,",I[i]);

	printf("\nDone\n");
	return 0;
}
