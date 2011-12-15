#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "archon.h"


//--------------------------------------------------------//
//	Bucket Storage class manages a small chunk of memory
//	to be used by small buckets
//option: use reserved area for that automatically!
//--------------------------------------------------------//

class BucketStorage	{
	enum	{
		SIZE_RESERVED	= 0x1000,
		SIZE_UNIT		= 0x400,
	};
	index arr[SIZE_RESERVED], have;
public:
	void reset()	{
		have = 0;
	}
	index* obtain(const index K)	{
		if (K<=SIZE_UNIT && have+K<=SIZE_RESERVED)	{
			have += K;
			return arr+have-K;
		}
		return NULL;
	}
}static sBuckets;


//--------------------------------------------------------//
//	Universal data key class, used as SAC input
//--------------------------------------------------------//
static const unsigned sKeyMask = 7;

template<int SIZE>
class Key	{
	enum { MASK = (2<<(SIZE*8-1))-1 };
	byte data[SIZE];
public:
	Key& operator=(const unsigned v)	{
		memcpy( data, &v, SIZE );
		return *this;
	}
	Key(const unsigned &v)	{
		*this = v;
	}
	operator unsigned() const	{
		return *reinterpret_cast<const unsigned*>(data) & MASK;
	}
};

template<> Key<1>& Key<1>::operator=(const unsigned v)	{
	assert(!(v>>8));
	data[0] = v;
	return *this;
}
template<> Key<2>& Key<2>::operator=(const unsigned v)	{
	assert(!(v>>16));
	*reinterpret_cast<dbyte*>(data) = v;
	return *this;
}
template<> Key<3>& Key<3>::operator=(const unsigned v)	{
	assert(!(v>>24));
	data[0]=data[1]=data[2]=0;
	*reinterpret_cast<unsigned*>(data) += v & MASK;
	return *this;
}
template<> Key<4>& Key<4>::operator=(const unsigned v)	{
	*reinterpret_cast<word*>(data) = v;
	return *this;
}

template<> Key<1>::operator unsigned() const	{
	return data[0];
}
template<> Key<2>::operator unsigned() const	{
	return *reinterpret_cast<const dbyte*>(data);
}


//--------------------------------------------------------//
// This SAC implementation is derived from ideas explained here:
// http://www.cs.sysu.edu.cn/nong/index.files/Two%20Efficient%20Algorithms%20for%20Linear%20Suffix%20Array%20Construction.pdf
//--------------------------------------------------------//

template<typename T>
class	Constructor	{
	const T *const data;
	suffix *const P;
	index *const R, *const RE, *const R2;
	const index N, K;
	index n1, name;

	enum {
		MASK_LMS	= 1U<<31U,
		MASK_UP		= 1U<<30U,
		MASK_SUF	= MASK_UP-1U,
	};

	//---------------------------------
	//	Strategy-0 implementation	//

	void directSort()	{
		for(index i=0; i!=N; ++i)
			P[i] = i+1;
		ray(P,N,1);
	}

	void ray(suffix *A, index num, unsigned depth)	{
		while(num>1)	{
			suffix *x,*z;
			{
				z = (x=A)+num;
				suffix s = A[num>>1];
				if(s<depth)	{
					assert(s+1==depth);
					if(num==2)
						return;
					const suffix t = A[num>>1] = A[0];
					*--z=s; s=t; --num;
				}
				assert(s>=depth);
				const T w = data[s-depth];
				suffix *y = x;
				for(;;)	{
					s = *y;
					if(s<depth)
						goto more;
					const T q = data[s-depth];
					if(q <= w)	{
						if(q != w) *y=*x,*x++=s;
						if(++y == z)
							break;
					}else	{
						more:
						if(--z == y) break;
						*y=*z,*z=s;
					}
				}
				y=z; z=A+num;
				num = y-x;
			}
			ray(A,x-A,depth); A = x+num;
			ray(A,z-A,depth); A = x;
			++depth;
		}
	}

	bool sufCompare(const suffix a, const suffix b)	{
		assert(a!=b);
		index d=0; do ++d;
		while(a>=d && b>=d && data[a-d]==data[b-d]);
		return a>=d && (b<d || data[a-d]<data[b-d]);
	}

	bool bruteCheck(suffix *const A, suffix *const B)	{
		for(suffix *x=A; ++x<B; )
			assert(sufCompare( x[-1], x[0] ));
		return true;
	}

	//---------------------------------
	//	Commonly used routines	//

	void checkData()	{
		if(sizeof(T) == sizeof(index))
			return;
		byte shift = sizeof(T)<<3;
		assert(!( (K-1)>>shift ));
		if(K>>shift)
			return;
		for(index i=0; i<N; ++i)
			assert(data[i]>=0 && data[i]<K);
	}

	bool checkUnique(const index num)	{
		for(index i=0; i!=num; ++i)
			for(index j=i+1; j!=num; ++j)
				if(P[i]==P[j])
					return false;
		return true;
	}

	void makeBuckets()	{
		memset( R, 0, K*sizeof(index) );
		index i,sum;
		for(i=0; i<N; ++i)
			++R[data[i]];
		for(R[i=K]=sum=N; i--;)
			R[i] = (sum -= R[i]);
		assert(!sum);
	}

	void buckets()	{
		if(R2)	{
			memcpy( RE, R2, (K-1U)*sizeof(index) );
			R[0] = 0; R[K] = N;
		}else
			makeBuckets();
	}

	void packTargetIndices()	{
		index i,j;
		// pack LMS into the first n1 suffixes
		if(!n1)
			return;
		for(j=i=0; ;++i)	{
			const suffix suf = P[i] & MASK_SUF;
			assert(suf>0 && i<N);
			if(P[i] & MASK_LMS)	{
				P[j] = suf;
				if(++j == n1)
					break;
			}
		}
	}

	void computeTargetValues()	{
		// compare LMS using known lengths
		suffix *const s1 = P+n1+1;
		index i, prev_len = 0;
		suffix prev = 0;
		for(name=0,i=0; i!=n1; ++i)	{
			const suffix cur = P[i];
			index &cur_len = s1[cur>>1];
			assert(cur_len);
			//todo: work around the jump
			if(cur_len == prev_len)	{
				index j=1; do	{
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

	//---------------------------------
	//	Strategy-1 implementation	//

	// here is the slowest part of the method!
	//todo: use templates/separate function
	// to remove unnecessary checks
	// on the second call to the function
	//todo: use buckets to traverse the SA efficiently
	// if R2 is available
	void induce_1()	{
		const index NL = N-1U;
		// condition "(j-1U) >= NL" cuts off
		// all 0-s and the N at once
		index i;
		suffix add;
		assert(N);
		//left2right
		buckets();
		for(i=0; i!=N; ++i)	{
			//todo: fix to support 1Gb input
			const suffix j = P[i] & MASK_SUF;
			if((j-1U) >= NL)
				continue;
			const T cur = data[j];
			if(data[j-1U] <= cur)	{
				add = 0;
				if(data[j-1U] == cur)
					add = P[i] & MASK_UP;	
				P[R[cur]++] = j+1U + add;
			}
		}
		//right2left
		buckets();
		add = MASK_UP;
		if(N>1 && data[0]<data[1])
			add += MASK_LMS;
		P[--RE[data[0]]] = 1U + add;
		i=N; do	{
			const suffix j = P[--i] & MASK_SUF;
			if((j-1U) >= NL)
				continue;
			const T cur = data[j];
			if(data[j-1U] >= cur)	{
				add = 0;
				if(data[j-1U] != cur || (P[i] & MASK_UP))	{
					add = MASK_UP;
					if(j+1U<N && cur<data[j+1U])
						add += MASK_LMS;
				}
				P[--RE[cur]] = j+1U + add;
			}
		}while(i);
	}

	void computeTargetLengths_1()	{
		suffix *const s1 = P+n1+1;
		index i=0,j=0;	// using area of P[n1,N)
		// find the length of each LMS substring
		// and write it into P[n1+1+(x>>1)]
		// no collisions guaranteed because LMS distance>=2
		while(++i<N)	{
			if(data[i-1] >= data[i])	
				continue;
			s1[i>>1] = i-j;	//length
			j = i;
			while(++i<N && data[i-1] <= data[i]);
		}
	}

	void reduce_1()	{
		index i,j;
		// scatter LMS into bucket positions
		memset( P, 0, N*sizeof(suffix) );
		P[N] = 0 + MASK_UP;
		buckets();
		//todo: optimize more
		bool prevUp = true;
		for(j=N,n1=0;;)	{
			for(i=--j; j && data[j-1U]==data[i]; --j);
			if(j && data[j-1U]<data[i])	{
				prevUp = false;
			}else	{	//up
				if(!prevUp)	{
					++n1; // found LMS!
					P[--RE[data[i]]] = i+1U + MASK_LMS + MASK_UP;
					prevUp = true;
				}
				if(!j)
					break;
			}
		}

		// sort by induction (evil technology!)
		induce_1();

		// scatter into indices and values
		packTargetIndices();
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
		computeTargetLengths_1();
		computeTargetValues();
	}

	template<typename Q>
	void packTargetValues(Q *const input)	{
		if(!n1)
			return;
		// pack values into [0,n1] and
		// move suffixes into [n1,2*n1]
		suffix *const s1 = P+n1, *x=s1;
		for(index j=0; ;++x)		{
			const suffix val = *x;
			assert( x<P+N );
			if(val)	{
				s1[j] = P[j];
				input[j] = val-1U;
				if(++j == n1)
					break;
			}
		}
	}

	template<typename Q>
	void solve(const index reserve)	{
		Q *const input = reinterpret_cast<Q*>(P);
		packTargetValues(input);
		if(name<n1)	{
			assert(n1+n1<N);
			Constructor<Q>( input, P+n1, n1, name, reserve+N-n1 );
		}else	{
			// permute back from values into indices
			assert(name == n1);
			for(index i=n1; i--; )
				P[n1+input[i]] = i+1U;
		}
	}

	void derive_1()	{
		index i,j;
		// get the list of LMS strings
		// LMS number -> actual string number
		//note: going left to right here!
		for(i=0,j=0; ; )	{
			do	{ ++i;
				assert(i<N);
			}while(data[i-1U] >= data[i]);
			P[j] = i;
			if(++j==n1)
				break;
			do	{ ++i;
				assert(i<N);
			}while(data[i-1U] <= data[i]);
		}
		suffix *const s1 = P+n1;
		// update the indices in the sorted array
		// LMS index -> string index
		for(i=0; i<n1; ++i)	{
			j = s1[i];
			assert(j>0 && j<=n1);
			s1[i] = P[j-1U];
		}
		memcpy( P, P+n1, n1*sizeof(suffix) );
		// scatter LMS back into proper positions
		buckets();
		//option: generate buckets again here
		// but make R2 static
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
		T prev_sym = K-1;
		for(i=n1; i--; )	{
			j = P[i]; P[i] = 0;
			assert(j>0 && j<=N				&& "Invalid suffix!");
			assert(data[j-1U] <= prev_sym	&& "Not sorted!");
			index *const pr = RE+data[j-1U];
			P[--*pr] = j;
			assert(pr[0] >= pr[-1]	&& "Stepped twice on the same suffix!");
			assert(pr[0] >= i		&& "Not sorted properly!");
		}
		// induce the rest of suffixes
		induce_1();
		// clean up the masks (TEMPORARY!)
		for(i=0; i!=N; ++i)
			P[i] &= MASK_SUF;
	}

	//---------------------------------
	//	Main entry point	//

public:
	Constructor(const T *const _data, suffix *const _P, const index _N, const index _K, const index reserved)
	: data(_data), P(_P)
	, R(_P+_N+1), RE(R+1), R2(sBuckets.obtain(_K-1))
	, N(_N), K(_K), n1(0), name(0)	{
		assert( N<=MASK_SUF && K<reserved );
		checkData();
		if(R2)	{
			makeBuckets();
			memcpy( R2, RE, (K-1)*sizeof(index) );
		}
		// directSort();
		// reduce the problem to LMS sorting
		reduce_1();
		// solve the reduced problem
		if(!name)
			return;
		else if(name<=0x100		&& (sKeyMask&0x1))
			solve<byte>(reserved);
		else if(name<=0x10000	&& (sKeyMask&0x2))
			solve<dbyte>(reserved);
		else if(name<=0x1000000	&& (sKeyMask&0x4))
			solve< Key<3> >(reserved);
		else	// this never happens
			solve<unsigned>(reserved);
		// derive all other suffixes
		derive_1();
	}
};


//--------------------------------------------------------//

//	INITIALIZATION	//

index Archon::estimateReserve(const index n)	{
	const int need = 0x10001 - ((sKeyMask&4) ? n/12 : 0);
	return need>0x101 ? need : 0x101;
}

Archon::Archon(const index Nx)
: Nmax(Nx)
, Nreserve(estimateReserve(Nx))
, P(new suffix[Nmax+1+Nreserve])
, str(new byte[Nmax+1])
, N(0), baseId(0) {
	assert(P && str);
}

Archon::~Archon()	{
	delete[] P;
	delete[] str;
}

unsigned Archon::countMemory() const	{
	return Nmax + (Nmax+1+Nreserve)*sizeof(suffix);
}


//	ENCODING	//

int Archon::en_read(FILE *const fx, index ns)	{
	assert(ns>=0 && ns<=Nmax);
	N = fread( str, 1, ns, fx );
	return N;
}

int Archon::en_compute()	{
	sBuckets.reset();
	Constructor<byte>( str, P, N, 0x100, Nreserve );
	//const Key<1> *const input = reinterpret_cast<const Key<1>*>(str);
	//Constructor< Key<1> >( input, P, N, 0x100, Nreserve );
	return 0;
}

int Archon::en_write(FILE *const fx)	{
	baseId = N;
	for(index i=0; i!=N; ++i)	{
		suffix pos = P[i];
		if(pos == N)	{
			baseId = i;
			pos = 0;
		}
		fputc( str[pos], fx);
	}
	assert(baseId != N);
	fwrite( &baseId, sizeof(index), 1, fx);
	return 0;
}


//	DECODING	//

void Archon::roll(const index i)	{
	P[i] = P[N+str[i]]++;
	assert(N==1 || i != P[i]);
}

int Archon::de_read(FILE *const fx, index ns)	{
	en_read(fx,ns);
	int ok = fread( &baseId, sizeof(index), 1, fx );
	assert(ok && baseId<N);
	return N;
}

int Archon::de_compute()	{
	index i,k;
	// compute buchet heads
	index *const R = P+N;
	memset( R, 0, 0x100*sizeof(index) );
	for(i=0; i!=N; ++i)
		++R[str[i]];
	for(k=N,i=0x100; i--;)
		R[i] = (k-=R[i]);
	// fill the jump table
#	ifndef	NDEBUG
	memset( P, 0, (N+1)*sizeof(index) );
#	endif
	for(i=0; i<baseId; i++)		roll(i);
	for(i=baseId+1U; i<N; i++)	roll(i);
	roll(baseId);
	return 0;
}

int Archon::de_write(FILE *const fx)	{
	index i, k=baseId;
	for(i=0; i!=N; ++i,k=P[k])
		putc(str[k],fx);
	assert( k==baseId );
	return 0;
}
