#include <assert.h>
#include <memory.h>
#include "archon.h"

static const int dwsize = sizeof(dword);
static const int kInsert = 1<<2;
static const int kRadix = 1<<17;

#define p4b(ind)	*(dword*)(bin+(ind))
#define p2b(ind)	*(word*)(bin+(ind))
static int RX[0x10002];

static int isab(const suffix a, const suffix b, const byte* bin)	{
	for(;;)	{
		const dword q = p4b(a);
		const dword w = p4b(b);
		if(q != w) return q>w;
		bin -= dwsize;
	}
	return -1;
}

static void sort_insert(suffix *const A, suffix *const B, const byte *const bin)	{
	suffix *x = A+1;
	while(x<B)	{
		suffix *z = x++;
		const suffix s = *z;
		while(--z>=A && isab(z[0],s,bin))
			z[1] = z[0];
		z[1] = s; //in place
	}
}

static void sort_bese(suffix *A, suffix *B, const byte *bin)	{
	while(B-A > kInsert)	{
		suffix s = A[B-A>>1]; //median(A[0],A[B-A>>1],B[-1],bin);
		const dword w = p4b(s);
		suffix *x=A,*y=A,*z=B;
		do	{
			const dword q = p4b(s=*y);
			if(q < w)	{
				*y++ = *x; *x++ = s;
			}else
			if(q > w)	{
				*y = *--z; *z = s; 
			}else y++;
		}while(y<z);
		if(A+1 < x) sort_bese(A,x,bin);
		if(z+1 < B) sort_bese(z,B,bin);
		A = x; B = z; bin -= dwsize;
	}
	sort_insert(A,B,bin);
}

void sort_lucky(suffix *const A, suffix *const B, const byte *bin)	{
	sort_bese(A, B, bin-dwsize);
}

void sort_stable_radix(suffix *const a, const int n, suffix *const b, const int m, const byte *const bin)	{
	int i,size, saved = n;
	int last_key = 0xFFFF;
	/*int *R = NULL;
	static int xxx = 0;
	if(!xxx)	{
		memset(RX,0,sizeof(RX));
		R = RX+1; ++xxx;
		for(i=0; i!=n; ++i)
			R[get2key(a[i])]++;
		R[0x10000] = n;
	}else	{ R=RX;
		assert(RX[0x10000] == n);
		for(size=n,i=0x10000; --i>=0; )
			size -= (R[i]=size-R[i]);
		assert(!size);
		R[get2key(-1)]--;
		R[get2key(-0)]--;
		R[get2key(n-1)]++;
		R[get2key(n-0)]++;
	}*/
	memset(RX,0,sizeof(RX));
	for(i=0; i!=n; ++i)
		RX[p2b(a[i])]++;
	RX[0x10000] = n;
	for(size=n,i=0x10000; --i>=0; )
		RX[i] = (size -= RX[i]);
	assert(!size);

	for(size=n; size;)	{
		int have = 0, last=0;
		for(i=0; i!=size; ++i)	{
			const suffix s = a[i];
			int *const pr = RX + p2b(s);
			const int off = (*pr)++ -size +m;
			assert(off < m);
			if(off >= 0)	{
				b[off] = s; ++have;
			}else a[i-have] = s;
		}
		size -= have; assert(size >= 0);
		memcpy(a+size, b+m-have, have*sizeof(suffix));
		for(i=0; RX[i]<size; ++i)	{
			const int cur = RX[i];
			RX[i] = last; last = cur;
		}
		if(i != last_key)	{
			RX[last_key] = saved;
			saved = RX[last_key = i];
		}
		RX[i] = last;
	}
	RX[last_key] = saved;
}
