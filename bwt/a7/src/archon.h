typedef	unsigned int	suffix;
typedef	unsigned int	index;
typedef unsigned char	byte;
typedef unsigned short	dbyte;
typedef unsigned long	word;


class Archon	{
	suffix *const P;
	byte *const str;
	const index Nmax;
	index N, baseId;
	void roll(const index i);

public:
	Archon(const index N);
	~Archon();
	// encoding
	int en_read(FILE *const fx, index ns);
	int en_compute();
	int en_write(FILE *const fx);
	// decoding
	int de_read(FILE *const fx, index ns);
	int de_compute();
	int de_write(FILE *const fx);
};
