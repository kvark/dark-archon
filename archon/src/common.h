typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

struct SymbolCode	{
	// maximum Huffman code for the 32bit input has almost 48 bit length
	// therefore, we need to restrict the code length to 32 upon encoding
	// ... ore use 64 bit code!
	dword	code;
	byte	length;
};

typedef struct SymbolCode const* SymbolCodePtr;