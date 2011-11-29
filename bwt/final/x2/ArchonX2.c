/*
*	Archon X2	(C) kvark, 2005
*		Archon project
*	Burrows-Wheeler Transformation algoritm
*
*		Last updated in November 2005
*		Direct sorting (a>b<=c) ~1/3
*		Units - fast dword
*		LSC tech. defence, no bswap
*/
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <memory.h>
#include <malloc.h>
#include <process.h>

#define INSERT	8
#define ASIG	'RA'

#define MAXST	4
#define LINGO	16
#define LINSIZE	((LINGO)*sizeof(uint))
#define TERM	(10+LINSIZE)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef int trax[256];
typedef trax trax2[257];

int n,baza,pos; int *p;
uchar *s_bin;
uchar *sfin, *bin, *fly;
trax rb; trax2 r,r2a;

clock_t t0;
FILE *fi,*fo;
int ch,ch2,i;
uchar sym;

void ray(int, int, uchar *);
void encode();
void decode();

uint q,w;
int s,cur,v;

struct AFileInfo	{
	ushort sig;
	int bs;
}afi;

void endall(signed char code)	{
	if(code>0) cprintf("\nAn error %d accured!", code);
	if(s_bin) free(s_bin);
	if(p) free(p);
	if(fi) fclose(fi);
	if(fo) fclose(fo);
	if(code)	{
		cprintf("\nCall format:");
		cprintf("\narchonx2 [e|d] [-bLog] <source> <dest>");
		cprintf("\nDefault block size = '-b22' = 4Mb");
		getch();
	}exit(code);
}

int main(int argc,char *argv[])	{
	int t0=clock();
	fi=fo=0; s_bin=0; p=0;
	if(argc<4) endall(-1);
	afi.bs = (4<<20);
	cprintf("\nArchon X2\t(C)kvark, 2005");
	for(i=2; i<argc; i++)	{
		if(argv[i][0]!='-')	{
			if(fi)	fo = fopen(argv[i],"wb");
			else	fi = fopen(argv[i],"rb");
		}else if(argv[i][1]=='b')	{
			sscanf(argv[i]+2,"%d",&ch);
			if(ch<=4 || ch>28) endall(11);
			afi.bs = 1<<ch;
		}
	}
	if(!fi || !fo) endall(1);
	cprintf("\nProgress: ");
	if(argv[1][0] == 'e')	{ afi.sig = ASIG;
		fwrite(&afi, sizeof(struct AFileInfo), 1, fo);
		p = (int *)malloc(afi.bs*sizeof(int));
		s_bin = (uchar *)malloc(afi.bs+TERM+1);
		memset(s_bin, 0xFF, TERM); bin = s_bin+TERM; 
		while(n = fread(bin,1,afi.bs,fi))	{
			fwrite(&n,4,1,fo);
			encode();
		}
	}else
	if(argv[1][0] == 'd')	{
		fread(&afi, sizeof(struct AFileInfo), 1, fi);
		if(afi.sig != ASIG) endall(3);
		p = (int *)malloc(afi.bs*sizeof(int));
		bin = (uchar *)malloc(afi.bs+1);
		while(fread(&n,1,4,fi))	{
			fread(bin,1,n+1,fi);
			fread(&baza,4,1,fi);
			decode();
		}
	}else endall(2);
	t0 = clock() - t0;
	cprintf("\tTime: %lums",t0);
	endall(0);
	return 0;
}

#define nextp(id) p[rb[sfin[-(id)]]++]=(id);

void decode()	{ sfin = bin+n;
	memset(rb, 0, sizeof(trax));
	for(i=0; i<=n; i++) rb[bin[i]]++;
	rb[sfin[-baza]]--;
	for(i=0,ch=0;ch<=256;ch++)
		i+=rb[ch], rb[ch]=i-rb[ch];
	putch('H');
	for(i=0; i<baza; i++)		nextp(i);
	for(i=baza+1; i<=n; i++)	nextp(i);
	for(pos=baza,i=0; i<n; i++)
		pos=p[pos], putc(sfin[-pos],fo);
}
#undef nextp

struct TLonStr	{
	uchar *sb;
	int len,off;
	char up;
}lst[MAXST+1],
*milen,*mac,*lc;

#define p2b(pt) (*(ushort *)pt)

void encode()	{
	baza=-1; //offset count
	putch('D'); // Radix sort
	memset(r, 0, sizeof(trax2));
	sfin = bin+n; //reverse order
	for(i=0; i < n>>1; i++)	{
		register uchar back;
		back = bin[n-1-i];
		bin[n-1-i] = bin[i];
		bin[i] = back;
	}
	for(fly=bin; fly<sfin-1; fly++)
		r[0][p2b(fly)]++;
	r[0][i=0x10000] = w = n;
	for(ch=255; ch>=0; ch--)	{
		rb[ch] = w; if(ch == *bin) w--;
		for(ch2=255; ch2>=0; ch2--)	{
			r2a[0][--i] = w;
			w -= r[0][i];
			r[0][i] = w;		
		}
	}//shifting...
	for(fly = bin; fly<sfin-1; fly++)	{
		if(fly[1] > fly[0] && fly[0] <= fly[-1])
			p[--r2a[0][p2b(fly)]] = fly+1-bin, fly++;
	}// Direct sort
	memset(lst,0,(MAXST+1)*sizeof(struct TLonStr));
	lst[MAXST].len = afi.bs;
	lst[MAXST].sb = bin-5;
	for(ch=0; ch<0x10000; ch++)	{
		ray(r2a[0][ch], r[0][ch+1], bin-5);
		r2a[0][ch] = r[0][ch];
	}//Left2Right wave
	*sfin=0; putch('\b'); putch('W');
	for(ch=0; ch<256; ch++)	{
		register int pos, lim = r2a[ch][ch];
		for(i=r[ch][0]; i<lim; i++)
			if(*(fly = bin+p[i]+1) >= ch)
				p[r2a[*fly][ch]++] = fly-bin;
		for(lim = r2a[ch][ch]; i<lim; i++)
			if(bin[pos = (p[i]+1)] == ch)
				p[lim++] = pos;
	}//Right2Left wave
	*sfin=0xFF; sym=0;
	p[--rb[*bin]] = 0; putc(*bin, fo);
	for(ch=255; ch>=0; ch--)
		for(i=r[ch+1][0]-1; i>=r[ch][0]; i--)	{
			if((pos = p[i]+1) == n)	{
				baza = i; putc(sym,fo);
				continue;
			}//finish it
			sym = bin[pos]; putc(sym,fo);
			if(sym <= ch) p[--rb[sym]] = pos;
		}
	fwrite(&baza,4,1,fo);
}
#undef p2b

char isab(register uint *a,register uint *b)	{
	uint *b0 = b; int cof; uchar *b1;
	for(i=0; i<LINGO; i++)	{
		if(*a != *b) return *a > *b;
		--a; --b;
	}//change order
	if(a>b)	{ b1 = (uchar *)a;
		cof = b1-(uchar*)b;
	}else	{ b1 = (uchar *)b;
		cof = b1-(uchar*)a;
	}//chose period
	mac = milen = lst+MAXST;
	for(lc=lst; lc<lst+MAXST; lc++)	{
		if(lc->len < milen->len) milen=lc;
		if(lc->off == cof  &&  lc->sb - lc->len < b1)
			if(lc->sb > mac->sb) mac=lc;
	}//continue until border
	for(i=b1-mac->sb; i>0; i-=4)	{
		if(*a != *b) break;
		a--; b--;
	}//replace old bound
	b1 += LINSIZE;
	if(i>0 || mac==lst+MAXST)	{
		uchar rez = *a>*b || (uchar*)a<=bin-4;
		v = (b0-b-1)<<2;
		if(v > milen->len)	{
			milen->sb = b1;
			milen->len = v;
			milen->off = cof;
			milen->up = (rez == a>b);
		}return rez;
	}//update bound
	if(b1 > mac->sb)	{
		mac->len += b1-mac->sb;
		mac->sb = b1;
	}return (mac->up == a>b);
}

#define pkey(ind) (uint *)(u+(ind))
#define key(str) (*pkey(str))

void ray(int A, int B, uchar *u)	{
	register int x,z;
	st: if (B-A < INSERT)	{
		for(x=A+1; x<B; x++)	{
			s = p[x]; z=x;
			while(--z>=A && isab(pkey(p[z]), pkey(s)))
				p[z+1] = p[z];
			p[z+1] = s;
		}
		return;
	}//the stonecode
	v = bin-4-u;
	s = p[(A+B)>>1];
	if(s<=v) s=p[A];
	x=A; z=B; w=key(s);
	for(cur=A; cur<z;)	{
		s=p[cur]; q=key(s);
		if(q < w)	{
			p[cur++] = p[x];
			p[x++] = s;
		}else
		if(q > w || s<=v)	{
			p[cur] = p[--z];
			p[z] = s; 
		}else cur++;
	}//recurse
	ray(A,x,u);
	ray(z,B,u);
	u-=4; A=x; B=z;
	goto st;
}

#undef INSERT
#undef ASIG
#undef MAXST
#undef LINGO
#undef LINSIZE
#undef TERM