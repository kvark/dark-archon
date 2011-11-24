/*	coder.c
 *	Stream encoding/decoding routines.
 *	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include "common.h"


enum	Constants	{
	MAX_CODE_LENGTH		= 32,
	RANDOM_LENGHT_TEST	= 0,
	DECODE_BITS			= 12,
};

static word	dec_offset[1+(1<<DECODE_BITS)];
static byte	dec_list[0x100 + (1<<DECODE_BITS)];
static struct SymbolCode	encode[0x100];


//------------------------------------------------------------------------------//
//	Simple coder methods		//
//------------------------------------------------------------------------------//

SymbolCodePtr coder_base()	{
	return encode;
}

unsigned coder_memory()	{
	return sizeof(dec_offset) + sizeof(dec_list) + sizeof(encode);
}


//------------------------------------------------------------------------------//
//	Build encoding table	//
//------------------------------------------------------------------------------//

dword coder_build_encoder(int const *const freq, byte const *const trans, const byte bits)	{
	// fill in the encoding table
	int i,k;
	memset(encode, 0, sizeof(encode) );
	assert(0<MAX_CODE_LENGTH && MAX_CODE_LENGTH<=32);
	srand(RANDOM_LENGHT_TEST);
	for(i=k=0; i!=0x100; ++i)	{
		struct SymbolCode *const ps = encode + i;
		ps->length = freq[i] ? bits : 0;
		ps->code = trans[i];
		if(RANDOM_LENGHT_TEST && freq[i])	{
			const int shift = RANDOM_LENGHT_TEST - ps->length;
			assert(shift>=0);
			ps->code <<= shift;
			ps->length += shift;
		}
		k += freq[i] * ps->length;
	}
	return k;
}


//------------------------------------------------------------------------------//
//	Encode byte stream	//
//------------------------------------------------------------------------------//

int coder_encode_stream(dword *const output, const byte *const input, const int num)	{
	int i,k;
	// transform input (k = dest bit index)
	for(output[i=k=0]=0; i!=num; ++i)	{
		const byte symbol = input[i];
		SymbolCodePtr const sc = encode + symbol;
		const int k2 = k + sc->length;
		assert( !(sc->code >> sc->length) );
		assert( k<k2 && sc->length<=MAX_CODE_LENGTH );
		output[k>>5] |= sc->code << (k & 0x1F);
		if((k>>5) != (k2>>5))	// next dword
			output[k2>>5] = sc->code >> ( 0x20-(k&0x1F) );
		k = k2;
	}
	return k;
};


//------------------------------------------------------------------------------//
//	Build decoding table	//
//------------------------------------------------------------------------------//

void coder_build_decoder()	{
	const dword mask = (1<<DECODE_BITS)-1;
	int i, total = 0;
	// collect bin frequencies
	memset( dec_offset, 0, sizeof(dec_offset) );
	for(i=0; i!=0x100; ++i)	{
		SymbolCodePtr const ps = encode + i;
		word *const pd = dec_offset + 1 + (ps->code & mask);
		if(ps->length < DECODE_BITS)	{
			int j = 1<<DECODE_BITS;
			if(!ps->length) continue;
			while(j>0)	{
				word *const px = pd + (j-=(1<<ps->length));
				assert(!*px && "Inconsistent codes");
				*px = 1; ++total;
			}
			assert(!j);
		}else	{
			++*pd; ++total;
		}
	}
	assert(total < sizeof(dec_list));
	// turn into bin offsets
	for(i=mask+1; i; --i)	{
		dec_offset[i] = (total -= dec_offset[i]);
	}
	assert(!total);
	// collect bins
	for(i=0; i!=0x100; ++i)	{
		SymbolCodePtr const ps = encode + i;
		word *const pd = dec_offset + 1 + (ps->code & mask);
		if(ps->length < DECODE_BITS)	{
			int j = 1<<DECODE_BITS;
			if(!ps->length) continue;
			while(j>0)	{
				word *const px = pd + (j-=(1<<ps->length));
				dec_list[(*px)++] = i;
			}
		}else	{
			dec_list[(*pd)++] = i;
		}
	}
};


//------------------------------------------------------------------------------//
//	Decode a symbol from the binary code	//
//------------------------------------------------------------------------------//

SymbolCodePtr coder_decode_symbol(const dword data)	{
	dword cod = data & ((1<<DECODE_BITS)-1);
	word *const ps = dec_offset + cod;
	const word limit = ps[1];
	SymbolCodePtr sc = NULL;
	word i;
	assert(ps[0] < limit);
	for(i=ps[0]; ; ++i)	{
		assert(i < limit);	// can be optimized a bit
		sc = encode + dec_list[i];
		cod = data & ((1<<sc->length)-1);
		if(cod == sc->code)	break;
	}
	return sc;
}


//------------------------------------------------------------------------------//
//	Extract symbol code to the left of the offset (in bits	//
//------------------------------------------------------------------------------//

dword coder_extract_code(byte const* const ptr, const int offset)	{
	dword code = *(dword*)(ptr+(offset>>3)) >> (offset&7);
	if (MAX_CODE_LENGTH>24)	{
		const byte log = 32 - (offset&7);
		const dword us = ptr[4+(offset>>3)] << 24;
		code |= us << (8-(offset&7));
	}
	return code;
}
