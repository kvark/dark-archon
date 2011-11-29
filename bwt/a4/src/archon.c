 /*
*	archon.c	(C) kvark, 2007
*		Archon project
*	Suffix Array sorting algorithm.
*	Method:	Two-stage (IT-2)
*/
#include <assert.h>
#include "direct.h"
#include "archon.h"

#define REVERSED
#define USE_IT2

static void timetool(int num)	{
	#ifdef VERBOSE
	static const int dots = 10;
	static int total = 0, next = 0;
	for(;num<0 || num>next; next += (total+dots)/(dots+1))	{
		if(num > next)	{
			putchar('+'); continue;
		}//now num<0
		total = -num; next = 0;
		putchar('[');
		for(num=1; num<=dots; num++)
			putchar('-');
		putchar(']');
		for(num=0; num<=dots; num++)
			putchar('\b');
	}
	#endif	//VERBOSE
}

#define NBIT	8
#define HI(num)	((int)(num)<<NBIT)
#define NSYM	HI(1)
#define LO(num)	(num&(NSYM-1))
#define N2SYM	HI(NSYM)

static int rx[NSYM], *r2, *ra;
byte *gin;	//input byte array
static int *p;	//suffix array itself
static int n, base, sorted;	//numbers

void getbounds(int id, int **top, int **bot)	{
	//works even with USE_IT2 disabled
	id = *((word*)(gin+DEAD+id)-1);
	*top = p+r2[id]; *bot = p+ra[id]-1;
}


int sufcheck(int *A, int num, char verb)	{
	#ifdef VERBOSE
	float sum = 0.0f;
	#else	//VERBOSE
	int sum = 0;
	#endif	//VERBOSE
	int i,k=0;
	if(num <= 1) return 0;
	timetool(-num);
	for(i=0; i<num-1; i++)	{
		int com,rez;
		timetool(i);
		com = compare(A[i],A[i+1],&rez);
		sum += com; if(!rez) k++;
		if(k && verb=='-') break;
	}
	if(verb!='+') return k;
	#ifdef VERBOSE
	printf("\nresult: %d faults\n",k);
	printf("Neighbor LCP: %.2f\n",sum/(num-1));
	#endif	//VERBOSE
	return 0;
}

int geninit(FILE *ff)	{
	extern int memory;
	n = ftell(ff);
	memory = sizeof(rx);
	gin = GETMEM(DEAD+n+1,byte);
	p = GETMEM(n+1,int);
	if(!p || !gin) return 0;
	fseek(ff,0,SEEK_SET);
	fread(gin+DEAD,1,n,ff);
	return memory;
}
int gencode()	{
	extern int memory;
	memset(gin,0,DEAD);
	r2 = GETMEM(N2SYM+1,int);
	ra = GETMEM(N2SYM+1,int);
	if(!r2 || !ra) return -1;
	if(!ankinit(n)) return -2;
	sorted = 0;
	return memory;
}
#ifdef VERBOSE
static void printeff(char *str, int use, int rit, int let, int dif)	{
	int log; if(!sorted || !let || !use) return;
	for(log=0; use>>log; log++);
	printf("%s count:2^%02d\t usage:%.1f%c\t eff:%.1f%c\t dif:%d\n",
		str, log, 100.f*rit/sorted, '%', 100.f*rit/let, '%', dif/use);
}
void genprint()	{
	printf("base: %d\t sorted: %.1f%c\n", base, 100.f*sorted/n, '%');
	ankprint(printeff);
}
#else	//VERBOSE
void genprint()	{}
#endif	//VERBOSE

void genexit()	{
	ankexit();
	FREE(r2); FREE(ra);
	FREE(gin); FREE(p);
}

static void sortbuckets()	{
	enum {POST=sizeof(dword)};
	int px[2] = {1,0}; byte cl = 0;
	ray(px,2,POST); timetool(-n);
	do	{ int ch,high = HI(cl+1);
		if(rx[cl]+1 >= rx[cl+1]) continue;
		ch = HI(cl); do	{
			int num = ra[ch] - r2[ch];
			if(num < 2) continue;
			ray(p+r2[ch], num, sizeof(word)+POST);
			sorted += num;
		}while(++ch != high);
		timetool(rx[cl]);
	}while(++cl);
	timetool(n);
}

int compute()	{
	//returns: base>=0 on success
	byte *in = gin+DEAD;
	if(!n) return -1; base = -1;
	#ifndef REVERSED
	/*DO: reverse */	{	
		byte *x,*z,tm;
		REVER(in,in+n,tm);
	}
	#endif	//!REVERSED
	memset(rx,0,NSYM*sizeof(int));
	memset(r2,0,N2SYM*sizeof(int));
	/*DO: count frequences */	{
		byte *fly,*fin=in+n-1;
		for(fly=in; fly<fin; fly++)
			r2[*(word*)fly]++;
	}
	/*DO: shift frequences */	{
		int i,ch = N2SYM;
		r2[ch] = i = n;
		do	{ int cl = NSYM;
			do	{ i -= r2[--ch];
				ra[ch] = r2[ch] = i;
			}while(--cl);
			rx[cl = ch>>NBIT] = i;
			if(cl == *in) p[--i]=1,--r2[ch];
		}while(ch);
	}
	#ifdef USE_IT2
	/*DO: get direct sort values */	{
		byte *fly=in,*fin=in+n;
		*fin = NSYM-1;
		do if(fly[0]>fly[1] && fly[0]>=fly[-1])
			p[ra[*(word*)fly]++] = fly+2-in, ++fly;
		while(++fly<fin);
	}
	sortbuckets();	//direct sort

	/*DO: two wave sort */	{
		byte cl; int i;
		memcpy(ra,r2+1,N2SYM*sizeof(int));
		in[n] = NSYM-1;	//Right2Left wave
		cl=0; do	{
			int lim = (--cl,ra[HI(cl)+cl]);
			for(i=r2[HI(cl+1)]; i-->lim;)	{
				int cc = in[p[i]];
				if(cc <= cl) p[--ra[HI(cc)+cl]] = p[i]+1;
			}
			for(lim = ra[HI(cl)+cl]; i>=lim; --i)
				if(in[p[i]] == cl) p[--lim] = p[i]+1;
		}while(cl);
		in[n] = in[0];	//Left2Right wave
		i=0; cl=0; do	{
			int ch = r2[HI(cl+1)]-r2[HI(cl)];
			while(ch--)	{ int pos;
				byte sym = in[pos = p[i++]];
				if(pos==n) base = i-1;
				else if(sym >= cl) p[rx[sym]++] = pos+1;
			}
		}while(++cl);
	}
	#else	//USE_IT2
	/*DO: total direct sort */	{
		/*DO: count stats */	{
			byte *fly,*fin=in+n-1;
			for(fly=in; fly<fin; fly++)
				p[ra[*(word*)fly]++] = fly+2-in;
		}
		sortbuckets();
		base = r2[*(word*)(in+n-2)];
		while(p[base]!=n) base++;
	}
	#endif	//USE_IT2
	printf("\n"); return base;
}

int verify()	{
	//returns: 0 on success
	int i = n; byte sym;
	byte *in = gin+DEAD;
	//i = sufcheck(p,n,'+');
	memset(rx,0,NSYM*sizeof(int));
	while(i--) rx[in[p[i]-1]]=i;
	timetool(-n); rx[in[0]]++;
	for(i=0; i<n; i++)	{
		if(p[i]==n) continue;
		sym = in[p[i]];
		if(p[rx[sym]] != 1+p[i]) break;
		rx[sym]++; timetool(i);
	}
	timetool(n); return n-i;
}

int encode(FILE *ff)	{
	byte *in = gin+DEAD;
	int i; in[n] = in[0];
	for(i=0; i!=n; ++i)
		putc(in[p[i]],ff);
	fwrite(&base,sizeof(int),1,ff);
	return 0;
}

int decode(FILE *ff)	{
	int i,k; byte *in = gin+DEAD;
	n -= sizeof(int);
	if(n < 0) return -1;
	base = *(int*)(in+n);
	
	memset(rx,0,NSYM*sizeof(int));
	for(i=0; i<n; i++) rx[in[i]]++;
	k = NSYM; do	{ --k;
		i-=rx[k]; rx[k]=i;
	}while(k);
	#define INEX	rx[in[i]]++
	#ifdef REVERSED
	#define IOPER	p[i]=INEX
	k = base;
	#else	//REVERSED
	#define IOPER	p[INEX]=i
	k = p[base];
	#endif	//REVERSED
	i=base; IOPER;
	for(i=0; i<base; i++)	IOPER;
	for(i=base+1; i<n; i++)	IOPER;
	for(i=n; i--; k=p[k]) putc(in[k],ff);
	#undef IOPER
	#undef INEX
	return n;
}
