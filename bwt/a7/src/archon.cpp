#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "archon.h"

//	Memory requirements:		7.25n
//	Execution time complexity:	O(n)

//--------------------------------------------------------//

template<int OFF>	int advance(int *const);
template<>	int advance<0>(int *const pr)	{ return pr[0]++;	}
template<>	int advance<1>(int *const pr)	{ return --pr[1];	}


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
		return (dx & 3) == 1;	// has to be 10b
	}

	void checkData();

	void classify()	{
		checkData();
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
			R[data[i]] += 1;
		}
		for(R[i=K]=sum=N; i--;)	{
			R[i] = (sum -= R[i]);
		}
		assert(!sum);
	}

	template<int OFF>
	void induce()	{
		buckets();
		for(int i=0; i<=N; ++i)	{
			const suffix j = P[ OFF ? (N-i) : i ];
			if(j>=0 && j<N && isUp(j)==OFF)
				P[advance<OFF>(R+data[j])] = j+1;
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
		suffix prev = 0;
		for(name=0,i=0; i<n1; ++i)	{
			const suffix pos = P[i];
			// we compare the next LMS substring with the previous one
			for(int d=1; d<=N; ++d)	{
				assert(!prev || pos<d || data[pos-d] >= data[prev-d]);
				if(!prev || pos<d || data[pos-d]!=data[prev-d] || isUp(pos-d) != isUp(prev-d))	{
					// this one is different - remember it
					//todo: keep group information here to accelerate farther sorting
					assert(pos>=d || (pos==d-1 && prev>pos));
					++name; prev=pos; break;
				}else	{
					assert( d<=pos && d<=prev );
					assert( d!=1 || (isElbow(pos-d) && isElbow(prev-d)) );
					if(d>1 && (isElbow(pos-d) || isElbow(prev-d)))
						break;
				}
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
		for(i=N,j=N; --i>=n1; )		{
			if(P[i]>=0)
				P[--j]=P[i];
		}
		assert(j>0 && j+n1 == N);
		P[j-1] = name;	// terminator!
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
			//note: this is the only place after recursion
			// where we need the bit array
			// moreover, we use it in a sequential order
			//todo: don't use bit array here,
			// reducing the memory requirements by N/4
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
			P[--*pr] = j;
			assert(pr[0] >= pr[-1]	&& "Stepped twice on the same suffix!");
			assert(pr[0] >= i		&& "Not sorted properly!");
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

template<>	void Sais<byte>::checkData()	{
	assert(K==0x100);
}
template<>	void Sais<suffix>::checkData()	{
	for(int i=0; i<N; ++i)
		assert(data[i]>=0 && data[i]<K);
}


//--------------------------------------------------------//

//	INITIALIZATION	//

Archon::Archon(const int Nx)
: P(new suffix[Nx+1])
, R(new int[((Nx>>9) ? (Nx>>1) : 256)+1])
, str(new byte[Nx+1])
, bitMask(new byte[(Nx>>2)+4])
, Nmax(Nx), N(-1), baseId(-1) {
	assert(P && R && str && bitMask);
}

Archon::~Archon()	{
	delete[] P;
	delete[] R;
	delete[] str;
	delete[] bitMask;
}


//	ENCODING	//

int Archon::en_read(FILE *const fx, int ns)	{
	assert(ns>=0 && ns<=Nmax);
	N = fread( str, 1, ns, fx );
	return N;
}

int Archon::en_compute()	{
	Sais<byte>( str, P, bitMask, N, R, 256 );
	return 0;
}

int Archon::en_write(FILE *const fx)	{
	baseId = -1;
	for(int i=0; i!=N; ++i)	{
		suffix pos = P[i];
		if(pos == N)	{
			baseId = i;
			pos = 0;
		}
		fputc( str[pos], fx);
	}
	fwrite( &baseId, sizeof(int), 1, fx);
	return 0;
}


//	DECODING	//

void Archon::roll(const int i)	{
	P[i] = R[str[i]]++;
	assert(N==1 || i != P[i]);
}

int Archon::de_read(FILE *const fx, int ns)	{
	en_read(fx,ns);
	int ok = fread( &baseId, sizeof(int), 1, fx );
	assert(ok && baseId>=0 && baseId<N);
	return N;
}

int Archon::de_compute()	{
	int i,k;
	memset( R, 0, 0x100*sizeof(int) );
	memset( P, -1, (N+1)*sizeof(int) );		// for debug
	for(i=0; i!=N; ++i)
		R[str[i]] += 1;
	for(k=N,i=0x100; i--;)
		R[i] = (k-=R[i]);
	for(i=0; i<baseId; i++)		roll(i);
	for(i=baseId+1; i<N; i++)	roll(i);
	roll(baseId);
	return 0;
}

int Archon::de_write(FILE *const fx)	{
	int i,k;
	for(i=0,k=baseId; i!=N; ++i,k=P[k])
		putc(str[k],fx);
	assert( k==baseId );
	return 0;
}
