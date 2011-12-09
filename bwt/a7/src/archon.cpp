#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "archon.h"

//	Memory requirements:		7n
//	Execution time complexity:	O(n)

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

template<typename T>
class	Constructor	{
	T *const data;
	suffix *const P;
	index *const R, *const RE, *const R2;
	const index N, K;
	index n1, name;
	const int strategy;

	enum {
		MASK_LMS	= 1U<<31U,
		MASK_UP		= 1U<<30U,
		MASK_SUF	= MASK_UP-1U,
	};

	int decide() const	{
		index bins[4] = {0,1,0,0};
		byte mask = 3U;
		for(index i=1; 0 && i<N; ++i)	{
			const int diff = (data[i-1]<<1) + (int)(mask&1) - (data[i]<<1);
			mask = (mask<<1U) + (diff>0 ? 1U:0U);
			++bins[mask&3U];
		}
		return 0;
	}

	void checkData();

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

	void valueLMS()	{
		// using area of P[n1,N)
		index i,j;
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
		index prev_len = 0;
		suffix prev = 0;
		for(name=0,i=0; i!=n1; ++i)	{
			const suffix cur = P[i];
			index &cur_len = P[n1+(cur>>1)];
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
		assert(n1+n1<=N);
		// sort by induction (evil technology!)
		induce_1();
		// pack LMS into the first n1 suffixes
		for(j=i=0; ;++i)	{
			const suffix suf = P[i] & MASK_SUF;
			assert(suf>0 && i<N);
			if(P[i] & MASK_LMS)	{
				P[j] = suf;
				if(++j == n1)
					break;
			}
		}
		// value LMS suffixes
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
		valueLMS();
		// pack values into the last n1 suffixes
		for(i=N,j=N; ;)		{
			assert( i>n1 );
			if(P[--i])	{
				P[--j] = P[i]-1U;
				if(j == N-n1)
					break;
			}
		}
	}

	void solve()	{
		suffix *const s1 = P+N-n1;
		if(name<n1)	{
			Constructor<suffix>( s1, P, n1, R, name );
		}else	{
			// permute back from values into indices
			assert(name == n1);
			for(index i=0; i<n1; ++i)
				P[s1[i]] = i+1U;
		}
	}

	void derive_1()	{
		suffix *const s1 = P+N-n1;
		index i,j;
		// get the list of LMS strings
		// LMS number -> actual string number
		//note: going left to right here!
		for(i=0,j=0; ++i<N; )	{
			if(data[i-1U] >= data[i])	
				continue;
			assert( n1+j<N );
			s1[j++] = i;
			while(++i<N && data[i-1U] <= data[i]);
		}
		assert(j==n1);
		// update the indices in the sorted array
		// LMS index -> string index
		for(i=0; i<n1; ++i)	{
			j = P[i];
			assert(j>0 && j<=n1);
			P[i] = s1[j-1U];
		}
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
	//	Ternary Qsort	//

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
					const suffix t = A[num>>1] = A[num-1];
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
						if(++y == z) break;
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


	void reduce_2()	{
		assert(!"implemented");
	}

	void reduce_3()	{
	}

	void induce_2()	{
	}

	void induce_3()	{
	}

	void derive_2()	{
	}

	void derive_3()	{
	}

public:
	Constructor(T *const _data, suffix *const _P, const index _N, index *const _R, const index _K)
	: data(_data), P(_P)
	, R(_R), RE(_R+1), R2(sBuckets.obtain(_K-1))
	, N(_N), K(_K), n1(0), name(0), strategy(decide())	{
		assert( N<=MASK_SUF );
		checkData();
		if(R2)	{
			makeBuckets();
			memcpy( R2, RE, (K-1)*sizeof(index) );
		}
		switch(strategy)	{
		case 0:
			directSort();
			break;
		case 1:
			reduce_1();
			solve();
			derive_1();
			break;
		case 2:
			reduce_2();
			solve();
			derive_2();
		case 3:
			reduce_3();
			solve();
			derive_3();
		}
		
	}
};

template<>	void Constructor<byte>::checkData()	{
	assert( K==0x100 );
	assert( sizeof(sBuckets) >= K*sizeof(index) );
}
template<>	void Constructor<suffix>::checkData()	{
	for(index i=0; i<N; ++i)
		assert(data[i]>=0 && data[i]<K);
}


//--------------------------------------------------------//

//	INITIALIZATION	//

Archon::Archon(const index Nx)
: P(new suffix[Nx+0x102])
, R(new index[((Nx>>9) ? (Nx>>1) : 0x100)+1])
, str(new byte[Nx+1])
, Nmax(Nx), N(0), baseId(0) {
	assert(P && R && str);
}

Archon::~Archon()	{
	delete[] P;
	delete[] R;
	delete[] str;
}


//	ENCODING	//

int Archon::en_read(FILE *const fx, index ns)	{
	assert(ns>=0 && ns<=Nmax);
	N = fread( str, 1, ns, fx );
	return N;
}

int Archon::en_compute()	{
	sBuckets.reset();
	Constructor<byte>( str, P, N, R, 256 );
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
	P[i] = R[str[i]]++;
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
