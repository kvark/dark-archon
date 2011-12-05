
class Archon	{
	typedef	int	suffix;
	suffix *const P;
public:
	const int Nmax;
	Archon(const int N);
	~Archon();
	int read(FILE *const fx);
	int compute();
	int write(FILE *const fx);
};
