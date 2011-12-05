/*	bwt.c
 *	Barrows-Wheeler transformation routines.
 *	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "bwt.h"
#include "coder.h"
#include "order.h"


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

struct Configuration	{
	dword (*bwt_start)();
	void (*bwt_encode)();
	dword (*get_key)(const int);
	SymbolCodePtr (*decode_symbol)(const dword);
	byte (*offset_length)(const int);
	byte (*get_char)(const int);
	void (*move_right)(int*const);
} static config = {NULL,NULL,NULL,NULL,NULL,NULL,NULL};

//	Coder start	//

static dword bwt_start_byte()	{
	BIT = 8;
	coder_build_encoder_byte();
	return N<<3;
}

static dword bwt_start_fixed()	{
	int nd,i;
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
	printf("Using fixed-bit encoding of %d symbols.\n", nd);
	return coder_build_encoder_fixed(R1,BIT);
}

static dword bwt_start_var()	{
	return coder_build_encoder(R1);
}

//	Encode stream	//

static void bwt_encode_byte()	{
	memcpy(source,P,N);
}

static void bwt_encode_fixed()	{
	const int total = coder_encode_stream( (dword*)source, (byte*)P, N );
	assert( total == total_bits );
	printf("Compressed into %d bits/symbol.\n", BIT);
}

static void bwt_encode_uni()	{
	int total;
	coder_build_decoder();
	total = coder_encode_stream( (dword*)source, (byte*)P, N );
	assert( total == total_bits );
	printf("Compressed into %.2f bits/sym, using %d Kb of memory.\n",
		total_bits*1.f / N, (termin + (total_bits>>3) + 4)>>10 );
}

//	Key access method	//

static dword get_key_byte(const int bi)	{
	return *(dword*)(source-4+(bi>>3));
}
static dword get_key_fixed(const int bi)	{
	const byte *const s2 = source + (bi>>3);
	const int bit = bi & 7;
	assert(s2-4 >= s_base);
	return (((dword)s2[0]<<24)<<(8-bit)) | (*(dword*)(s2-4)>>bit);
}

//	Symbol decode method	//

static SymbolCodePtr decode_symbol_byte(const dword data)	{
	return coder_base() + (data>>24);
}

//	Symbol length on the offset method	//

static byte offset_length_fixed(const int offset)	{
	return BIT;
}
static byte offset_length_var(const int offset)	{
	const dword code = coder_extract_code(source,offset);
	return config.decode_symbol(code)->length;
}

//	Get char for BWT and verification		//

static byte get_char_byte(const int offset)	{
	return source[(offset>>3)-1];
}

static byte get_char_uni(const int offset)	{
	const dword us = config.get_key(offset);
	return config.decode_symbol(us) - coder_base();
}

//	Shift to the character to the right		//

static byte is_border(int bi)	{
	return B[bi>>3] & (1<<(bi&7));
}

static void move_right_fixed(int *const offset)	{
	*offset += BIT;
}

static void move_right_var(int *const offset)	{
	while( !is_border(++*offset) );
}


//------------------------------------------------------------------------------//
//	Bentley-Sedgewick suffix sorting, adapted for bits	//
//------------------------------------------------------------------------------//

static void sort_bese(int *A, int *B, int depth)	{
	while(A+1 < B)	{
		int *x=A,*y=A+1,*z=B;
		const dword U = config.get_key(A[0] - depth);
		while(y != z)	{
			const int s = *y;
			const dword V = config.get_key(s - depth);
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
		if(!R1[i])	k = -1;				// skip it
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
				const dword key = config.get_key(P[k]-rad_bits);
				if( (key>>(32-log)) == rest_code)
					break;
			}
			assert(k != X[shift_code+1]);
		}
		R1[i] = k;
	}
}


// EXTERNALS

//------------------------------------------------------------------------------//
//	Report memory usage
//------------------------------------------------------------------------------//

unsigned bwt_memory()	{
	return coder_memory() + order_memory() +
		Nmax*SINT + sizeof(R1) +
		(X||Y ? 2*(SINT<<rad_bits) : 0)+
		(B ? (Bmax>>2) : 0);
}


//------------------------------------------------------------------------------//
//	Initialize transformator
//------------------------------------------------------------------------------//

void bwt_init(int maxN, byte radix, enum KeyConfig conf_id)	{
	// set key config
	struct Configuration const config_array[] =	{
		{bwt_start_byte,	bwt_encode_byte,	get_key_byte,	decode_symbol_byte,			offset_length_fixed,	get_char_byte,	move_right_fixed	},	//byte
		{bwt_start_fixed,	bwt_encode_fixed,	get_key_fixed,	coder_decode_symbol_fixed,	offset_length_fixed,	get_char_uni,	move_right_fixed	},	//fixed-bit
		{bwt_start_var,		bwt_encode_uni,		get_key_fixed,	coder_decode_symbol,		offset_length_var,		get_char_uni,	move_right_var		},	//var-bit
	};
	config = config_array[conf_id];
	assert(conf_id != KEY_BYTE || !(radix&0x7));
	// allocate memory
	Nmax = maxN; rad_bits = radix;
	Bmax = maxN<<3;
	assert( termin>0 && maxN>0 && maxN<=(1<<30) );
	// todo: allocate one large memory block
	if(conf_id != KEY_UNPACK)	{
		X = (int*)malloc( SINT+(SINT<<rad_bits) );
		Y = (int*)malloc( SINT+(SINT<<rad_bits) );
		memset( X, 0, SINT<<rad_bits );
	}else	{
		// store the destination string
		B = (byte*)malloc(Nmax);
	}
	P = (int*)malloc( (Nmax+1)*SINT );
	s_base = (byte*)malloc( termin+(Bmax>>3)+4 );
	if(conf_id == KEY_VARIABLE)
		B = (byte*)malloc( (Bmax>>3)+1 );
}


//------------------------------------------------------------------------------//
//	Kill transformator
//------------------------------------------------------------------------------//

void bwt_exit()	{
	if(X||Y)	{
		free(X); free(Y);
		X = Y = NULL;
	}
	if(P || s_base)	{
		free(P); free(s_base);
		P = NULL;
		s_base = NULL;
	}
	if(B)	{
		free(B);
		B = NULL;
	}
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
			int v2=0;	//todo: optimize it?
			config.move_right(&v2);
			ch = config.get_char(v2);
		}else	{
			int v2=v;
			config.move_right(&v2);
			ch = config.get_char(v2);
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

//	A useful routine of going right and returning the bucket
static dword advance_radix(int *const bi)	{
	config.move_right(bi);
	assert( rad_bits<=24 );
	return (config.get_key(*bi) >> 8) >> (24-rad_bits);
}

int bwt_transform()	{
	int i, sorted=0;
	assert(N>0 && N<=Nmax);
	order_init(source,N);
	
	memset(R1,0,sizeof(R1));
	for(i=0; i!=N; ++i)
		R1[source[i]] += 1;

	assert( config.bwt_start );
	total_bits = config.bwt_start();
	assert(total_bits <= Bmax);
	
	memset(s_base, -1, termin);
	source = s_base + termin;
	config.bwt_encode();
	memset( P, -1, N*SINT );

	{	// radix sort
		dword prev_key = 2<<rad_bits;
		int k;
		if(B)	{
			memset( B, 0, (total_bits>>3)+1 );
			B[0] = 1;
		}
		// gather frequencies
		for(k=total_bits; k>0; )	{
			const dword key = config.get_key(k);
			const SymbolCodePtr sc = config.decode_symbol(key);
			if(B)	{
				B[k>>3] |= 1<<(k&7);
			}
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
			const int k_old = k;
			const dword new_key = advance_radix(&k);
			long diff = (new_key<<1) - prev_key;
			prev_key += diff;
			assert( rad_bits >= k-k_old );
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

	if( VERIFY==VF_SORT )	{
		memcpy(X,Y, SINT<<rad_bits);
	}

	if(useItoh)	{
		P[N] = 0;
		for(i=N; i>=0; --i)	{
			int id = P[i];
			assert(id>=0 && id<=total_bits);
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


//------------------------------------------------------------------------------//
//	Perform the reverse BWT
//------------------------------------------------------------------------------//

int unbwt_read(FILE *const fx)	{
	source = s_base;
	if(!fread( &base_id, sizeof(base_id), 1, fx ))
		return -1;
	N = fread(source,1,Nmax,fx);
	return N;
}

static void roll(const int i)	{
	P[i] = R1[source[i]]++;
	assert(N==1 || i != P[i]);
}

void unbwt_transform()	{
	int i,k;
	memset( R1, 0, sizeof(R1) );
	memset( P, -1, (N+1)*SINT );		//for debug
	for(i=0; i!=N; ++i)
		R1[source[i]] += 1;
	for(k=N,i=0x100; i--;)	{
		R1[i] = (k-=R1[i]);
	}
	for(i=0; i<base_id; i++)	roll(i);
	for(i=base_id+1; i<N; i++)	roll(i);
	roll(base_id);
}

void unbwt_write(FILE *const fx)	{
	int i,k;
	for(i=0,k=base_id; i!=N; ++i,k=P[k])
		putc(source[k],fx);
	assert(k == base_id);
}

//------------------------------------------------------------------------------//
//	Perform the optimized reverse BWT
//------------------------------------------------------------------------------//

void unbwt_transform_fast()	{
	int i, k=base_id, len=0, k_start=-1;
	if(!N) return;
	unbwt_transform();
	// now be smart!
	source[P[N]=N] = ~source[N-1];
	for(i=0; i!=N; )	{
		const int k_next = P[k];
		if(k_next<0)	{ // jump!
			const int s_pos = -k_next-1;
			const int dest = P[k-1];
			assert(dest>=0);
			len = P[dest];	//todo: cut the chain if jumping
			assert(len>0 && s_pos<i && i+len<=N);
			memcpy(B+i,B+s_pos,len);
			k_start = k = dest + 1;
			i+=len; len=0;
		}else	{
			B[i] = source[k];
			if(source[k] != source[k+1])	{
				if(len>1)	{
					assert(k_start>=N-2 || P[k_start+2]>=0);
					P[k_start] = k;
					P[k_start+1] = -(i-len)-1;
					assert(k>=N-1 || P[k+1]>=0);
					P[k] = len;
				}
				len = 0;
				k_start = k_next;
			}else	{
				//assert(P[k]+1 == P[k+1]);
				++len;
			}
			k=k_next; ++i;
		}
	}
	assert(k == base_id);
}

void unbwt_write_fast(FILE *const fx)	{
	fwrite(B,1,N,fx);
}