#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "archon.h"


//--------------------------------------------------------//
// This SAC implementation is derived from ideas explained here:
// http://www.cs.sysu.edu.cn/nong/t_index.files/Two%20Efficient%20Algorithms%20for%20Linear%20Suffix%20Array%20Construction.pdf
//--------------------------------------------------------//

template<typename T>
class	Constructor	{
	const T	*const data;	// input string
	suffix	*const P;		// suffix array
	t_index	*const R,		// radix start
			*const RE,		// radix end
			*const R2;		// radix backup
	const	t_index N;		// input length
	const	t_index K;		// number of unique values
	t_index	n1;				// number of LMS
	t_index	d1;				// memory units occupied by the new data
	t_index	name;			// new number of unique values

	//---------------------------------
	//	Strategy-0 implementation	//

	void directSort()	{
		for(t_index i=0; i!=N; ++i)
			P[i] = i+1;
		ray(P,N,1);
	}

	void ray(suffix *A, t_index num, unsigned depth)	{
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
					if(s>=depth)	{
						const T q = data[s-depth];
						if(q <= w)	{
							if(q != w) *y=*x,*x++=s;
							if(++y == z)
								break;
							continue;
						}
					}
					if(--z == y) break;
					*y=*z,*z=s;
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
		t_index d=0; do ++d;
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
		if(sizeof(T) == sizeof(t_index))
			return;
		byte shift = sizeof(T)<<3;
		assert(!( (K-1)>>shift ));
		if(K>>shift)
			return;
		for(t_index i=0; i<N; ++i)
			assert(data[i]>=0 && data[i]<K);
	}

	bool checkUnique(const t_index num)	{
		for(t_index i=0; i!=num; ++i)
			for(t_index j=i+1; j!=num; ++j)
				if(P[i]==P[j])
					return false;
		return true;
	}

	void makeBuckets()	{
		memset( R, 0, K*sizeof(t_index) );
		t_index i,sum;
		for(i=0; i<N; ++i)
			++R[data[i]];
		for(R[i=K]=sum=N; i--;)
			R[i] = (sum -= R[i]);
		assert(!sum);
	}

	void buckets()	{
		if(R2)	{
			memcpy( RE, R2, (K-1U)*sizeof(t_index) );
			R[0] = 0; R[K] = N;
		}else
			makeBuckets();
	}

	void packTargetIndices()	{
		t_index i,j;
		// pack LMS into the first n1 suffixes
		if(!n1)
			return;
		for(j=i=0; ;++i)	{
			assert(i<N);
			const suffix s = ~P[i];
			if(static_cast<t_index>(s) < N)	{
				P[j] = s;
				if(++j == n1)
					break;
			}
		}
	}

	void computeTargetValues()	{
		// compare LMS using known lengths
		suffix *const s1 = P+n1;
		t_index i, prev_len = 0;
		suffix prev = 0;
		for(name=0,i=0; i!=n1; ++i)	{
			const suffix cur = P[i];
			suffix &cur_len = s1[cur>>1];
			assert(cur_len);
			//todo: work around the jump
			if(cur_len == prev_len)	{
				suffix j=1; do	{
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
	//todo: use buckets to traverse the SA efficiently
	// if R2 is available

	void induce()	{
		const t_index NL = N-1U;
		t_index i;
		T prev; suffix *pr=NULL;
		assert(N);
		//left2right
		buckets();
		pr = P + R[prev=0];
		for(i=0; i!=N; ++i)	{
			const suffix s = ~P[i];
			if(static_cast<t_index>(s-1) >= NL)
				continue;
			const T cur = data[s];
			if(data[s-1] <= cur)		{
				if(cur != prev)	{
					R[prev] = pr-P;
					pr = P + R[prev=cur];
				}
				*pr++ = ~(s+1);
				assert(R[data[s]] <= RE[data[s]]);
				P[i] = s;	//clear mask
			}
		}
		//right2left
		buckets();
		pr = P + RE[prev=data[0]];
		*--pr = ~1;
		i=N; do	{
			const suffix s = ~P[--i];
			if(static_cast<t_index>(s-1) >= NL)
				continue;
			const T cur = data[s];
			if(data[s-1] >= cur)		{
				if(cur != prev)	{
					RE[prev] = pr-P;
					pr = P + RE[prev=cur];
				}
				*--pr = ~(s+1);
				assert(R[data[s]] <= RE[data[s]]);
				P[i] = s;	//clear mask
			}
		}while(i);
	}

	// find the length of each LMS substring
	// and write it into P[n1+1+(x>>1)]
	// no collisions guaranteed because LMS distance>=2
	void computeTargetLengths()	{
		if(!n1)
			return;
		suffix *const s1 = P+n1;
		t_index i=0,j=0,k=0;	// using area of P[n1,N)
		for(;;)	{
			do	{ ++i; assert(i<N);
			}while(data[i-1] >= data[i]);
			s1[i>>1] = i-j;	//length
			j = i;
			if(++k == n1)
				break;
			do	{ ++i; assert(i<N);
			}while(data[i-1] <= data[i]);
		}
	}

	void reduce()	{
		assert(!n1 && N);
		// scatter LMS into bucket positions
		memset( P, 0, N*sizeof(suffix) );
		buckets();
		//todo: optimize more
		bool prevUp = true;
		for(t_index j=N;;)	{
			t_index i = j-1;
			while(--j && data[j-1U]==data[i]);
			if(j && data[j-1U]<data[i])	{
				prevUp = false;
				continue;
			}//up
			if(!prevUp)	{ // found LMS!
				++n1; prevUp = true; 
				P[--RE[data[i]]] = ~(i+1);
			}
			if(!j)
				break;
		}

		// sort by induction (evil technology!)
		induce();

		// scatter into indices and values
		packTargetIndices();
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
		computeTargetLengths();
		computeTargetValues();
	}

	template<typename Q>
	void packTargetValues(Q *const input)	{
		if(!n1)
			return;
		// number of words occupied by new data
		d1 = 1 + (sizeof(Q)*n1-1) / sizeof(suffix);
		assert(d1<=n1);
		// pack values into [0,m1] and
		// move suffixes into [m1,m1+n1]
		suffix *const s1 = P+d1, *x=P+n1;
		for(t_index j=0; ;++x)		{
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
	void solve(const t_index reserve)	{
		Q *const input = reinterpret_cast<Q*>(P);
		packTargetValues(input);
		if(name<n1)	{
			// count the new memory reserve
			t_index left = reserve;
			assert( n1+d1 <= N );
			left += N-n1-d1; // take in account new data and suffix storage
			if(R2)	{
				assert( left >= K-1 );
				left -= K-1;
			}
			// finally, solve the sub-problem
			Constructor<Q>( input, P+d1, n1, name, left );
		}else	{
			// permute back from values into indices
			assert(name == n1);
			for(t_index i=n1; i--; )
				P[d1+input[i]] = i+1;
		}
	}

	void derive()	{
		t_index i,j;
		memcpy( P, P+d1, n1*sizeof(suffix) );
		suffix *const s1 = P+n1;
		// get the list of LMS strings into [n1,2*n1]
		// LMS number -> actual string number
		//note: going left to right here!
		for(i=0,j=0; ; )	{
			do	{ ++i; assert(i<N);
			}while(data[i-1] >= data[i]);
			s1[j] = i;
			if(++j==n1)
				break;
			do	{ ++i; assert(i<N);
			}while(data[i-1] <= data[i]);
		}
		// update the indices in the sorted array
		// LMS t_index -> string t_index
		for(i=0; i<n1; ++i)	{
			assert(P[i]>0 && P[i]<=static_cast<suffix>(n1));
			P[i] = s1[P[i]-1];
		}
		// scatter LMS back into proper positions
		buckets();
		memset( P+n1, 0, (N-n1)*sizeof(suffix) );
		T prev_sym = K-1;
		suffix *pr = P + RE[prev_sym];
		for(i=n1; i--; )	{
			j = P[i]; P[i] = 0;
			assert(j>0 && j<=N				&& "Invalid suffix!");
			const T cur = data[j-1];
			if(cur != prev_sym)	{
				assert(cur<prev_sym		&& "Not sorted!");
				RE[prev_sym] = pr-P;
				pr = P + RE[prev_sym = cur];
			}
			*--pr = ~j;
			assert(pr[0] >= pr[-1]	&& "Stepped twice on the same suffix!");
			assert(pr[0] >= i		&& "Not sorted properly!");
		}
		RE[prev_sym] = pr-P;
		// induce the rest of suffixes
		induce();
		// fix the negatives
		for(i=0; i!=N; ++i)	{
			if(P[i]<0)
				P[i] = ~P[i];
		}
	}

	//---------------------------------
	//	Main entry point	//

public:
	Constructor(const T *const _data, suffix *const _P, const t_index _N, const t_index _K, const t_index reserved)
	: data(_data), P(_P), R(reinterpret_cast<t_index*>(_P+_N)), RE(R+1)
	, R2(reserved>=_K*2 ? reinterpret_cast<t_index*>(_P+_N+reserved-_K+1) : NULL)
	, N(_N), K(_K), n1(0), d1(0), name(0)	{
		assert( N>0 && K<reserved );
		checkData();
		if(R2)	{
			assert(R2 >= R+K+1);
			makeBuckets();
			memcpy( R2, RE, (K-1)*sizeof(t_index) );
		}
#		ifdef	SAIS_DISABLE
		directSort();
		return;
#		endif
		// reduce the problem to LMS sorting
		reduce();
		// solve the reduced problem
#		ifndef SAIS_COMPARE
		if(name<=0x100)
			solve<byte>(reserved);
		else if(name<=0x10000)
			solve<dbyte>(reserved);
		else
#		endif
			solve<unsigned>(reserved);
		// derive all other suffixes
		derive();
	}
};


//--------------------------------------------------------//

//	INITIALIZATION	//

t_index Archon::estimateReserve(const t_index n)	{
	return 0x10001;
}

Archon::Archon(const t_index Nx)
: Nmax(Nx)
, Nreserve(estimateReserve(Nx))
, P(new suffix[Nmax+Nreserve])
, str(new byte[Nmax+1])
, N(0), baseId(0) {
	assert(P && str);
	assert(sizeof(t_index) == sizeof(suffix));
}

Archon::~Archon()	{
	delete[] P;
	delete[] str;
}

unsigned Archon::countMemory() const	{
	return Nmax + (Nmax+Nreserve)*sizeof(suffix);
}


//	ENCODING	//

int Archon::en_read(FILE *const fx, t_index ns)	{
	assert(ns>=0 && ns<=Nmax);
	N = fread( str, 1, ns, fx );
	return N;
}

int Archon::en_compute()	{
	Constructor<byte>( str, P, N, 0x100, Nmax-N+Nreserve );
	return 0;
}

int Archon::en_write(FILE *const fx)	{
	baseId = N;
	for(t_index i=0; i!=N; ++i)	{
		suffix pos = P[i];
		if(pos == N)	{
			baseId = i;
			pos = 0;
		}
		fputc( str[pos], fx);
	}
	assert(baseId != N);
	fwrite( &baseId, sizeof(t_index), 1, fx);
	return 0;
}


//	DECODING	//

void Archon::roll(const t_index i)	{
	P[i] = P[N+1+str[i]]++;
	assert(N==1 || i != P[i]);
}

int Archon::de_read(FILE *const fx, t_index ns)	{
	en_read(fx,ns);
	int ok = fread( &baseId, sizeof(t_index), 1, fx );
	assert(ok && baseId<N);
	return N;
}

int Archon::de_compute()	{
	t_index i,k;
	// compute buchet heads
	t_index *const R = reinterpret_cast<t_index*>(P+N+1);
	assert( N+0x102 <= Nmax+Nreserve );
	memset( R, 0, 0x100*sizeof(t_index) );
	for(i=0; i!=N; ++i)
		++R[str[i]];
	for(k=N,i=0x100; i--;)
		R[i] = (k-=R[i]);
	// fill the jump table
#	ifndef	NDEBUG
	memset( P, 0, (N+1)*sizeof(t_index) );
#	endif
	for(i=0; i<baseId; i++)		roll(i);
	for(i=baseId+1U; i<N; i++)	roll(i);
	roll(baseId);
	return 0;
}

int Archon::de_write(FILE *const fx)	{
	t_index i, k=baseId;
	for(i=0; i!=N; ++i,k=P[k])
		putc(str[k],fx);
	assert( k==baseId );
	return 0;
}
