#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <memory.h>
#include <time.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

static int R[256] = {0}, BIT;
static byte CD[256],DC[256],*s;

int r_cmp(const void *a, const void *b)	{
	return R[(byte*)a - CD] - R[(byte*)b - CD];
}

dword get_key(const int bi)	{
	const byte *const s2 = s + (bi>>3);
	const int bit = bi & 7;
	return (((dword)s2[0]<<24)<<(8-bit)) | (*(dword*)(s2-4)>>bit);
}
byte get_char(const int id)	{
	int v = id*BIT;
	const word us = *(word*)(s+(v>>3));
	const word d = (us >> (v&7)) & ((1<<BIT)-1);
	return DC[d];
}


void sort_bese(int *A, int *B, int depth)	{
	while(A+1 < B)	{
		int *x=A,*y=A+1,*z=B;
		const dword U = get_key(A[0]*BIT - depth);
		while(y != z)	{
			const int s = *y;
			const dword V = get_key(s*BIT - depth);
			if(V<U)	{
				*y++ = *x; *x++ = s;
			}else if(V>U)	{
				*y = *--z; *z = s; 
			}else y++;
		}
		sort_bese(A,x,depth);
		sort_bese(z,B,depth);
		A = x; B = z; depth += 32;
	}
}



int main(const int argc, const char *argv[])	{
	FILE *fx = NULL;
	time_t t0 = 0;
	const int radPow = 20, termin = 10,
		SINT = sizeof(int);
	int i,N,nd,k,source=-1;
	int *I,*X;
	printf("Var BWT: Radix+BeSe\n");
	if(argc != 3) return -1;

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
	X = (int*)malloc( SINT+(SINT<<radPow) )+1;
	memset( X, 0, SINT<<radPow );
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10,
		(N*(1+SINT) + (SINT<<radPow) + SINT+termin)>>10 );
	
	t0 = clock();
	//zero order statistics
	for(i=0; i!=N; ++i)
		R[s[i]] += 1;
	for(CD[0]=i=0; i!=0xFF; ++i) 
		CD[i+1] = CD[i] + (R[i] ? 1:0);
	nd = CD[0xFF] + (R[0xFF] ? 1:0);	//int here => no overflow
	//qsort(CD, 0x100,1, r_cmp);	//reorder most frequent -> least
	for(BIT=0; (1<<BIT)<nd; ++BIT);
	
	//transform input (k = dest bit index)
	*--s = 0;
	for(i=0,k=0; i!=N;)	{
		byte v = CD[s[++i]];
		s[k>>3] |= v<<(k&7);
		if(k>>3 != (k+BIT)>>3)
			s[(k>>3)+1] = v>>( 8-(k&7) );
		k += BIT;
		X[ get_key(k) >> (32-radPow) ] += 1;
	}
	printf("Symbols (%d) compressed (%d bits).\n", nd,BIT);
	
	for(i=1<<radPow,k=N; i--;)
		X[i] = (k -= X[i]);
	for(i=0; ++i<=N; )
		I[X[ get_key(i*BIT) >> (32-radPow) ]++] = i;
	printf("Radix (%d bits) completed.\n", radPow);

	for(X[-1]=0,i=0; i!=1<<radPow; ++i)	{
		sort_bese(I+X[i-1],I+X[i],radPow);
	}
	printf("SufSort completed: %.3f sec\n",
		(clock()-t0)*1.f / CLOCKS_PER_SEC );
	
	for(i=0; i!=256; ++i)
		DC[CD[i]] = i;
	//prepare verification
	memset(R,0,sizeof(R));
	for(i=N; i--;)	{
		R[ get_char(I[i]-1) ] = i;
	}
	//write output
	fx = fopen(argv[2],"wb");
	for(i=0; i!=N; ++i)	{
		int v = I[i]; byte ch;
		if(v == N)	{
			assert(source<0);
			source = i;
			ch = get_char(0);
			//R[ ch ] += 1;
		}else	{
			ch = get_char(v);
			if(I[R[ch]++] != 1+v) break;
		}
		fputc( ch, fx );
	}
	printf("Verification: %s\n", (i==N?"OK":"Failed") );
	assert(source >= 0);
	fwrite(&source,4,1,fx);
	fclose(fx);
	
	//finish it
	free(s+1-termin);
	free(I); free(X-1);
	printf("Done\n");
	return 0;
}

