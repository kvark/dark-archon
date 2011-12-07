typedef	int	suffix;
typedef unsigned char byte;
typedef unsigned long dword;


class Archon	{
	suffix *const P;
	int	*const R;
	byte *const str;
	byte *const bitMask;
	const int Nmax;
	int N, baseId;
	void roll(const int i);

public:
	Archon(const int N);
	~Archon();
	// encoding
	int en_read(FILE *const fx, int ns);
	int en_compute();
	int en_write(FILE *const fx);
	// decoding
	int de_read(FILE *const fx, int ns);
	int de_compute();
	int de_write(FILE *const fx);
};
