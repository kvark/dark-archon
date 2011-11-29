/*
*	ArchonX	(C) kvark, December 2004
*		Archon project
*	Burrows-Wheeler Transformation algoritm
*
*		Direct sorting (a>b<=c) ~1/3
*		Units - fast dword
*		Bound - stone wall
*
*/
#define DEBUG	0
#define STATS	0

#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <memory.h>

#define MB		(4<<20)
#define TERM	10
#define LIN		8
#define SYM		(char)(bin[0]&0xFE)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef int trax[257];

int n,p[MB+1];
uchar bin[MB+1+TERM];
uchar *hival;
trax r,ra,rb,rc;

clock_t t0;
FILE *fi,*fo;
int w1,w2;
int ch,sym;
int i,j;

void ray(uint,uint,uchar*);
void parse();

int main(int argc,char *argv[])	{
	int t0=clock();
	if(argc!=3) return(-1);
	fi = fopen(argv[1],"rb");
	fo = fopen(argv[2],"wb");
	if(!fi || !fo) return(1);
	cprintf("\nArchon X1\t(C)kvark, 2004");
	if(STATS) cprintf("\n%8p %8p ",p,bin);
	cprintf("\nProgress: ");
	while(n = fread(bin,1,MB,fi))	{
		fwrite(&n,4,1,fo);
		hival = bin+n+4;
		w1=w2=0; parse();
		//statistics
		if(STATS) cprintf("\nDirectly sorted %f",(float)w1/n);
		if(STATS) cprintf("\nSorted kvarks   %f",(float)w2/w1);
	}
	fclose(fi); fclose(fo);
	t0 = clock() - t0;
	cprintf("\tTime: %lu",t0);
	return(0);
}

void parse()	{ int baza=-1;
	bin[n++]=SYM; putch(' ');
	putch('\b'); putch('R'); // Radix sort
	memset(r,0,sizeof(trax));
	memset(rc,0,sizeof(trax));
	memset(bin+n,0xFF,TERM);
	//memset(p,0,n*4);
	for(i=0; i<n-2; i++)	{ r[bin[i]]++;
		if(bin[i] > bin[i+1])	{
			if(bin[i+1] > bin[i+2])
				rc[bin[i]]++;
			else r[bin[++i]]++;
		}	
	}//offset count
	for(i=0,ch=0; ch<=256; ch++)	{
		i+=r[ch],  r[ch] = i-r[ch];
		ra[ch] = r[ch];
		rc[ch] += r[ch];
		rb[ch] = rc[ch];
	}//shifting...
	for(i=0; i<n; i++)
		if(bin[i] > bin[i+1]  &&  bin[i+1] <= bin[i+2])
			(p[rc[bin[i]]++]=i, i++);
	putch('\b'); putch('D'); // Direct sort
	for(ch=0; ch<256; ch++)	{
		//part 2: (x)>y<=z
		if(STATS) w1 += rc[ch]-rb[ch];
		ray(rb[ch],rc[ch],bin+1);
	}
	putch('\b'); putch('W'); //Final waves
	for(ch=0; ch<256; ch++)	{
		//part 1: (x)>y>z
		j=rc[ch]; i=r[ch];
		for(; i<rc[ch]; i++)	{
			if(!p[i]) continue;
			sym = bin[p[i]-1];
			if(sym > ch)	{
				while(bin[p[rb[sym]]+1] < ch  &&  rb[sym] < rc[sym])
					p[ra[sym]++] = p[rb[sym]++];
				p[ra[sym]++] = p[i]-1;
			}else
			if(sym == ch)
				p[j++] = p[i]-1;
		}
		//part 3: (x)=y>=z
		for(; i<j; i++)
			if(p[i]  &&  bin[p[i]-1] == ch)
				p[j++] = p[i]-1;
		rc[ch] = j;
		ra[ch] = r[ch+1];
	}
	p[--ra[bin[n-1]]] = n-1;
	putc(bin[n-1],fo);
	for(ch=255; ch>=0; ch--)
		for(i=r[ch+1]-1; i>=r[ch]; i--)	{
			if(!p[i])	{
				putc(SYM,fo);
				baza = i; continue;
			}//finish it
			sym = bin[p[i]-1];
			putc(sym,fo);
			if(sym <= ch)
				p[--ra[sym]] = p[i]-1;
		}
	fwrite(&baza,4,1,fo);
}

uint key(int,void *);
#ifdef _MSC_VER
	uint key(int num,void *voff)	{
		uchar *off = (uchar *)voff + num;
		return((off[0]<<24)|(off[1]<<16)|(off[2]<<8)|off[3]);
	}
#else
	#pragma aux key	=	\
		"add ebx,eax"	\
		"mov eax,[ebx]"	\
		"bswap eax"		\
		parm [eax][ebx]	\
		modify [eax];
#endif

uint s,q,w; char r0;
#define near(sa,sb) (hival-((sa)>(sb)?(sa):(sb)))

char isab(uchar *s1,uchar *s2)	{
	r0 = (char)memcmp(s1,s2,near(s1,s2));
	return(r0 ? r0>0 : s1>s2);
}
void ray(uint A,uint B,uchar *u)	{
	int x,z; //bazis
	st: if(STATS) w2 += B-A;
	if (B-A < LIN)	{
		for(w=A+1; w<B; w++)	{
			i=p[w]; q=w;
			while(--q>=A && isab(u+p[q],u+i))
				p[q+1] = p[q];
			p[q+1]=i;
		}
		return;
	}//the stonecode
	if(DEBUG) cprintf("\r%8d%8d",u-bin,ch);
	i=p[(A+B)>>1];
	if(u+i >= bin+n)	{
		p[(A+B)>>1] = p[B-1];
		p[B-1] = i;
		i=p[A]; B--;
	}x=A; z=B;
	w=key(i,u); i=A;
	while(i<z)	{
		s=p[i]; q=key(s,u);
		if(q < w)	{
			p[i]=p[x];
			p[x]=s;
			x++; i++;
		}else
		if(q > w || s+u >= bin+n)	{
			z--; p[i]=p[z];
			p[z]=s;
		}else i++;
	}
	ray(A,x,u);
	ray(z,B,u);
	u+=4; A=x; B=z;
	goto st;
}