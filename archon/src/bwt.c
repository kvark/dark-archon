/*	bwt.c
 *	Barrows-Wheeler transformation routines.
 *	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"


//	Coder methods
extern SymbolCodePtr	coder_base();
extern unsigned			coder_memory();
extern dword			coder_build_encoder(int const *const, byte const *const, const byte);
extern int				coder_encode_stream(dword *const, const byte *const, const int);
extern void				coder_build_decoder();
extern SymbolCodePtr	coder_decode_symbol(const dword);
extern dword			coder_extract_code(byte const* const ptr, const int offset);

//	Order methods
extern void		order_init();
extern unsigned	order_memory();


// TYPES AND DATA

enum	Verification	{
	VF_NONE,
	VF_IT,
	VF_SORT
}static const VERIFY = VF_SORT;


enum	Constants	{
	COMPARE_BITS		= 32,
};


static int R1[0x100], BIT=0;
static byte CD[0x100];
static byte *source, *s_base;

static const unsigned SINT = sizeof(int), useItoh=1, termin=10;
static int N=0, Nmax=0, base_id=-1, total_bits=0;
static byte rad_bits;
static int *P=NULL,*X=NULL,*Y=NULL;


// INTERNALS

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

void prepare_verification()	{
	int i;
	for(i=0; i!=0x100; ++i)	{
		struct SymbolCode const* const sc = coder_base() + i;
		int k;
		if(!sc->length)	k = -1;				// skip it
		else if(sc->length <= rad_bits)	{ // good
			const byte log = rad_bits - sc->length;
			k = X[sc->code << log];
		}else	{ // worst case
			// enumerate the radix group manually
			const byte log = sc->length - rad_bits;	// bits to enumerate
			const dword shift_code = sc->code >> log;			// known code
			const dword rest_code = sc->code & ((1<<log)-1);	// code to compare
			// scan the radix group
			//todo: binary search
			for(k=X[shift_code]; k!=X[shift_code+1]; ++k)	{
				const dword key = get_key(P[k]-rad_bits);
				if( (key>>(32-log)) == rest_code)
					break;
			}
			assert(k != X[shift_code+1]);
		}
		R1[i] = k;
	}
}


// EXTERNALS

int bwt_memory()	{
	return coder_memory() + order_memory() +
		N*SINT + 2*(SINT<<rad_bits) + sizeof(CD)+sizeof(R1);
}


void bwt_init(int maxN, byte radix)	{
	Nmax = maxN; rad_bits = radix;
	assert( termin>0 && maxN>0 && maxN<=(1<<30) );
	// todo: allocate one large memory block
	X = (int*)malloc( SINT+(SINT<<rad_bits) );
	Y = (int*)malloc( SINT+(SINT<<rad_bits) );
	memset( X, 0, SINT<<rad_bits );
	P = (int*)malloc( Nmax*SINT );
}

void bwt_exit()	{
	assert(!s_base && P && X && Y);
	free(P);
	free(X); free(Y);
}


int bwt_read(FILE *const fx)	{
	source = (byte*)P;
	N = fread(source,1,Nmax,fx);
	return N;
}


void bwt_write(FILE *const fx)	{
	int i;
	assert(N>0);
	if (VERIFY==VF_SORT)
		prepare_verification();

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
			if( VERIFY==VF_SORT )	{
				assert( R1[ch]>=0 );
				if(P[R1[ch]++] != v+length)
					break;
			}
		}
		fputc( ch, fx );
	}
	if( VERIFY==VF_SORT )	{
			printf("Verification: %s\n", (i==N?"OK":"Failed") );
	}
	free(s_base);
	s_base = NULL;
}


int bwt_transform()	{
	int i, nd=0, sorted=0;
	assert(N>0);
	order_init(source,N);
	
	memset(R1,0,sizeof(R1));
	for(i=0; i!=N; ++i)
		R1[source[i]] += 1;

	for(CD[0]=i=0; i!=0xFF; ++i)
		CD[i+1] = CD[i] + (R1[i]? 1:0);
	nd = CD[0xFF] + (R1[0xFF] ? 1:0);	//int here => no overflow
	assert( nd>0 );
	for(BIT=0; (1<<BIT)<nd; ++BIT);

	// optimize
	/*memset( DC, 0, sizeof(DC) );
	for(i=0; i!=0x100; ++i)
		DC[CD[i]] = i;
	if (opt.fOrder)	{
		opt.fOrder(nd);
		for(i=0; i!=0x100; ++i)
			CD[DC[i]] = i;
	}*/
	
	total_bits = coder_build_encoder(R1,CD,BIT);
	coder_build_decoder();
	assert(!s_base);
	s_base = (byte*)malloc(termin + (total_bits>>3) + 4);
	memset(s_base, -1, termin);
	source = s_base + termin;

	i = coder_encode_stream( (dword*)source, (byte*)P, N );
	assert( i == total_bits );
	
	for(i=0; i<total_bits; )	{
		const dword key = advance_radix( &i, rad_bits );
		X[key] += 1;
	}
	assert(i == total_bits);
	printf("Symbols (%d) compressed into %.2f bits/sym, using %d Kb of memory.\n",
		nd, total_bits*1.f / N, (termin + (total_bits>>3) + 4)>>10 );

	memset( P, -1, N*SINT );

	{	// radix sort
		dword prev_key = (dword)-1;
		int k;
		for(i=1<<rad_bits, k=X[i]=Y[i]=N; i--;)
			X[i] = (k -= X[i]);
		memcpy( Y, X, SINT+(SINT<<rad_bits) );
		for(i=k=0; i!=N; ++i)	{
			const dword new_key = advance_radix( &k, rad_bits );
			long diff = (new_key<<1) - prev_key;
			prev_key += diff;
			if( useItoh && diff<0 ) ++prev_key;
			else P[X[new_key]++] = k;
		}
		printf("Radix (%d bits) completed.\n", rad_bits);
		assert( k == total_bits );
	}

	for(i=0; i!=1<<rad_bits; ++i)	{
		sort_bese( P+Y[i],P+X[i], rad_bits );
		sorted += X[i]-Y[i];
	}

	if( VERIFY==VF_SORT )
		memcpy(X,Y, SINT<<rad_bits);

	if(useItoh)	{
		for(i=N; i--; )	{
			int id = P[i];
			assert(id > 0);
			if(id == total_bits)	{
				assert(base_id<0);
				base_id = i;
			}else	{
				const dword key = advance_radix( &id, rad_bits );
				int *const py = Y+key+1;
				if(*py <= i)	{
					const int to = --*py;
					assert(VERIFY!=VF_IT || X[key] <= to);
					assert(P[to] == -1);
					P[to] = id;
				}
			}
		}
		assert( VERIFY!=VF_IT || !memcmp(X,Y+1, SINT<<rad_bits) );
		printf("IT-1 completed: %.2f bad elements\n", sorted*1.f/N);
	}

	/*if(opt.fOrder)	{
		for(i=0; i!=0x100; ++i)
			DC[CD[i]] = i;
	}*/
	return base_id;
}