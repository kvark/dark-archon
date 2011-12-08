typedef	unsigned int	suffix;
typedef unsigned char	byte;
typedef unsigned long	dword;


class Archon	{
	suffix *const P;
	unsigned *const R;
	byte *const str;
	const unsigned Nmax;
	unsigned N, baseId;
	void roll(const unsigned i);

public:
	Archon(const unsigned N);
	~Archon();
	// encoding
	int en_read(FILE *const fx, unsigned ns);
	int en_compute();
	int en_write(FILE *const fx);
	// decoding
	int de_read(FILE *const fx, unsigned ns);
	int de_compute();
	int de_write(FILE *const fx);
};
