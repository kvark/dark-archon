#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>

enum	{
	SIZE = 1<<15
};

static unsigned char src[SIZE],dst[SIZE];
static int R[0x100], rb[0x101] = {0};


int main()	{
	time_t t; unsigned i,j;
	const unsigned K = 1<<15;
	printf("Started\n");
	for(i=0; i!=SIZE; ++i)	{
		++rb[src[i] = i*5423];
	}
	for(rb[i=0x100]=j=SIZE; i--;)	{
		rb[i] = (j -= rb[i]);
	}
	// iteration +
	printf("Iter-1\n");
	t = clock();
	for(j=0; j!=K; ++j)	{
		memcpy( R, rb+0, 0x100*sizeof(int) );
		for(i=0; i!=SIZE; ++i)
			dst[R[src[i]]++] = src[i];
	}
	t = clock() - t;
	printf("%.2f\n",t*1.f/CLOCKS_PER_SEC);
	// iteration -
	printf("Iter-2\n");
	t = clock();
	for(j=0; j!=K; ++j)	{
		memcpy( R, rb+1, 0x100*sizeof(int) );
		i=SIZE; do	{
			dst[--R[src[i]]] = src[i];
		}while(--i);
	}
	t = clock() - t;
	printf("%.2f\n",t*1.f/CLOCKS_PER_SEC);
	// iteration ++
	printf("Iter-3\n");
	t = clock();
	for(j=0; j!=K; ++j)	{
		memcpy( R, rb+0, 0x100*sizeof(int) );
		for(i=0; i!=0x100; ++i)
			--R[i];
		for(i=0; i!=SIZE; ++i)
			dst[++R[src[i]]] = src[i];
	}
	t = clock() - t;
	printf("%.2f\n",t*1.f/CLOCKS_PER_SEC);
	return 0;
}
