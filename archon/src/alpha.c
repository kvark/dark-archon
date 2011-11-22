/*	alpha.c
 *	Alphabet compression utility.
*	project Archon (C) Dzmitry Malyshau, 2011
 */

#include "common.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>


/*	Huffman coding
	Symbol -> Node
	Nodes -> List
	Node -> Bit,Parent
	Symbol -> Code
*/

struct HuffCode	{
	dword	code;
	byte	length;
};

struct HuffNode	{
	struct HuffNode* parent;
	dword	weight;
	byte	value;
};
typedef struct HuffNode *NodePtr;

static void get_code(struct HuffCode *const cod, const struct HuffNode *const nod)	{
	if(nod && nod->parent)	{
		get_code( cod, nod->parent );
		cod->code = (cod->code<<1) + nod->value;
		++cod->length;
	}
}

static void free_node(struct HuffNode *const nod)	{
	if(nod->parent)	{
		free_node( nod->parent );
		nod->parent = NULL;	
	}
	if(nod->weight)
		nod->weight = 0;
	else
		free(nod);
}

static int cmp_nodes(const void *pa, const void *pb)	{
	const NodePtr na = *(NodePtr*)pa;
	const NodePtr nb = *(NodePtr*)pb;
	return (int)nb->weight - (int)na->weight;
}

static void make_codes(const dword *const freq, struct HuffCode *const result)	{
	int i, n_nodes = 0;
	NodePtr table[0x100], nodes[0x100];
	memset( result,0, 0x100 * sizeof(struct HuffCode) );
	for(i=0; i!=0x100; ++i)	{
		if(freq[i])	{
			struct HuffNode *const nod = malloc( sizeof(struct HuffNode) );
			nod->parent = NULL;
			nod->weight = freq[i];
			nod->value = 0;
			table[i] = nod;
			nodes[n_nodes++] = nod;
		}else
			table[i] = NULL;
	}
	for(; n_nodes>1; --n_nodes)	{
		struct HuffNode *na,*nb,
			*const nod = malloc( sizeof(struct HuffNode) );
		qsort( nodes, n_nodes, sizeof(void*), cmp_nodes );
		na = nodes[n_nodes-1];
		nb = nodes[n_nodes-2];
		nod->parent = NULL;
		nod->weight = na->weight + nb->weight;
		nod->value = 0;
		na->parent = nb->parent = nodes[n_nodes-2] = nod;
		na->value = 0;
		nb->value = 1;
	}
	for(i=0; i!=0x100; ++i)	{
		get_code( result+i, table[i] );
	}
	for(i=0; i!=0x100; ++i)	{
		if(!table[i])	continue;
		table[i]->weight = 0;
		free_node( table[i] );
	}
}


int main(const int argc, const char *argv[])	{
	dword freq[0x100] = {0};
	struct HuffCode codes[0x100];
	FILE *fi = NULL;
	int length = 0, sym = 0, total = 0, alpha_size = 0;
	printf("Alphabet compression tool\n");
	printf("\tproject Archon (C) Dzmitry Malyshau\n");
	if(argc != 2) return -1;
	memset( codes, 0, sizeof(codes) );
	fi = fopen(argv[1],"rb");
	if(!fi) return -2;
	printf("Reading...\n");
	fseek(fi,0,SEEK_END);
	length = ftell(fi);
	fseek(fi,0,SEEK_SET);
	while( fread(&sym,1,1,fi) )	{
		++freq[sym];
	}
	fclose(fi);
	printf("Building tree...\n");
	make_codes(freq,codes);
	printf("Encoding...\n");
	for(sym=0; sym!=0x100; ++sym)	{
		if(!freq[sym]) continue;
		++alpha_size;
		total += freq[sym] * codes[sym].length;
	}
	printf("\tAlphabet size: %d\n", alpha_size );
	printf("\tAvg length: %.2f bits\n", total*1.f / length );
	printf("Done\n");
	return 0;
}