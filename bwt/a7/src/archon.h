typedef	signed int		suffix;
typedef	unsigned int	t_index;
typedef unsigned char	byte;
typedef unsigned short	dbyte;
typedef unsigned long	word;


class Archon	{
	const t_index Nmax, Nreserve;
	suffix *const P;
	byte *const str;
	t_index N, baseId;
	void roll(const t_index i);

public:
	static t_index estimateReserve(const t_index);
	Archon(const t_index N);
	~Archon();
	unsigned countMemory() const;
	// encoding
	int en_read(FILE *const fx, t_index ns);
	int en_compute();
	int en_write(FILE *const fx);
	// decoding
	int de_read(FILE *const fx, t_index ns);
	int de_compute();
	int de_write(FILE *const fx);
};
