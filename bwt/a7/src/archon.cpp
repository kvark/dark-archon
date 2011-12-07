#include <assert.h>
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

	
	byte isUp(const int i) const	{
		return (bits[i>>3] >> (i&7)) & 1;
	}
	void setUp(const int i)	{
		bits[i>>3] |= 1<<(i&7);
	}
	bool isElbow(const int i) const	{
		assert(i>=0);
		const dword dx = *(dword*)( bits+(i>>3) ) >> (i&0x7U);
		return (dx & 3) == 1;	//has to be 10b
	}

	void classify()	{
		memset( bits, 0, (N>>3)+1 );
		int prev = data[0]+1;
		for(int i=0; i!=N; ++i)	{
			const int cur = data[i];
			if(prev > cur)	{
				setUp(i);
				prev = cur+1;
			}else
				prev = cur;
		}
		// make sure the reversed string
		// does not start with LMS
		setUp(N);
	}

	void buckets()	{
		memset( R, 0, K*sizeof(int) );
		int i,sum;
		for(i=0; i<N; ++i)	{
			assert(data[i]>=0 && data[i]<K);
			R[data[i]] += 1;
		}
		for(R[i=K]=sum=N; i--;)	{
			R[i] = (sum -= R[i]);
		}
		assert(!sum);
	}

	template<int OFF>	int advance(const T);
	template<>	int advance<0>(const T x)	{ return R[x]++;	}
	template<>	int advance<1>(const T x)	{ return --RE[x];	}

	template<int OFF>
	void induce()	{
		buckets();
		for(int i=0; i<=N; ++i)	{
			const suffix j = P[ OFF ? (N-i) : i ];
			if(j>=0 && j<N && isUp(j)==OFF)
				P[advance<OFF>(data[j])] = j+1;
		}
	}

	void reduce()	{
		int i;
		memset( P, -1, N*sizeof(suffix) );
		P[N] = 0;
		// scatter LMS into bucket positions
		buckets();
		for(i=N; --i>=0; )	{
			if(isElbow(i))
				P[--RE[data[i]]] = i+1;
		}
		// sort by induction (evil technology!)
		induce<0>();
		induce<1>();
		// pack LMS into the first n1 suffixes
		for(n1=i=0; i<N; ++i)	{
			const suffix pos = P[i];
			assert(pos>=0);
			if(isElbow(pos-1))
				P[n1++] = pos;
		}
		assert(n1+n1<=N);
		// compare LMS substrings char by char
		// with LMS signs as delimeters
		// to define the first set of values
		// and write them into [n1,2*n1)
		memset( P+n1, -1, (N-n1)*sizeof(suffix) );
		int prev = -1;
		for(name=0,i=0; i<n1; ++i)	{
			const suffix pos = P[i];
			// we compare the next LMS substring with the previous one
			for(int d=1; d<=N; ++d)	{
				assert(prev<0 || data[pos-d] >= data[prev-d]);
				if(prev<0 || data[pos-d]!=data[prev-d] || isUp(pos-d) != isUp(prev-d))	{
					// this one is different - remember it
					//TODO: keep group information here to accelerate farther sorting
					++name; prev=pos; break;
				}else if(d>1 && (isElbow(pos-d) || isElbow(prev-d)))
					break;
			}
			// write somewhere in the accessible area
			// it's guaranteed that no two LMS are together
			// therefore we can divide the index by two here
			const int id = n1+(pos>>1);
			assert(id<N);
			P[id] = name-1;
		}
		// pack values into the last n1 suffixes
		int j;
		for(i=j=N; --i>=n1; )		{
			if(P[i]>=0)
				P[--j]=P[i];
		}
		assert(j+n1 == N);
	}

	void solve()	{
		suffix *const s1 = P+N-n1;
		if(name<n1)	{
			Sais<suffix>( s1, P, bits+(N>>3)+1, n1, R, name );
		}else	{
			// permute back from values into indices
			assert(name == n1);
			for(int i=0; i<n1; ++i)
				P[s1[i]] = i+1;
		}
	}

	void goback()	{
		suffix *const s1 = P+N-n1;
		int i,j;
		// get the list of LMS strings
		// LMS number -> actual string number
		//note: going left to right here!
		for(i=0,j=0; i<N; ++i)	{
			if(isElbow(i))	{
				assert( n1+j<N );
				s1[j++] = i+1;
			}
		}
		assert(j==n1);
		// update the indices in the sorted array
		// LMS index -> string index
		for(i=0; i<n1; ++i)	{
			j = P[i];
			assert(j>0 && j<=n1);
			P[i] = s1[j-1];
		}
		// scatter LMS back into proper positions
		buckets();
		memset( P+n1, -1, (N-n1)*sizeof(suffix) );
		T prev_sym = K-1;
		for(i=n1; --i>=0; )	{
			j = P[i]; P[i] = -1;
			assert(j>0 && j<=N				&& "Invalid suffix!");
			assert(data[j-1] <= prev_sym		&& "Not sorted!");
			int *const pr = RE+data[j-1];
			suffix *const s = P + --*pr;
			assert(pr[0] >= pr[-1]	&& "Stepped twice on the same suffix!");
			assert(pr[0] >= i		&& "Not sorted properly!");
			*s = j;
		}
		// induce the rest of suffixes
		induce<0>();
		induce<1>();
	}

public:
	Sais(T *const _data, suffix *const _P, byte *const _bits, const int _N, int *const _R, const int _K)
	: data(_data), P(_P), bits(_bits), N(_N), R(_R), RE(_R+1), K(_K), n1(-1), name(-1)	{
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
, bitMask(new byte[(Nx>>2)+4])
, N(-1), Nmax(Nx) {
	assert(P && R && str && bitMask);
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
	int base_id = -1;
	for(int i=0; i!=N; ++i)	{
		suffix pos = P[i];
		if(pos == N)	{
			base_id = i;
			pos = 0;
		}
		fputc( str[sTermin+pos], fx);
	}
	fwrite( &base_id, sizeof(int), 1, fx);
	return 0;
}
