/*	huff.c
 *	Huffman binary encoding routines.
 *	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <assert.h>

#include "common.h"
#include "huff.h"


// Types and Data

struct Node	{
	int parent;
	dword weight;
	dword value;
	byte length;
}static node[0x201];

static byte max_length=0;
static int next_id=0;


//------------------------------------------------------------------------------//
//	Initialize Huffman encoder		//
//------------------------------------------------------------------------------//

void huff_init(byte length)	{
	max_length = length;
	next_id=0;
}


//------------------------------------------------------------------------------//
//	Report memory used		//
//------------------------------------------------------------------------------//


unsigned huff_memory()	{
	return sizeof(node);
}


//------------------------------------------------------------------------------//
//	Register a new symbol		//
//------------------------------------------------------------------------------//

int huff_add(dword weight)	{
	struct Node *const pn = node + next_id;
	assert( next_id < sizeof(node)/sizeof(struct Node) );
	pn->parent = -1;
	pn->weight = weight;
	pn->value = 0;
	return next_id++;
}


//------------------------------------------------------------------------------//
//	Extract symbol code		//
//------------------------------------------------------------------------------//

struct SymbolCode huff_extract(int id)	{
	const struct SymbolCode sc = { node[id].value, node[id].length };
	assert(id>=0 && id<next_id);
	return sc;
}


//------------------------------------------------------------------------------//
//	Construct Huffman code tree		//
//------------------------------------------------------------------------------//

byte huff_compute(struct SymbolCode *const ps)	{
	const int total = next_id;
	int i, num_left=next_id, table[0x100];
	assert( next_id <= sizeof(table)/sizeof(int) );
	for(i=0; i!=next_id; ++i)
		table[i] = i;
	// construct the node tree
	while(num_left > 1)	{
		// choose two maximum weight nodes
		int min0=0, min1=1;
		dword w0 = node[table[0]].weight;
		dword w1 = node[table[1]].weight;
		if(w0 > w1)	{
			const dword w2=w0;
			w0=w1; w1=w2;
			min0=1; min1=0;
		}
		for(i=2; i<num_left; ++i)	{
			const dword w = node[table[i]].weight;
			if(w>=w1)	continue;
			if(w<w0)	{
				min1=min0; w1=w0;
				min0=i; w0=w;
			}else	{
				min1=i; w1=w;
			}
		}
		// compose them into a new node
		assert(min0 != min1);
		assert(node[table[min0]].weight == w0);
		assert(node[table[min1]].weight == w1);
		i = huff_add(w0+w1);
		node[table[min0]].parent = node[table[min1]].parent = i;
		node[table[min0]].value = 0;
		node[table[min1]].value = 1;
		--num_left;
		if(min0 != num_left)
			table[min0] = (min1==num_left ? i : table[num_left]);
		table[min1] = i;
	}
	// fill in the codes
	assert(table[0]+1 == next_id);
	node[table[0]].length = 0;
	for(num_left=0,i=next_id-1; --i>=0;)		{
		const int par = node[i].parent;
		const byte len = 1+node[par].length;
		assert(par>i);	// append the code
		node[i].value += node[par].value<<1;
		node[i].length = len;
		// remember the maximum code length
		if(len > num_left)
			num_left = len;
	}
	assert(num_left <= max_length);
	return num_left;
}
