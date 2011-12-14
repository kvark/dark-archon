typedef	unsigned int	suffix;
typedef	unsigned int	index;
typedef unsigned char	byte;
typedef unsigned short	dbyte;
typedef unsigned long	word;


class Archon	{
	const index Nmax, Nreserve;
	suffix *const P;
	byte *const str;
	index N, baseId;
	void roll(const index i);

public:
	static index estimateReserve(const index);
	Archon(const index N);
	~Archon();
	unsigned countMemory() const;
	// encoding
	int en_read(FILE *const fx, index ns);
	int en_compute();
	int en_write(FILE *const fx);
	// decoding
	int de_read(FILE *const fx, index ns);
	int de_compute();
	int de_write(FILE *const fx);
};
