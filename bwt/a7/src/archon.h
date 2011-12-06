typedef	int	suffix;
typedef unsigned char byte;
typedef unsigned long dword;


class Archon	{
	suffix *const P;
	int	*const R;
	byte *const str;
	byte *const bitMask;
	int N;

public:
	const int Nmax;
	
	Archon(const int N);
	~Archon();
	int read(FILE *const fx);
	int compute();
	int write(FILE *const fx);
};
