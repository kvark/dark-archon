#include <stdio.h>

#include "common.h"
#include "archon.h"

Archon::Archon(const int N): Nmax(N), P(new suffix[Nmax+1])	{
	//empty
}

Archon::~Archon()	{
	delete[] P;
}

int Archon::read(FILE *const fx)	{
	return 0;
}

int Archon::compute()	{
	return 0;
}

int Archon::write(FILE *const fx)	{
	return 0;
}
