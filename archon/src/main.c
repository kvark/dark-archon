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


//	Ordering methods
extern void optimize_none(int);
extern void optimize_freq(int);
extern void optimize_greedy(int);
extern void optimize_matrix(int);
extern void optimize_topo(int);
extern void optimize_bubble(int);

//	Coder methods
extern SymbolCodePtr	coder_base();
extern unsigned			coder_memory();
extern dword			coder_build_encoder(int const *const, byte const *const, const byte);
extern int				coder_encode_stream(dword *const, const byte *const, const int);
extern void				coder_build_decoder();
extern SymbolCodePtr	coder_decode_symbol(const dword);
extern dword			coder_extract_code(byte const* const ptr, const int offset);


//	Internal structures

enum	Verification	{
	VER_NONE,
	VER_IT,
	VER_SORT
}static const VERIFY = VER_SORT;

enum	Constants	{
	COMPARE_BITS		= 32,
};


int R1[0x100] = {0}, BIT;
int R2[0x100][0x100] = {{0}};
byte CD[0x100],DC[0x100];
static byte *source,*s_base;
static const unsigned termin = 10;


//	Key access method	//

static dword get_key_bytes(const int bi)	{
	return *(dword*)(source-4+(bi>>3));
}
static dword get_key_fixed(const int bi)	{
	const byte *const s2 = source + (bi>>3);
	const int bit = bi & 7;
	assert(s2-4 >= s_base);
	return (((dword)s2[0]<<24)<<(8-bit)) | (*(dword*)(s2-4)>>bit);
}
static dword (*const get_key)(const int) = get_key_fixed;

//	Symbol decode method	//

static SymbolCodePtr symbol_decode_fixed(const dword data)	{
	static struct SymbolCode sc;
	sc.code = data & ((1<<BIT)-1);
	sc.length = BIT;
	return &sc;
}

static SymbolCodePtr (*const symbol_decode)(const dword) = coder_decode_symbol;


//	Symbol length on the offset method	//

static byte offset_length_fixed(const int offset)	{
	return BIT;
}
static byte offset_length_uni(const int offset)	{
	const dword code = coder_extract_code(source,offset);
	return coder_decode_symbol(code)->length;
}
static byte (*const offset_length)(const int) = offset_length_uni;


//	Radix helper routine	//

static dword advance_radix(int *const offset, const byte bits)	{
	const int k = *offset + offset_length(*offset);
	const dword us = *(dword*)(source + (k>>3) - 3) << (8-(k&0x7));
	assert( bits <= 24 );
	*offset = k;
	return us >> (32-bits);
	//return get_key(k) >> (32-bits);
}


//	Sorting		//

static byte get_char(const int offset)	{
	const dword us = coder_extract_code(source,offset);
	return coder_decode_symbol(us) - coder_base();
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

// Helper verification routine
// for each symbol find the lowest string starting with it

void prepare_verification(int const *const R, const byte power, int const *const P)	{
	int i;
	for(i=0; i!=0x100; ++i)	{
		struct SymbolCode const* const sc = coder_base() + i;
		int k;
		if(!sc->length)	k = -1;				// skip it
		else if(sc->length <= power)	{ // good
			const byte log = power - sc->length;
			k = R[sc->code << log];
		}else	{ // worst case
			// enumerate the radix group manually
			const byte log = sc->length - power;	// bits to enumerate
			const dword shift_code = sc->code >> log;			// known code
			const dword rest_code = sc->code & ((1<<log)-1);	// code to compare
			// scan the radix group
			//todo: binary search
			for(k=R[shift_code]; k!=R[shift_code+1]; ++k)	{
				const dword key = get_key(P[k]-power);
				if( (key>>(32-log)) == rest_code)
					break;
			}
			assert(k != R[shift_code+1]);
		}
		R1[i] = k;
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
	//{"none",	optimize_none	},
	{"freq",	optimize_freq	},
	{"greedy",	optimize_greedy	},
	{"matrix",	optimize_matrix	},
	{"topo",	optimize_topo	},
	{"bubble",	optimize_bubble	},
	{NULL,		NULL			}
};

struct Options read_command_line(const int ac, const char *av[])	{
	struct Options o = { NULL, NULL, NULL, 10 };
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


//	Main entry point	//

int main(const int argc, const char *argv[])	{
	time_t t0 = 0; FILE *fx = NULL;
	const unsigned SINT = sizeof(int), useItoh = 1;
	int i,N,nd,k,base_id=-1,sorted=0,total_bits = 0;
	int *P,*X,*Y; struct Options opt;
	
	printf("Var BWT: Radix+BeSe\n");
	printf("archon <f_in> <f_out> [-o alphabet_order]\n");
	opt = read_command_line(argc,argv);
	if( !opt.nameIn || !opt.nameOut )	{
		printf("Error: IO undefined\n");
		return -1;
	}
	if( 0 && !opt.fOrder )	{
		printf("Error: unknown order function\nTry one of these: ");
		for(i=0; pFun[i].name; ++i)
			printf("%s%s", i?", ":"", pFun[i].name);
		printf("\n");
		return -2;
	}

	// read input & allocate memory
	fx = fopen( opt.nameIn, "rb" );
	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	fseek(fx,0,SEEK_SET);
	P = (int*)malloc( N*SINT );
	assert( termin>1 && N>0 );
	source = (byte*)P;
	fread(source,1,N,fx);
	fclose(fx); fx = NULL;
	// todo: allocate one large memory block
	X = (int*)malloc( SINT+(SINT<<opt.radPow) );
	Y = (int*)malloc( SINT+(SINT<<opt.radPow) );
	memset( X, 0, SINT<<opt.radPow );
	k = 2*sizeof(CD)+sizeof(R1)+sizeof(R2)+coder_memory() +	//static
		N*SINT + 2*(SINT<<opt.radPow);						//dynamic
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10, k>>10 );

	t0 = clock();
	{	// zero & first order statistics
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

	// optimize
	memset( DC, 0, sizeof(DC) );
	for(i=0; i!=0x100; ++i)
		DC[CD[i]] = i;
	if (opt.fOrder)	{
		opt.fOrder(nd);
		for(i=0; i!=0x100; ++i)
			CD[DC[i]] = i;
	}
	
	total_bits = coder_build_encoder(R1,CD,BIT);
	coder_build_decoder();
	s_base = (byte*)malloc(termin + (total_bits>>3) + 4);
	memset(s_base, -1, termin);
	source = s_base + termin;

	k = coder_encode_stream( (dword*)source, (byte*)P, N );
	assert( k == total_bits );
	
	for(k=0; k<total_bits; )	{
		const dword key = advance_radix( &k, opt.radPow );
		X[key] += 1;
	}
	assert(k == total_bits);
	printf("Symbols (%d) compressed into %.2f bits/sym, using %d Kb of memory.\n",
		nd, total_bits*1.f / N, (termin + (total_bits>>3) + 4)>>10 );

	memset( P, -1, N*SINT );

	{	// radix sort
		dword prev_key = (dword)-1;
		for(i=1<<opt.radPow, k=X[i]=Y[i]=N; i--;)
			X[i] = (k -= X[i]);
		memcpy( Y, X, SINT+(SINT<<opt.radPow) );
		for(i=k=0; i!=N; ++i)	{
			const dword new_key = advance_radix( &k, opt.radPow );
			long diff = (new_key<<1) - prev_key;
			prev_key += diff;
			if( useItoh && diff<0 ) ++prev_key;
			else P[X[new_key]++] = k;
		}
		printf("Radix (%d bits) completed.\n", opt.radPow);
		assert( k == total_bits );
	}

	for(i=0; i!=1<<opt.radPow; ++i)	{
		sort_bese( P+Y[i],P+X[i], opt.radPow );
		sorted += X[i]-Y[i];
	}
	printf("SufSort completed: %.3f sec\n",
		(clock()-t0)*1.f / CLOCKS_PER_SEC );

	if( VERIFY==VER_SORT )
		memcpy(X,Y, SINT<<opt.radPow);

	if(useItoh)	{
		for(i=N; i--; )	{
			int id = P[i];
			assert(id > 0);
			if(id == total_bits)	{
				assert(base_id<0);
				base_id = i;
			}else	{
				const dword key = advance_radix( &id, opt.radPow );
				int *const py = Y+key+1;
				if(*py <= i)	{
					const int to = --*py;
					assert(VERIFY!=VER_IT || X[key] <= to);
					assert(P[to] == -1);
					P[to] = id;
				}
			}
		}
		assert( VERIFY!=VER_IT || !memcmp(X,Y+1, SINT<<opt.radPow) );
		printf("IT-1 completed: %.2f bad elements\n", sorted*1.f/N);
	}

	if(opt.fOrder)	{
		for(i=0; i!=0x100; ++i)
			DC[CD[i]] = i;
	}
	
	if (VERIFY==VER_SORT)
		prepare_verification( X, opt.radPow, P );

	// write the output
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
			const byte length = offset_length(v);
			ch = get_char(v);
			if( VERIFY==VER_SORT )	{
				assert( R1[ch]>=0 );
				if(P[R1[ch]++] != v+length)
					break;
			}
		}
		fputc( ch, fx );
	}
	if( VERIFY==VER_SORT )	{
		printf("Verification: %s\n", (i==N?"OK":"Failed") );
	}
	fclose(fx); fx = NULL;

	// finish like a man
	free(s_base);
	free(P);
	free(X); free(Y);
	printf("Done\n");
	return 0;
}

