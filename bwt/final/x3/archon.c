/*
*	Archon X3	(C) kvark, 2006
*		Archon project
*	Burrows-Wheeler Transformation algoritm
*
*		Anchors tech + Deep tech
*		Direct sorting (a<b>=c)
*		Units: dword, no bswap
*/

//#define SUFCHECK
#include "total.h"
#include <stdlib.h>
#include <time.h>
#include <limits.h>

const ushort ASIG = 'RA';
#define TERM	(320)
typedef int trax2[0x10001];

int BS,n,*p;
int baza,ch;
uchar *s_bin, *bot, sym;
uchar *sfin, *bin;
int rb[0x100];
trax2 r,r2a;

clock_t t0;
FILE *fi,*fo;
int fsize,memory;
int mode_enc;

void encode();
void decode();

//DirectSort Help routines
uchar *substring(int id)	{ return bin+id-1; }
void getlimits(int id, int **lo, int **hi)	{
	int pre = p2b(substring(id));
	*hi = p+r2a[pre]-1; *lo = p+r[pre];
}
int subequ(int *p0, int* p1, int dis)	{
	return bin[*p0+dis] == bin[*p1+dis];
}
int poksize(int pok)	{ return r[pok+1]-r[pok]; }
int *findsuf(int pok, int suf)	{
	int *rez = p+r[pok];
	while(*rez != suf) rez++;
	return rez;
}
ushort getcurpok()	{ return ch; }
int getdeep(uchar *bof)	{ return bin-bof; }
//End of routines

#ifdef SUFCHECK
int sufcheck(int A[], int B[])	{ int *x;
	for(x=A; x<B-1; x++)	{
		uchar *a = bin-3+x[0];
		uchar *b = bin-3+x[1];
		while(p4b(a)==p4b(b)) a-=4,b-=4;
		if(p4b(a) > p4b(b)) return x-A;
	}return -1;
}
#endif

void startall()	{
	memory = sizeof(r) + sizeof(rb) + sizeof(r2a);
	p = malloc(fsize*sizeof(int));
	s_bin = malloc(fsize+TERM+4);
	memset(s_bin,0,TERM); bin = s_bin+TERM;
	memory += fsize*(sizeof(int)+1)+TERM;
	if(!mode_enc)	{
		bot = malloc(fsize); memory += fsize;
	}
}
void endall(int code)	{
	if(code>0) printf("\nAn error %d accured!", code);
	if(s_bin) free(s_bin);
	if(bot) free(bot);
	if(p) free(p);
	if(fi) fclose(fi);
	if(fo) fclose(fo);
	if(code)	{
		printf("\nCall format:");
		printf("\narchonx3 {e|d} [-bSize] <source> <dest>");
		printf("\nDefault block: -b4m or -b4096k");
		getchar(); exit(code);
	}
}

int main(int argc,char *argv[])	{
	int t0 = clock();
	setbuf(stdout,NULL);
	printf("\nArchon X3\t(C)kvark, 2006");
	if(argc < 4) endall(-1);
	else argv++; // modes
	if(argv[0][0] == 'e') mode_enc = 1;
	else if(argv[0][0] == 'd') mode_enc = 0;
	else endall(-1);
	argv++; fsize = 1<<22; // block 4m
	if(argv[0][0] == '-')	{
		char *sp = argv[0]+2;
		if(sp[-1] != 'b') return -1;
		for(fsize = 0; sp[0]>='0' && sp[0]<='9'; sp++)
			fsize = fsize*10 + sp[0]-'0';
		if(sp[0] == 'm') fsize <<= 20;
		else if(sp[0] == 'k') fsize <<= 10;
		if(fsize < 8 || fsize > 1<<28) fsize = 1<<22;
		argv++;
	}
	fi = fopen(argv[0],"rb");
	fo = fopen(argv[1],"wb");
	if(!fi || !fo)	{
		if(!fi) printf("\nCan't open input file");
		if(!fo) printf("\nCan't create out file");
		endall(-2);
	}//offs
	printf("\n<%s> progress: ",argv[0]);
	if(mode_enc)	{
		fwrite(&ASIG,2,1,fo);
		fwrite(&fsize,4,1,fo);
		startall(); do	{
			n = fread(bin,1,fsize,fi);
			encode(); // HERE !!
		}while(n==fsize);
		#if VERBLEV >= 2
		printf("\nRoutine calls:");
		printf("\n\tDigger(%d), NumSt(%d)",call_digger,numst);
		printf("\n\tIndiana(%d+%d+%d)",call_fw_anch,call_bk_anch,call_fw_buck);
		printf("\n\tDeepRay(%d), SmartIns(%d)",call_deep_ray,call_smart_ins);
		printf("\n\tSplit(%d), Pseudo(%d)",call_split,call_pseudo);
		#endif
	}else	{ ushort sig;
		fread(&sig,2,1,fi);
		if(sig != ASIG) endall(-3);
		fread(&fsize,4,1,fi);
		startall(); do	{
			n = fread(bin,1,fsize+4,fi)-4;
			memcpy(&baza,bin+n,4);
			decode();
			fwrite(bot,1,n,fo);
		}while(n == fsize);
	}//exit
	endall(0);
	t0 = 1000*(clock() - t0)/CLOCKS_PER_SEC;
	#if VERBLEV >= 1
	printf("\nBlock: %luk, Used memory: %lu kb",fsize>>10,memory>>10);
	printf("\nExecution time: %lums",t0);
	#endif
	return 0;
}

#define nextp(id) p[id] = rb[bin[id]]++;
void decode()	{
	register int i,pos;
	register uchar cl;
	memset(rb, 0, 0x100*sizeof(int));
	for(i=0; i<n; i++) rb[bin[i]]++;
	i=n; cl=0; do	{ cl--;
		i -= rb[cl], rb[cl] = i;
	}while(cl);
	putchar('H');
	bot[0] = cl = bin[baza];
	pos = p[0] = rb[cl]++;
	for(i=0; i<baza; i++)	nextp(i);
	for(i=baza+1; i<n; i++)	nextp(i);
	for(i=1; i<n; i++,pos=p[pos])
		bot[i] = bin[pos];
}
#undef nextp

void encode()	{
	register uchar cl,*fly;
	register int i,pos,lim;
	putchar('D'); baza=-1; // Radix sort
	memset(r, 0, sizeof(trax2));
	sfin = bin+n; //scan
	for(fly=bin; fly<sfin-1; fly++)
		r[p2b(fly)]++;
	r[ch=0x10000] = pos = n;
	i = 256; do	{ i--;
		cl=0; do	{
			pos -= r[--ch];
			r2a[ch] = r[ch] = pos;
		}while(--cl);
		rb[i] = pos;
		if((uchar)i == *bin)	{
			p[--pos] = 0; r[ch]--;
		}//for start
	}while(i);
	sfin[0] = 0xFF; fly=bin; //border
	do if(BADSUF(fly))
		p[r2a[p2b(fly)]++] = fly+1-bin, fly++;
	while(++fly<sfin);
	// Direct sort
	StartHelp(n);
	for(ch=0; ch<0x10000; ch++)	{
		//printf("%4x\b\b\b\b",ch);
		ray(p+r[ch], p+r2a[ch], bin-5);
	}//Right2Left wave
	StopHelp();
	memcpy(r2a,r+1,sizeof(trax2)-sizeof(int));
	putchar('\b'); putchar('W'); *sfin=0xFF;
	cl=0; do	{ cl--;
		lim = r2a[(cl<<8)+cl];
		for(i=r[(uint)(cl+1)<<8]-1; i>=lim; i--)	{
			unsigned char cc = bin[pos = p[i]+1];
			if(cc <= cl) p[--r2a[(cc<<8)+cl]] = pos;
		}
		for(lim = r2a[(cl<<8)+cl]; i>=lim; i--)
			if(bin[pos = p[i]+1] == cl)
				p[--lim] = pos;
	}while(cl);
	*sfin=0x00; //Left2Right wave
	cl=0; i=0; do	{
		ch = r[(uint)(cl+1)<<8]-r[cl<<8];
		assert(i == r[cl<<8]);
		for(; ch--; i++)	{
			if((pos = 1+p[i]) == n)	{
				baza=i; putc(*bin,fo);
				continue;
			}//finish
			sym = bin[pos]; putc(sym,fo);
			if(sym >= cl) p[rb[sym]++] = pos;
		}
	}while(++cl);
	#ifdef SUFCHECK
	printf("\nChecking SA...");
	pos = sufcheck(p,p+n);
	if(pos < 0) printf("OK");
	else printf("err in p[%d]",pos);
	#endif
	fwrite(&baza,4,1,fo);
}
#undef TERM