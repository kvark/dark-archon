
enum KeyConfig	{
	KEY_BYTE,
	KEY_FIXED,
	KEY_VARIABLE,
	KEY_NONE
};

void		bwt_init(int,byte,enum KeyConfig);
void		bwt_exit();
unsigned	bwt_memory();
int			bwt_read(FILE *const);
void		bwt_write(FILE *const);
int			bwt_transform();