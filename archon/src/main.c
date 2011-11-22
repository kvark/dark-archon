/*	main.c
 *	Archon entry point.
*	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <memory.h>
#include <time.h>
#include "common.h"

int R1[0x100] = {0}, BIT;
int R2[0x100][0x100] = {{0}};
byte CD[0x100],DC[0x100];
static byte *source,*s0;

extern void optimize_none(int);
extern void optimize_freq(int);
extern void optimize_greedy(int);
extern void optimize_matrix(int);
extern void optimize_topo(int);
extern void optimize_bubble(int);

struct SymbolCode	{
	dword	code;
	byte	length;
};

//	Key access method	//

static dword get_key_bytes(const int bi)	{
	return *(dword*)(source-4+(bi>>3));
}
static dword get_key_fixed(const int bi)	{
	const byte *const s2 = source + (bi>>3);
	const int bit = bi & 7;
	return (((dword)s2[0]<<24)<<(8-bit)) | (*(dword*)(s2-4)>>bit);
}
static dword (*const get_key)(const int) = get_key_fixed;

//	Symbol length method	//

static int symbol_length_fixed(const byte sym)	{
	return BIT;
}
static int (*const symbol_length)(const byte) = symbol_length_fixed;

//	Symbol decode method	//

static struct SymbolCode symbol_decode_fixed(const dword data)	{
	const struct SymbolCode sc = { data&((1<<BIT)-1), BIT };
	return sc;
}
static struct SymbolCode (*const symbol_decode)(const dword) = symbol_decode_fixed;

//	Symbol length on the offset method	//

static byte offset_length_fixed(const int offset)	{
	return BIT;
}
static byte offset_length_uni(const int offset)	{
	//warning: this needs debugging
	const dword key = get_key(offset);
	const struct SymbolCode sc = symbol_decode_fixed(key);
	return sc.length;
}
static byte (*const offset_length)(const int) = offset_length_fixed;


//	Sorting		//

static byte get_char(const int v)	{
	const dword us = *(word*)(source+(v>>3)) >> (v&7);
	const struct SymbolCode code = symbol_decode(us);
	return DC[ code.code ];
}


static void sort_bese(int *A, int *B, int depth)	{
	while(A+1 < B)	{
		int *x=A,*y=A+1,*z=B;
		const dword U = get_key(A[0] - depth);
		while(y != z)	{
			const int s = *y;
			const dword V = get_key(s - depth);
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


//	Global options	//

struct Options	{
	const char *nameIn,*nameOut;
	void (*fOrder)(int);
	unsigned radPow;
};

struct FunParam	{
	const char *name;
	void (*fun)(int);
}static const 	pFun[] =	{
	{"none",	optimize_none	},
	{"freq",	optimize_freq	},
	{"greedy",	optimize_greedy	},
	{"matrix",	optimize_matrix	},
	{"topo",	optimize_topo	},
	{"bubble",	optimize_bubble	},
	{NULL,		NULL			}
};

struct Options read_command_line(const int ac, const char *av[])	{
	struct Options o = { NULL, NULL, optimize_none, 20 };
	int i,j;
	for(i=1; i!=ac; ++i)	{
		const char *par = av[i];
		if( par[0]=='-' )	{
			assert( !par[2] );
			switch(par[1])	{
			case 'o':
				par = av[++i];
				assert(i!=ac);
				for(j=0; pFun[j].name && strcmp(par,pFun[j].name); ++j);
				o.fOrder = pFun[j].fun;
				break;
			default:
				printf("Unknow parameter: %s\n",par);
			}
		}else
		if(!o.nameIn)	{
			o.nameIn = par;
		}else if(!o.nameOut)	{
			o.nameOut = par;
		}else	{
			printf("Excess argument: %s\n",par);
		}
	}
	return o;
}



int main(const int argc, const char *argv[])	{
	FILE *fx = NULL;
	time_t t0 = 0;
	const unsigned termin = 10, SINT = sizeof(int), useItoh = 2;
	int i,N,nd,k,base_id=-1,sorted=0,total_bits = 0;
	int *P,*X,*Y; struct Options opt;
	
	printf("Var BWT: Radix+BeSe\n");
	printf("archon <f_in> <f_out> [-o alphabet_order]\n");
	opt = read_command_line(argc,argv);
	assert( opt.radPow <= 30 );
	if( !opt.nameIn || !opt.nameOut )	{
		printf("Error: IO undefined\n");
		return -1;
	}
	if( !opt.fOrder )	{
		printf("Error: unknown order function\nTry one of these: ");
		for(i=0; pFun[i].name; ++i)
			printf("%s%s", i?", ":"", pFun[i].name);
		printf("\n");
		return -2;
	}

	fx = fopen( opt.nameIn, "rb" );
	//read input & allocate memory
	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	fseek(fx,0,SEEK_SET);
	assert( termin>1 && N>0 );
	s0 = (byte*)malloc(N+termin);
	memset(s0, -1, termin);
	source = s0 + termin;
	fread(source,1,N,fx);
	fclose(fx);
	P = (int*)malloc( N*SINT );
	memset( P, -1, N*SINT );
	X = (int*)malloc( SINT+(SINT<<opt.radPow) );
	Y = (int*)malloc( SINT+(SINT<<opt.radPow) );
	memset( X, 0, SINT<<opt.radPow );
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10,
		(N*(2+SINT) + (SINT<<16) + 2*(SINT<<opt.radPow) + SINT+termin)>>10 );

	t0 = clock();
	{	//zero & first order statistics
		byte b = 0xFF, c = 0xFF;
		for(i=0; i!=N; ++i)	{
			const byte a = source[i];
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
	total_bits = N*BIT;

	//optimize
	memset( DC, 0, sizeof(DC) );
	for(i=0; i!=0x100; ++i)
		DC[CD[i]] = i;
	if(useItoh == 2)
		opt.fOrder(nd);
	for(i=0; i!=0x100; ++i)
		CD[DC[i]] = i;

	//transform input (k = dest bit index)
	*--source = 0;
	for(i=k=0; i!=N;)	{
		const byte symbol = source[++i];
		const byte length = symbol_length(symbol);	// this one or the previous?
		const byte v = CD[symbol];
		source[k>>3] |= v<<(k&7);
		if(k>>3 != (k+length)>>3)	// next byte
			source[(k>>3)+1] = v>>( 8-(k&7) );
		k += length;
		X[ get_key(k) >> (32-opt.radPow) ] += 1;
	}
	printf("Symbols (%d) compressed (%d bits).\n", nd,BIT);

	{	//radix sort
		dword prev_key = (dword)-1;
		for(i=1<<opt.radPow, k=X[i]=Y[i]=N; i--;)
			X[i] = (k -= X[i]);
		memcpy( Y, X, SINT+(SINT<<opt.radPow) );
		for(i=k=0; (k+=symbol_length(source[i]),1) && ++i!=N; )	{
			dword new_key = get_key(k) >> (32-opt.radPow);
			long diff = (new_key<<1) - prev_key;
			prev_key += diff;
			if( useItoh && diff<0 ) ++prev_key;
			else P[X[new_key]++] = k;
		}
		printf("Radix (%d bits) completed.\n", opt.radPow);
	}

	for(i=0; i!=1<<opt.radPow; ++i)	{
		sort_bese( P+Y[i],P+X[i], opt.radPow );
		sorted += X[i]-Y[i];
	}
	printf("SufSort completed: %.3f sec\n",
		(clock()-t0)*1.f / CLOCKS_PER_SEC );

	if(useItoh)	{
		for(i=N; i--; )	{
			const int prev = P[i];
			assert(prev > 0);
			if(prev == total_bits)	{
				assert(base_id<0);
				base_id = i;
			}else	{
				const int id = prev + offset_length(prev);
				const dword key = get_key(id) >> (32-opt.radPow);
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
		const int next = P[i],
			id = next - offset_length(next);
		//warning: untested!
		R1[ get_char(id) ] = i;
	}
	//write output
	fx = fopen( opt.nameOut, "wb" );
	assert(base_id >= 0);
	fwrite(&base_id,4,1,fx);

	for(i=0; i!=N; ++i)	{
		const int v = P[i];
		byte ch;
		assert(v>0);
		if(v == total_bits)	{
			//todo: optimize it?
			ch = get_char(0);
		}else	{
			const int length = offset_length(v);
			ch = get_char(v);
			//verification
			if(P[R1[ch]++] != v+length) break;
		}
		fputc( ch, fx );
	}
	printf("Verification: %s\n", (i==N?"OK":"Failed") );
	fclose(fx);

	//finish it
	free(s0);
	free(P);
	free(X); free(Y);
	printf("Done\n");
	return 0;
}

