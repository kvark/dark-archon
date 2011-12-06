#include <memory.h>
#include <stdio.h>

#include "common.h"
#include "archon.h"


//--------------------------------------------------------//

template<typename T>
class	Sais	{
	T *const data;
	suffix *const P;
	byte *const bits;
	int *const R, *const RE;
	const int N,K;
	int n1,name;

	
	byte isLimit(const int i) const	{
		return (bits[i>>3] >> (i&7)) & 1;
	}
	void setLimit(const int i)	{
		bits[i>>3] |= 1<<(i&7);
	}
	bool isElbow(const int i) const	{
		if(i<=0) return false;
		const dword dx = *(dword*)( bits+((i-1)>>3) ) >> ((i-1)&0x7U);
		return (dx & 3) == 1;	//has to be 10b
	}

	void classify()	{
		memset( bits, 0, (N>>3)+1 );
		int prev = data[0]+1;
		for(int i=0; i!=N; ++i)	{
			const int cur = data[i];
			if(prev > cur)	{
				setLimit(i);
				prev = cur+1;
			}else
				prev = cur;
		}
	}

	void buckets()	{
		memset( R, 0, K*sizeof(int) );
		int i,sum;
		for(i=0; i<N; ++i)
			R[data[i]] += 1;
		for(R[i=K]=sum=N; i--;)	{
			R[i] = (sum -= R[i]);
		}
	}

	template<int OFF>	int advance(const T);
	template<>	int advance<0>(const T x)	{ return R[x]++;	}
	template<>	int advance<1>(const T x)	{ return --RE[x];	}

	template<int OFF>
	void induce()	{
		buckets();
		for(int i=0; i<N; ++i)	{
			const suffix j = P[i];
			if(j>=0 && j<N && isLimit(j)==OFF)
				P[advance<OFF>(data[j])] = j;
		}
	}

	void reduce()	{
		buckets();
		memset( P, -1, N*sizeof(suffix) );
		int i;
		for(i=0; i<N; ++i)	{
			if(isElbow(i))
				P[--RE[data[i]]] = i;
		}
		induce<0>();
		induce<1>();
		n1 = 0;
		for(i=0; i<N; ++i)	{
			if(isElbow(P[i]))
				P[n1++] = P[i];
		}
		memset( P+n1, -1, (N-n1)*sizeof(suffix) );
		int prev=-1;
		name = 0;
		for(i=0; i<n1; ++i)	{
			suffix pos = P[i];
			bool diff = false;
			for(int d=0; d<N; ++d)	{
				if(prev<0 || data[pos-d]!=data[prev-d] || isLimit(pos-d) != isLimit(prev-d))	{
					diff = true; break;
				}else if(d>0 && (isElbow(pos-d) || isElbow(prev-d)))
					break;
				if(diff) { ++name; prev=pos; }
				pos >>= 1;
				P[n1+pos] = name-1;
			}
		}
		int j;
		for(i=j=N-1; i>=n1; --i)		{
			if(P[i]>=0) P[--j]=P[i];
		}
	}

	void solve()	{
		suffix *const s1 = P+N-n1;
		if(name<n1)	{
			Sais<suffix>( s1, P, bits, n1, R, name );
		}else	{
			// permute back from values into indices
			assert(name == n1);
			for(int i=0; i<n1; ++i)
				P[s1[i]] = i;
		}
	}

	void goback()	{
		suffix *const s1 = P+N-n1;
		int i,j;
		// get the list of LMS strings
		// LMS number -> actual string number
		for(i=1,j=0; i<N; ++i)	{
			if(isElbow(i)) s1[j++] = i;
		}
		// update the indices in the sorted array
		// LMS index -> string index
		for(i=0; i<n1; ++i)
			P[i] = s1[P[i]];
		// scatter LMS back into proper positions
		buckets();
		memset( P+n1, -1, (N-n1)*sizeof(suffix) );
		for(i=n1-1; i>=0; --i)	{
			j = P[i]; P[i] = -1;
			P[--RE[data[j]]] = j;
		}
		// induce the rest of suffixes
		induce<0>();
		induce<1>();
	}

public:
	Sais(T *const _data, suffix *const _P, byte *const _bits, const int _N, int *const _R, const int _K)
	: data(_data), P(_P), bits(_bits), N(_N), R(_R), RE(_R+1), K(_K)	{
		classify();
		reduce();
		solve();
		goback();
	}
};


//--------------------------------------------------------//


static const int sTermin = 10;

Archon::Archon(const int Nx)
: P(new suffix[Nx+1])
, R(new int[((Nx>>9) ? (Nx>>1) : 256)+1])
, str(new byte[sTermin+Nx+1])
, bitMask(new byte[(Nx>>3)+4])
, N(-1), Nmax(Nx) {
	//empty
}

Archon::~Archon()	{
	delete[] P;
	delete[] R;
	delete[] str;
	delete[] bitMask;
}

int Archon::read(FILE *const fx)	{
	byte *const input = str + sTermin;
	N = fread(input,1,Nmax,fx);
	return N;
}

int Archon::compute()	{
	Sais<byte>( str+sTermin, P, bitMask, N, R, 256 );
	return 0;
}

int Archon::write(FILE *const fx)	{
	return 0;
}
