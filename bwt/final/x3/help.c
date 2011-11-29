/*
	Archon3r2 subroutines for stringsorting
	Fixed bug: left border (experimantal)
*/

#include "total.h"
#include <stdlib.h>
#include <limits.h>

#define INSERT	10
#define DEEP	150
#define MAXST	64
#define ABIT	7
const int AMASK	= (1<<ABIT)-1;

int call_indiana, call_fw_anch, call_bk_anch;
int call_fw_buck, call_split, call_pseudo;
int call_digger, call_deep_ray, call_smart_ins;

struct TLonStr	{
	uchar *sa;
	int len,off;
	char up;
}lst[MAXST+1];
struct TLonStr *milen,*mac,*lc;

int ndis,numst,lucky;
uchar *offs; int **anch;

int init_hardcore()	{ lucky++;
	offs = (uchar *)malloc(ndis);
	anch = (int **)malloc(ndis*sizeof(int*));
	if(!offs || !anch) return -1;
	memset(offs, 0, ndis*sizeof(uchar));
	memset(anch, 0, ndis*sizeof(int*));
	return 0;
}
void exit_hardcore()	{ lucky=0;
	free(offs); free(anch);
}
int StartHelp(int n)	{
	numst = lucky = 0;
	ndis = (n+AMASK)>>ABIT;
	lst[0].len = n; numst=0;
	lst[0].sa = substring(-4);
	return init_hardcore();
}
void StopHelp()	{
	if(lucky) exit_hardcore();
}

/*
*	isab - smart string compare routine
*/
int isab(register uchar *a,register uchar *b)	{
	int cof,i; uchar *bx;
	cof = (a>b ? a-(bx=b) : b-(bx=a));
	//if(!cof) return -1;
	mac = milen = lst; //choose period
	for(lc=lst+1; lc <= lst+numst; lc++)	{
		if(lc->len < milen->len) milen=lc;
		if(lc->off == cof  &&  lc->sa - lc->len < bx)
			if(lc->sa > mac->sa) mac=lc;
	}//continue until border
	for(i = bx-mac->sa; i>=0; i-=4)	{
		if(p4b(a) != p4b(b)) break;
		a-=4; b-=4;
	}//replace old bound
	assert(getdeep(a)<4 || getdeep(b)<4);
	assert(getdeep(a)<320 && getdeep(b)<320);

	if(i>0 || mac==lst)	{
		int rez = (p4b(a)>p4b(b) || b<=lst[0].sa);
		int clen = bx - (a>b?b:a) - 4;
		if(numst < MAXST)	{
			milen = lst+(++numst);
			milen->len = 0;
		}//replace-add
		if(clen > milen->len)	{
			milen->sa = bx;
			milen->len = clen;
			milen->off = cof;
			milen->up = (rez == (a>b));
		}return rez;
	}//update bound
	if(bx > mac->sa)	{
		mac->len += bx-mac->sa;
		mac->sa = bx;
	}return (mac->up == (a>b));
}

int median(int a,int b,int c,uchar bof[])	{
	uint qa = p4b(a+bof), qb = p4b(b+bof), qc = p4b(c+bof);
	if(qa > qb)	return (qa<qc ? a : (qb>qc?b:c));
	else		return (qb<qc ? b : (qa>qc?a:c));
}
/*
*	deep_ray - the deep BeSe implementation
*	key is uint64 instead of uint32
*/
void deep_ray(int *A, int *B, uchar *boff)	{
	register int *x,*y,*z; ulong w,w2;
	call_deep_ray++;
	while(B-A > INSERT)	{
		int s = median(A[0],A[B-A>>1],B[-1],boff);
		//not needed because of 'median' style
		//if(getdeep(boff+s) >= 0) s = A[1];
		x=A; y=A; z=B;
		w = p4b(s+boff); w2 = p4b(s-4+boff);
		while(y<z)	{
			register uint q;
			uchar *sp = (s=*y)+boff;
			assert(getdeep(sp)<320);
			q = p4b(sp);
			if(q == w)	{ q = p4b(sp-4);
				if(q > w2)	{
					*y = *--z; *z = s;
				}else if(q < w2 || getdeep(sp) >= 0)	{
					*y++ = *x; *x++ = s;
				}else y++;
			}else if(q < w)	{
				*y++ = *x; *x++ = s;
			}else	{ // q > w
				*y = *--z; *z = s;
			}
		}//recurse
		if(A+1 < x) deep_ray(A,x,boff);
		if(z+1 < B) deep_ray(z,B,boff);
		A = x; B = z; boff -= 8;
	}//insertion
	for(x=A+1; x<B; x++)	{
		int s = *(z=x);
		while(--z>=A && isab(boff+z[0],boff+s))
			z[1] = z[0];
		z[1] = s; //in place
	}
}

int icmp(const void *v0, const void *v1)	{
	return *(int*)v0 - *(int*)v1;
}
/*
*	indiana - general anchor sort
*/
void indiana(int A[], int B[], const int dif, int an[])	{
	int *x,*z,*hi,*lo; int pre,num=B-A;
	call_indiana++; //assert(lucky);
	qsort(A,num,sizeof(int),icmp);
	getlimits(A[0]-dif,&lo,&hi);
	an[0]=~an[0]; //now pre = suffixes left
	for(x=z=an, pre=num-1;;)	{ int id;
		while(x > lo && (id=dif+*--x,bsearch(&id,A,num,sizeof(int),icmp)))
			if(x[0]=~x[0],!--pre) goto allok;
		while(z < hi && (id=dif+*++z,bsearch(&id,A,num,sizeof(int),icmp)))
			if(z[0]=~z[0],!--pre) goto allok;
		assert(x>lo || z<hi);
	}allok:
	//if(z-x > (B-A<<2)) fprintf(fd,"\n%d/%d %d",B-A,z-x,dif);
	for(z=A,x--; z<B; z++)	{
		do x++; while(x[0]>=0);
		*z = (x[0]=~x[0]) + dif;
	}
}

/*
*	split - splits suffixes into 3 groups according
*	to the first 'ofs' characters from 'bof'
*/
int * split(int *A, int *B,uchar *bof,int ofs,int piv,int **end)	{
	call_split++;
	piv+=ofs; do	{ int *x,*y,*z;
		ulong w = p4b(bof + piv);
		x=y=A,z=B; do	{ int s = *y;
			ulong q = p4b(bof+s);
			assert(getdeep(bof+s)<320);
			if(q == w) y++;
			else if(q<w || getdeep(bof+s)>=4)	{
				*y++ = *x; *x++ = s;
			}else	{ // q>w
				*y = *--z; *z = s;
			}
		}while(y<z);
		A = x; B = y; bof -= 4;
	}while(A<B && (ofs-=4) > 0);
	end[0] = B; return A;
}

#define MAXDIFF		32
#define MAXLINDIFF	0
/*
*	digger - deep routines guider
*	chooses anchor or calls deep_ray
*/
void digger(int A[], int B[], uchar bof[])	{
	int min_fw,min_bk,min_cr,diff;
	int *x,*afw=0,*abk=0,*acr=0;
	if(B-A <= 1) return;
	if(!lucky) init_hardcore();
	call_digger++;
	min_fw = min_cr = INT_MAX; min_bk = INT_MIN;
	for(x=A; x<B; x++)	{
		uchar *bp; int *an;
		int off,tj = x[0]>>ABIT;
		if((an = anch[tj]) == 0) continue;
		off = (tj<<ABIT)+offs[tj];
		bp = substring(off);
		if(bp[-1] > bp[0]) continue;
		diff = x[0]-off;
		if(p2b(bp) == getcurpok())	{
			if(diff>0 && diff<min_cr)
				min_cr=diff,acr=an;
		}else
		if(diff > 0)	{
			if(diff < min_fw) min_fw=diff,afw=an;
		}else	if(diff > min_bk) min_bk=diff,abk=an;
	}diff = 0;
	//if forward anchor sort
	if(afw && min_fw < MAXDIFF)	{
		call_fw_anch++;
		indiana(A,B,diff=min_fw,afw);
	}else //if backward sort
	if(abk && min_bk > -MAXDIFF)	{ int i=0;
		for(x=A+1; x<B && !i; x++)
			for(i=-min_bk; i>0 && subequ(A,x,i); i--);
			if(!i)	{ call_bk_anch++;
				indiana(A,B,diff=min_bk,abk);
			}
	}else //same bucket
	if(acr && min_cr < MAXDIFF)	{ int *z;
		x = split(A,B,bof,min_cr,*acr,&z);
		if(x+1 < z)	{ call_fw_buck++;
			indiana(x,z,diff=min_cr,acr);
		}else diff = -1;
		if(A+1 < x) deep_ray(A,x,bof);
		if(z+1 < B) deep_ray(z,B,bof);
	}//pseudo or deep
	if(!diff)	{ int pre,s,tj;
		uchar *spy = substring(A[0]);
		for(diff=0; diff<MAXLINDIFF; diff++,spy--)
			if(BADSUF(spy) && (pre=p2b(spy))<getcurpok())
				if(poksize(pre) > (B-A>>4)) break;
		if(diff < MAXLINDIFF)	{ call_pseudo++;
			s = A[0]-diff; tj = s>>ABIT;
			//assert(s>=0 && s<n);
			acr = findsuf(pre,s);
			//assert(acr < p+r2a[pre]);
			anch[tj] = acr; offs[tj] = s&AMASK;
			indiana(A,B,diff,acr);
		}else deep_ray(A,B,bof);
	}//update anchors
	for(x=A; x<B; x++)	{
		int tj = x[0] >> ABIT;
		diff = x[0] & AMASK;
		if(!anch[tj] || offs[tj]>diff)	{
			anch[tj] = x; offs[tj] = diff;
		}
	}
}
#undef MAXDIFF
#undef MAXLINDIFF

/*
*	smart_ins - insertion sort limited with DEEP
*	calls 'digger' to sort bad substrings
*/
void smart_ins(int A[], int B[], uchar *bof)	{
	register int *x=A+1,*z;
	int badcase = 0;
	do	{ int s = *x; z=x-1;
		do	{
			int limit = (DEEP+3)>>2;
			register uchar *a,*b;
			a = bof + z[0]; b = bof+s;
			while(p4b(a)==p4b(b) && --limit) a-=4,b-=4;
			if(p4b(a) < p4b(b) || getdeep(a) >= 4)
				break;
			if(!limit)	{
				z[0]=~z[0];
				badcase++; break;
			}//shift
			do z[1] = z[0];
			while(--z>=A && z[0]<0);
		}while(z>=A);	
		z[1] = s; //put in place
	}while(++x<B);
	if(badcase)	{ bof -= DEEP;
		call_smart_ins++;
		x=A; do	{ //skip bad
			for(z=x; z[0]<0; z++) z[0]=~z[0];
			if(x+1 < ++z) digger(x,z,bof);
		}while((x=z)<B);
	}
}

/* 
*	ray - the modified mkqsort
*	deep = bin-boff, dword comparsions
*/
void ray(int *A, int *B, register uchar *boff)	{
	register int *x,*y,*z;
	while(B-A > INSERT)	{
		int s = median(A[0],A[B-A>>1],B[-1],boff);
		ulong w = p4b(s+boff);
		x=y=A; z=B; do	{
			ulong q = p4b((s=*y)+boff);
			if(q < w)	{
				*y++ = *x; *x++ = s;
			}else
			if(q > w)	{
				*y = *--z; *z = s; 
			}else y++;
		}while(y<z);
		if(A+1 < x) ray(A,x,boff);
		if(z+1 < B) ray(z,B,boff);
		A = x; B = z; boff-=4;
		if(getdeep(boff) > DEEP)	{
			digger(A,B,boff);
			return;
		}
	}//insertion
	if(A+1 < B) smart_ins(A,B,boff);
}

#undef INSERT
#undef DEEP
#undef MAXST
#undef ABIT