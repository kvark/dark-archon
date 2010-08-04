#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <memory.h>
#include <time.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

static int R1[0x100] = {0}, BIT;
static int R2[0x100][0x100] = {0};
static byte CD[0x100],DC[0x100],*s;

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

void optimize(int nd)	{
	int i,k;
	//the algorithm is not correct! :(
	FILE *const fd = fopen("matrix.txt","w");
	for(i=0; i!=0x100; ++i)
		DC[CD[i]] = i;
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
	for(k=0; k!=nd; ++k)
		CD[DC[k]] = k;
}

static int* cur_sort_array = NULL;

int cmp_dest(const void *p0, const void *p1)	{
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
	int i,j,k=0;
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
	for(i=0; i!=nd; ++i)
		CD[stack[i]] = i;
}



int main(const int argc, const char *argv[])	{
	FILE *fx = NULL;
	time_t t0 = 0;
	const unsigned radPow = 20, termin = 10,
		SINT = sizeof(int), useItoh = 2;
	int i,N,nd,k,source=-1,sorted=0;
	int *P,*X,*Y;
	printf("Var BWT: Radix+BeSe\n");
	assert(radPow <= 30);
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
	P = (int*)malloc( N*SINT );
	memset( P, -1, N*SINT );
	X = (int*)malloc( SINT+(SINT<<radPow) );
	Y = (int*)malloc( SINT+(SINT<<radPow) );
	memset( X, 0, SINT<<radPow );
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10,
		(N*(2+SINT) + (SINT<<16) + 2*(SINT<<radPow) + SINT+termin)>>10 );
	
	t0 = clock();
	{	//zero & first order statistics
		byte b = 0xFF, c = 0xFF;
		for(i=0; i!=N; ++i)	{
			const byte a = s[i];
			R1[a] += 1;
			if(a!=b) { c=b; b=a; }
			R2[a][c] += 1;
		}
	}
	for(CD[0]=i=0; i!=0xFF; ++i) 
		CD[i+1] = CD[i] + (R1[i]? 1:0);
	nd = CD[0xFF] + (R1[0xFF] ? 1:0);	//int here => no overflow
	assert( nd>0 );
	for(BIT=0; (1<<BIT)<nd; ++BIT);

	//optimize
	memset( DC, 0, sizeof(DC) );
	for(i=0; i!=0x100; ++i)
		DC[CD[i]] = i;
	//if(useItoh == 2) optimize_topo(nd);
	if(useItoh == 2) optimize(nd);
	
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
		
	{	//radix sort
		dword prev_key = (dword)-1;
		for(i=1<<radPow,k=X[i]=Y[i]=N; i--;)
			X[i] = (k -= X[i]);
		memcpy( Y, X, SINT+(SINT<<radPow) );
		for(i=0; ++i<=N; )	{
			dword new_key = get_key(i*BIT) >> (32-radPow);
			long diff = (new_key<<1) - prev_key;
			prev_key += diff;
			if( useItoh && diff<0 ) ++prev_key;
			else P[X[new_key]++] = i;
		}
		printf("Radix (%d bits) completed.\n", radPow);
	}

	for(i=0; i!=1<<radPow; ++i)	{
		sort_bese(P+Y[i],P+X[i],radPow);
		sorted += X[i]-Y[i];
	}
	printf("SufSort completed: %.3f sec\n",
		(clock()-t0)*1.f / CLOCKS_PER_SEC, sorted*1.f/N );
	
	if(useItoh)	{
		for(i=N; i--; )	{
			const int id = P[i]+1;
			assert(id > 0);
			if(id > N)	{
				assert(source<0);
				source = i;
			}else	{
				const dword key = get_key(id*BIT) >> (32-radPow);
				if(Y[key+1] <= i)	{
					const int to = --Y[key+1];
					assert(X[key] <= to);
					assert(P[to] == -1);
					P[to] = id;
				}
			}
		}
		printf("IT-1 completed: %.2f bad elements\n", sorted*1.f/N);
	}
	
	if(useItoh != 2)	{
		for(i=0; i!=256; ++i)
			DC[CD[i]] = i;
	}
	//prepare verification
	memset(R1,0,sizeof(R1));
	for(i=N; i--;)	{
		R1[ get_char(P[i]-1) ] = i;
	}
	//write output
	fx = fopen(argv[2],"wb");
	assert(source >= 0);
	fwrite(&source,4,1,fx);

	for(i=0; i!=N; ++i)	{
		int v = P[i]; byte ch;
		assert(v>0);
		if(v == N)	{
			//todo: optimize it?
			ch = get_char(0);
		}else	{
			ch = get_char(v);
			if(P[R1[ch]++] != 1+v) break;
		}
		fputc( ch, fx );
	}
	printf("Verification: %s\n", (i==N?"OK":"Failed") );
	fclose(fx);
	
	//finish it
	free(s+1-termin);
	free(P);
	free(X); free(Y);
	printf("Done\n");
	return 0;
}

