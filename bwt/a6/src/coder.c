/*	coder.c
 *	Stream encoding/decoding routines.
 *	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#include "common.h"
#include "coder.h"
#include "huff.h"


// Types and Data

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
	return huff_memory() +
		sizeof(dec_offset) + sizeof(dec_list) + sizeof(encode);
}

//------------------------------------------------------------------------------//
//	Build encoding table: trivial byte variant		//
//------------------------------------------------------------------------------//

void coder_build_encoder_byte()	{
	int i;
	for(i=0; i!=0x100; ++i)	{
		encode[i].code = i;
		encode[i].length = 8;
	}
}

//------------------------------------------------------------------------------//
//	Build encoding table: fixed-bit variant		//
//------------------------------------------------------------------------------//

dword coder_build_encoder_fixed(int const *const freq, const byte bits)	{
	int i,k,rank=0;
	memset(encode, 0, sizeof(encode) );
	assert(0<MAX_CODE_LENGTH && MAX_CODE_LENGTH<=32);
	srand(RANDOM_LENGHT_TEST);
	for(i=k=0; i!=0x100; ++i)	{
		struct SymbolCode *const ps = encode + i;
		ps->length = freq[i] ? bits : 0;
		ps->code = rank;
		if(!freq[i]) continue;
		dec_list[rank++] = i;
		if(RANDOM_LENGHT_TEST)	{
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
//	Build encoding table: huffman codes		//
//------------------------------------------------------------------------------//

dword coder_build_encoder(int const *const freq)	{
	int i,k,q;
	memset(encode, 0, sizeof(encode) );
	assert(0<MAX_CODE_LENGTH && MAX_CODE_LENGTH<=32);
	huff_init(MAX_CODE_LENGTH);
	for(i=0; i!=0x100; ++i)	{
		if(freq[i])
			huff_add(freq[i]);
	}
	huff_compute();
	for(i=k=q=0; i!=0x100; ++i)	{
		if(!freq[i]) continue;
		encode[i] = huff_extract(q);
		k += freq[i] * encode[i].length;
		++q;
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
	int i, total = 0;
	// collect bin frequencies
	memset( dec_offset, 0, sizeof(dec_offset) );
	for(i=0; i!=0x100; ++i)	{
		SymbolCodePtr const ps = encode + i;
		if(ps->length < DECODE_BITS)		{
			const byte free_log = DECODE_BITS - ps->length;
			const dword id = ps->code << free_log;
			word *pd = dec_offset + 1 + id;
			int j;
			if(!ps->length) continue;
			for(j=0; !(j>>free_log); ++j,++pd)	{
				assert(!*pd && "Inconsistent codes");
				*pd = 1; ++total;
			}
		}else	{
			// leave only required number of bits
			const dword id = ps->code >> (ps->length - DECODE_BITS);
			dec_offset[1+id] += 1; ++total;
		}
	}
	assert(total < sizeof(dec_list));
	// turn into bin offsets
	for(i=(1<<DECODE_BITS); i; --i)	{
		dec_offset[i] = (total -= dec_offset[i]);
	}
	assert(!total);
	// collect bins
	for(i=0; i!=0x100; ++i)	{
		SymbolCodePtr const ps = encode + i;
		if(!ps->length) continue;
		if(ps->length < DECODE_BITS)	{
			const byte free_log = DECODE_BITS - ps->length;
			const dword id = ps->code << free_log;
			word *pd = dec_offset + 1 + id;
			int j;
			for(j=0; !(j>>free_log); ++j,++pd)
				dec_list[(*pd)++] = i;
		}else	{
			const dword id = ps->code >> (ps->length - DECODE_BITS);
			word *const pd = dec_offset + 1 + id;
			dec_list[(*pd)++] = i;
		}
	}
};

//------------------------------------------------------------------------------//
//	Decode a symbol from the binary code: fixed-bit version	//
//------------------------------------------------------------------------------//

SymbolCodePtr coder_decode_symbol_fixed(const dword data)	{
	const byte bits = encode[dec_list[0]].length;
	const dword code = data>>(32-bits);
	const byte ch = dec_list[code];
	const SymbolCodePtr ptr = encode + ch;
	assert(ptr->code == code && ptr->length == bits);
	return ptr;
}

//------------------------------------------------------------------------------//
//	Decode a symbol from the binary code	//
//------------------------------------------------------------------------------//

SymbolCodePtr coder_decode_symbol(const dword data)	{
	dword cod = (data >> 8) >> (24-DECODE_BITS);
	word *const ps = dec_offset + cod;
	const word limit = ps[1];
	SymbolCodePtr sc = NULL;
	word i;
	assert(DECODE_BITS <= 24);
	assert(ps[0] < limit);
	for(i=ps[0]; ; ++i)	{
		assert(i < limit);	// can be optimized a bit
		sc = encode + dec_list[i];
		cod = data >> (32-sc->length);
		if(cod == sc->code)	break;
	}
	return sc;
}


//------------------------------------------------------------------------------//
//	Extract symbol code to the left of the offset (in bits)	//
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
