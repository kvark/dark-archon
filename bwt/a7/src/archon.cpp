#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "archon.h"

//	Memory requirements:		7.125n
//	Execution time complexity:	O(n)

//--------------------------------------------------------//

template<typename T>
class	SaIs	{
	T *const data;
	suffix *const P;
	byte *const bits;
	unsigned *const R, *const RE;
	const unsigned N,K;
	unsigned n1,name;

	enum {
		SUF_MASK = 0x7FFFFFFFU,
		LMS_MASK = 0x80000000U
	};

	void setUp(const unsigned i)	{
		bits[i>>3] |= 1<<(i&7);
	}
	bool isElbow(const unsigned i) const	{
		assert(i<=N);
		const dword dx = *(dword*)( bits+(i>>3) ) >> (i&0x7U);
		return (dx & 3) == 1;	// has to be 10b
	}

	void checkData();

	void makeBuckets()	{
		memset( R, 0, K*sizeof(unsigned) );
		unsigned i,sum;
		for(i=0; i<N; ++i)
			++R[data[i]];
		for(R[i=K]=sum=N; i--;)
			R[i] = (sum -= R[i]);
		assert(!sum);
	}

	void buckets();

	void induce()	{
		const unsigned NL = N-1U;
		// condition "(j-1U) >= NL" cuts off
		// all 0-s and the N at once
		unsigned i;
		assert(N);
		//left2right
		buckets();
		for(i=0; i!=N; ++i)	{
			//todo: fix to support 2Gb input
			const suffix j = P[i];
			if((j-1U) >= NL)
				continue;
			const T cur = data[j];
			if(data[j-1] <= cur)
				P[R[cur]++] = j+1;
		}
		//right2left
		buckets();
		P[--RE[data[0]]] = 1;
		i=N; do	{
			const suffix j = P[--i];
			if((j-1U) >= NL)
				continue;
			const T cur = data[j];
			if(data[j-1] >= cur)
				P[--RE[cur]] = j+1;
		}while(i);
	}

	void valueLMS()	{
		// using area of P[n1,N)
		unsigned i,j;
		// find the length of each LMS substring
		// and write it into P[n1+(x>>1)]
		// no collisions guaranteed because LMS distance>=2
		for(i=0,j=0; ++i<N; )	{
			if(data[i-1] >= data[i])	
				continue;
			P[n1 + (i>>1)] = i-j;	//length
			j = i;
			while(++i<N && data[i-1] <= data[i]);
		}
		// compare LMS using known lengths
		unsigned prev_len = 0;
		suffix prev = 0;
		for(name=0,i=0; i!=n1; ++i)	{
			const suffix cur = P[i];
			unsigned &cur_len = P[n1+(cur>>1)];
			assert(cur_len);
			//todo: work around the jump
			if(cur_len == prev_len)	{
				j=1; do	{
					if(data[cur-j] != data[prev-j])
						goto greater;
				}while(++j <= cur_len);
			}else	{
				greater:	//warning!
				++name; prev = cur;
				prev_len = cur_len;
			}
			cur_len = name;
		}
	}

	void reduce()	{
		unsigned i,j;
		// classification stage
		memset( bits, 0, (N>>3)+1 );
		//todo: find the number of LMS here
		j = data[0]+1U;
		for(i=0; i!=N; ++i)	{
			const unsigned& cur = data[i];
			if(j > cur)	{
				setUp(i);
				j = cur+1;
			}else
				j = cur;
		}
		// make sure the reversed string
		// does not start with LMS
		setUp(N);
		// scatter LMS into bucket positions
		memset( P, 0, N*sizeof(suffix) );
		P[N] = 0;
		buckets();
		//todo: optimize more
		bool prevUp = true;
		for(j=N,n1=0;;)	{
			for(i=--j; j && data[j-1]==data[i]; --j);
			if(j && data[j-1]<data[i])	{
				prevUp = false;
			}else	{	//up
				if(!prevUp)	{
					++n1; // found LMS!
					P[--RE[data[i]]] = i+1;
					prevUp = true;
				}
				if(!j)
					break;
			}
		}
		assert(n1+n1<=N);
		// sort by induction (evil technology!)
		induce();
		// pack LMS into the first n1 suffixes
		for(j=i=0; ;++i)	{
			const suffix pos = P[i];
			assert(pos>0);
			if(isElbow(pos-1))	{
				//todo: pack this bit into P[i]
				P[j] = pos;
				if(++j == n1)
					break;
			}
		}
		// value LMS suffixes
#		ifndef	NDEBUG
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
#		endif
		valueLMS();
		// pack values into the last n1 suffixes
		for(i=N,j=N; ;)		{
			assert( i>n1 );
			if(P[--i])	{
				P[--j] = P[i]-1;
				if(j == N-n1)
					break;
			}
		}
	}

	void solve()	{
		suffix *const s1 = P+N-n1;
		if(name<n1)	{
			SaIs<suffix>( s1, P, bits, n1, R, name );
		}else	{
			// permute back from values into indices
			assert(name == n1);
			for(unsigned i=0; i<n1; ++i)
				P[s1[i]] = i+1;
		}
	}

	void goback()	{
		suffix *const s1 = P+N-n1;
		unsigned i,j;
		// get the list of LMS strings
		// LMS number -> actual string number
		//note: going left to right here!
		for(i=0,j=0; ++i<N; )	{
			if(data[i-1] >= data[i])	
				continue;
			assert( n1+j<N );
			s1[j++] = i;
			while(++i<N && data[i-1] <= data[i]);
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
#		ifndef	NDEBUG
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
#		endif
		T prev_sym = K-1;
		for(i=n1; i--; )	{
			j = P[i]; P[i] = 0;
			assert(j>0 && j<=N				&& "Invalid suffix!");
			assert(data[j-1] <= prev_sym		&& "Not sorted!");
			unsigned *const pr = RE+data[j-1];
			P[--*pr] = j;
			assert(pr[0] >= pr[-1]	&& "Stepped twice on the same suffix!");
			assert(pr[0] >= i		&& "Not sorted properly!");
		}
		// induce the rest of suffixes
		induce();
	}

public:
	SaIs(T *const _data, suffix *const _P, byte *const _bits, const unsigned _N, unsigned *const _R, const unsigned _K)
	: data(_data), P(_P), bits(_bits), N(_N), R(_R), RE(_R+1), K(_K), n1(0), name(0)	{
		checkData();
		reduce();
		solve();
		goback();
	}
};

static	unsigned R2[0x101];

template<>	void SaIs<byte>::checkData()	{
	assert(K==0x100);
	makeBuckets();
	memcpy( R2, R, sizeof(R2) );
}
template<>	void SaIs<suffix>::checkData()	{
	for(unsigned i=0; i<N; ++i)
		assert(data[i]>=0 && data[i]<K);
}

template<>	void SaIs<byte>::buckets()	{
	memcpy( R, R2, sizeof(R2) );
}
template<>	void SaIs<suffix>::buckets()	{
	makeBuckets();
}


//--------------------------------------------------------//

//	INITIALIZATION	//

Archon::Archon(const unsigned Nx)
: P(new suffix[Nx+0x102])
, R(new unsigned[((Nx>>9) ? (Nx>>1) : 0x100)+1])
, str(new byte[Nx+1])
, bitMask(new byte[(Nx>>3)+4])
, Nmax(Nx), N(0), baseId(0) {
	assert(P && R && str && bitMask);
}

Archon::~Archon()	{
	delete[] P;
	delete[] R;
	delete[] str;
	delete[] bitMask;
}


//	ENCODING	//

int Archon::en_read(FILE *const fx, unsigned ns)	{
	assert(ns>=0 && ns<=Nmax);
	N = fread( str, 1, ns, fx );
	return N;
}

int Archon::en_compute()	{
	SaIs<byte>( str, P, bitMask, N, R, 256 );
	return 0;
}

int Archon::en_write(FILE *const fx)	{
	baseId = N;
	for(unsigned i=0; i!=N; ++i)	{
		suffix pos = P[i];
		if(pos == N)	{
			baseId = i;
			pos = 0;
		}
		fputc( str[pos], fx);
	}
	assert(baseId != N);
	fwrite( &baseId, sizeof(unsigned), 1, fx);
	return 0;
}


//	DECODING	//

void Archon::roll(const unsigned i)	{
	P[i] = R[str[i]]++;
	assert(N==1 || i != P[i]);
}

int Archon::de_read(FILE *const fx, unsigned ns)	{
	en_read(fx,ns);
	int ok = fread( &baseId, sizeof(unsigned), 1, fx );
	assert(ok && baseId<N);
	return N;
}

int Archon::de_compute()	{
	unsigned i,k;
	// compute buchet heads
	memset( R, 0, 0x100*sizeof(unsigned) );
	for(i=0; i!=N; ++i)
		R[str[i]] += 1;
	for(k=N,i=0x100; i--;)
		R[i] = (k-=R[i]);
	// fill the jump table
#	ifndef	NDEBUG
	memset( P, 0, (N+1)*sizeof(unsigned) );
#	endif
	for(i=0; i<baseId; i++)		roll(i);
	for(i=baseId+1; i<N; i++)	roll(i);
	roll(baseId);
	return 0;
}

int Archon::de_write(FILE *const fx)	{
	unsigned i, k=baseId;
	for(i=0; i!=N; ++i,k=P[k])
		putc(str[k],fx);
	assert( k==baseId );
	return 0;
}
