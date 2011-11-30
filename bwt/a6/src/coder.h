
SymbolCodePtr	coder_base();
unsigned		coder_memory();
void			coder_build_encoder_byte();
dword			coder_build_encoder_fixed(int const *const, const byte);
dword			coder_build_encoder(int const *const);
int				coder_encode_stream(dword *const, const byte *const, const int);
void			coder_build_decoder();
SymbolCodePtr	coder_decode_symbol_fixed(const dword);
SymbolCodePtr	coder_decode_symbol(const dword);
dword			coder_extract_code(byte const* const ptr, const int offset);
