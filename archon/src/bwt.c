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
extern dword			coder_build_encoder_fixed(int const *const, const byte);
extern dword			coder_build_encoder(int const *const);
extern int				coder_encode_stream(dword *const, const byte *const, const int);
extern void				coder_build_decoder();
extern SymbolCodePtr	coder_decode_symbol(const dword);
extern dword			coder_extract_code(byte const* const ptr, const int offset);

//	Order methods
extern void		order_init(byte const* const, const int);
extern unsigned	order_memory();


// TYPES AND DATA

enum	Verification	{
	VF_NONE,
	VF_IT,
	VF_SORT
}static const VERIFY = VF_SORT;


enum	Constants	{
	COMPARE_BITS		= 32,
	LENGTH_BITS			= 5,
	LENGTH_OFFSET		= 32-LENGTH_BITS,
	INDEX_MASK			= (1<<LENGTH_OFFSET)-1,
};


static int R1[0x100], BIT=0;
static byte *source=NULL, *s_base=NULL, *B=NULL;

static const unsigned SINT = sizeof(int), useItoh=1, termin=10;
static int N=0, Nmax=0, Bmax=0, base_id=-1, total_bits=0;
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


//	Sorting		//

static byte get_char(const int offset)	{
	//const dword us = coder_extract_code(source,offset);
	const dword us = get_key(offset);
	return coder_decode_symbol(us) - coder_base();
}


//------------------------------------------------------------------------------//
//	Bentley-Sedgewick suffix sorting, adapted for bits	//
//------------------------------------------------------------------------------//

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
		A = x; B = z;
		depth += COMPARE_BITS;
	}
}


//------------------------------------------------------------------------------//
//	Helper verification routine
// for each symbol finds the lowest string starting with it
//------------------------------------------------------------------------------//

static void prepare_verification()	{
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

static byte is_border(int bi)	{
	return B[bi>>3] & (1<<(bi&7));
}

static dword advance_radix(int *const bi)	{
	while( !is_border(++*bi) );
	assert( rad_bits<=24 );
	return (get_key(*bi) >> 8) >> (24-rad_bits);
}


// EXTERNALS

//------------------------------------------------------------------------------//
//	Report memory usage
//------------------------------------------------------------------------------//

int bwt_memory()	{
	return coder_memory() + order_memory() +
		Nmax*SINT + 2*(SINT<<rad_bits) + sizeof(R1) + (Bmax>>2);
}


//------------------------------------------------------------------------------//
//	Initialize transformator
//------------------------------------------------------------------------------//

void bwt_init(int maxN, byte radix)	{
	Nmax = maxN; rad_bits = radix;
	Bmax = maxN<<3;
	assert( termin>0 && maxN>0 && maxN<=(1<<30) );
	// todo: allocate one large memory block
	X = (int*)malloc( SINT+(SINT<<rad_bits) );
	Y = (int*)malloc( SINT+(SINT<<rad_bits) );
	memset( X, 0, SINT<<rad_bits );
	P = (int*)malloc( Nmax*SINT );
	s_base = (byte*)malloc( termin+(Bmax>>3)+4 );
	B = (byte*)malloc( (Bmax>>3)+1 );
}


//------------------------------------------------------------------------------//
//	Kill transformator
//------------------------------------------------------------------------------//

void bwt_exit()	{
	assert(s_base && P && X && Y && B);
	free(X); free(Y);
	free(P);
	free(s_base); free(B);
}


//------------------------------------------------------------------------------//
//	Read input stream
//------------------------------------------------------------------------------//

int bwt_read(FILE *const fx)	{
	source = (byte*)P;
	N = fread(source,1,Nmax,fx);
	return N;
}


//------------------------------------------------------------------------------//
//	Write transformation result into the output stream
// perform additional suffix verification
//------------------------------------------------------------------------------//

void bwt_write(FILE *const fx)	{
	int i;
	assert(N>0 && N<=Nmax);
	if (VERIFY==VF_SORT)
		prepare_verification();

	assert(base_id >= 0);
	fwrite(&base_id,4,1,fx);
	
	for(i=0; i!=N; ++i)	{
		const int v = P[i];
		byte ch=0;
		assert(v>0);
		if(v == total_bits)	{
			int v2=0;
			advance_radix(&v2);	// we just need the offset
			//todo: optimize it?
			ch = get_char(v2);
		}else	{
			int v2=v;
			advance_radix(&v2);	// we just need the offset
			ch = get_char(v2);
			if( VERIFY==VF_SORT )	{
				assert( R1[ch]>=0 );
				if(P[R1[ch]++] != v2)
					break;
			}
		}
		fputc( ch, fx );
	}
	if( VERIFY==VF_SORT )	{
			printf("Verification: %s\n", (i==N?"OK":"Failed") );
	}
}


//------------------------------------------------------------------------------//
//	Perform the Burrows-Wheeler transformation
//------------------------------------------------------------------------------//

int bwt_transform()	{
	const int is_fixed = 0;
	int i, sorted=0;
	assert(N>0 && N<=Nmax);
	order_init(source,N);
	
	memset(R1,0,sizeof(R1));
	for(i=0; i!=N; ++i)
		R1[source[i]] += 1;

	if(is_fixed)	{
		int nd;
		for(nd=i=0; i!=0x100; ++i)
			nd += (R1[i]!=0);
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
		printf("Using fixed-bit encoding of %d symbols\n", nd);
		total_bits = coder_build_encoder_fixed(R1,BIT);
	}else	{
		total_bits = coder_build_encoder(R1);
	}

	assert(total_bits <= Bmax);
	coder_build_decoder();
	memset(s_base, -1, termin);
	source = s_base + termin;

	i = coder_encode_stream( (dword*)source, (byte*)P, N );
	assert( i == total_bits );
	printf("Compressed into %.2f bits/sym, using %d Kb of memory.\n",
		total_bits*1.f / N, (termin + (total_bits>>3) + 4)>>10 );

	memset( P, -1, N*SINT );

	{	// radix sort
		dword prev_key = 1<<rad_bits;
		int k;
		memset( B, 0, (total_bits>>3)+1 );
		B[0] = 1;
		// gather frequencies
		for(k=total_bits; k>0; )	{
			const dword key = get_key(k);
			const SymbolCodePtr sc = coder_decode_symbol(key);
			B[k>>3] |= 1<<(k&7);
			k -= sc->length;
			X[(key>>8) >> (24-rad_bits)] += 1;
		}
		assert(!k);
		// compute offsets
		for(i=1<<rad_bits, k=X[i]=Y[i]=N; i--;)
			X[i] = (k -= X[i]);
		assert(!k);
		memcpy( Y, X, SINT+(SINT<<rad_bits) );
		// fill bad suffixes
		for(k=0; k<total_bits;)	{
			const dword new_key = advance_radix(&k);
			long diff = (new_key<<1) - prev_key;
			prev_key += diff;
			if( useItoh && diff<0 ) ++prev_key;
			else P[X[new_key]++] = k;
		}
		assert(k==total_bits);
		printf("Radix (%d bits) completed.\n", rad_bits);
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
				const dword key = advance_radix( &id );
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
	}else	{
		for(i=0; i!=N && P[i]!=total_bits; ++i);
		base_id = i;
	}
	return base_id;
}
