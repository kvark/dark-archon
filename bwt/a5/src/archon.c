#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>

#include "archon.h"

enum { DM_NONE, DM_TIME, DM_CHECK };
static const int kOverrun = 10;
static const int kDebug = DM_CHECK;
static const float kTime = 1.f/CLOCKS_PER_SEC;

#define IMT_ORDER	(4)
#define IMT_GROUPS	(1<<IMT_ORDER)
#define IMT_HIGHEST	(IMT_GROUPS>>1)
#define RADIX_BUF	(3)

void sort_lucky(suffix*, suffix*, const byte*);
void sort_stable_radix(suffix *const, const int, suffix *const, const int, const byte *const);

static void fill_groups(const byte *const bin, suffix *const p, const int n, int *const g)	{
	int i, mask=0, submask=0, was = 0;
	int off[4];
	off[0] = off[1] = 0;
	off[2] = off[3] = n;
	memset(g, 0, IMT_GROUPS*sizeof(int));
	for(i=0; i!=n; ++i)	{
		//update mask
		const int prev = (was<<1) + (mask>>(IMT_ORDER-1)),
			cur = (was = (int)bin[i]) << 1;
		int val = i+1;	//points _after_ string
		mask = submask + (cur < prev ? IMT_HIGHEST : 0);
		submask = mask>>1;
		//update groups
		switch(submask)	{
			case 0x0:	val = p[off[0]++];
			case 0x1:	p[off[1]++] = val;
				break;
			case 0x7:	val = p[--off[3]];
			case 0x6:	p[--off[2]] = val;
				break;
		}
		g[mask]++;
	}
	g[mask = IMT_GROUPS] = i = n;
	while(--mask>=0) g[mask] = (i -= g[mask]);
	assert(g[0x2] == off[0] && g[0x4] == off[1]);
	assert(g[0xC] == off[2] && g[0xE] == off[3]);
}

static void order_unlucky(const byte *const bin, suffix *const p, int *const g)	{
	const suffix n = g[IMT_GROUPS];
	int lim,i,j;
	
	// 000$
	for(i=0; i != n && bin[i-1]<=bin[i]; ++i)
		p[i] = i+1;
	lim = i;
	// 001 -> 00..01 -> 00
	for(i=g[0x4],j=g[0x2]; i!=j;)	{
		suffix s;
		while((s=p[--i])!=n && bin[s-1] <= bin[s])
			p[--j] = s+1;
	}
	assert(i == lim);
	// 110 -> 11..10 -> 11
	for(i=g[0xC],j=g[0xE]; i!=j;)	{
		suffix s;
		while((s=p[i++])!=n && bin[s-1] >= bin[s])
			p[j++] = s+1;
	}
	assert(i == n);
	// 11 -> 011
	for(i=n,j=g[0x8]; i!=g[0xC];)	{
		const suffix s = p[--i];
		if(s!=n && bin[s-1]<bin[s])
			p[--j] = s+1;
	}
	assert(j == g[0x6]);
	// 011 -> 0101..011
	for(i=g[0x8]; i!=j;)	{
		suffix s;
		while((s=p[--i])+1<n && bin[s-1]>bin[s] && bin[s]<bin[s+1])
			p[--j] = s+2;
	}
	lim = j;
	// 00 -> 0101..0100
	for(i=0,j=g[0x4]; i!=j;)	{
		suffix s;
		while((s=p[i++])+1<n && bin[s-1]>bin[s] && bin[s]<bin[s+1])
			p[j++] = s+2;
	}
	assert(i == lim);
	// 0 -> 10
	for(i=0,j=g[0x8]; i!=g[0x8];)	{
		const suffix s = p[i++];
		if(s!=n && bin[s-1]>bin[s])
			p[j++] = s+1;
	}
	assert(j == g[0xC]);
}

static int check_array(const suffix *const p, const int n, const byte *const bin)	{
	static int R[0x100]; int i;
	for(i=n; i--;) R[bin[p[i]-1]] = i;
	R[bin[0]]++;
	for(i=0; i!=n; ++i)	{
		const suffix s = p[i];
		if(s!=n && p[R[bin[s]]++] != s+1) break;
	}
	return i;
}

static void output_array(const suffix *const p, const int n, const byte *const bin,  FILE *const ff)	{
	int i,base=-1;
	/*for(i=0; i!=n; ++i)
		putc(bin[p[i]],ff);
	*/
	for(i=0; p[i]!=n; ++i) putc(bin[p[i]],ff);
	base = i; putc(bin[0],ff);
	while(++i != n) putc(bin[p[i]],ff);
	fwrite(&base, sizeof(int), 1, ff);
}

int main(int argc, char *argv[])	{
	FILE *ff = NULL;
	byte *bin = NULL;
	//suffix *p = NULL, *q = NULL;
	suffix p[10], *q = NULL;
	int n = 0, m, sorted, g[IMT_GROUPS+1];
	clock_t t_all,t_cur,t_direct;
	if(argc != 3) return -1;
	ff = fopen(argv[1],"rb");
	if(!ff) return -2;
	fseek(ff,0,SEEK_END);
	n = ftell(ff);
	m = (n>>RADIX_BUF)+1;
	fseek(ff,0,SEEK_SET);
	
	bin = (byte*)malloc(n + kOverrun);
	//p = (suffix*)malloc((n+1)*sizeof(suffix));
	q = (suffix*)malloc(m*sizeof(suffix));
	if(!bin || !p || !q) return -3;
	memset(bin, 0x00, kOverrun);
	bin += kOverrun;
	fread(bin,1,n,ff);
	fclose(ff); ff = NULL;
	printf("Archon5\n");

	//stage-1: group filling
	t_all = t_cur = clock();
	fill_groups(bin,p,n,g);
	if(kDebug >= DM_CHECK)	{
		printf("Debug mode\n");
		memset(p,			-1,	(g[0x2]-0)*sizeof(suffix));
		memset(p+g[0x4],	-1,	(g[0xC]-g[0x4])*sizeof(suffix));
		memset(p+g[0xE],	-1,	(n-g[0xE])*sizeof(suffix));
	}
	if(kDebug >= DM_TIME)	{
		t_cur = clock()-t_cur;
		printf("Stage 1: %.3f\n", kTime*t_cur);
	}

	//stage-2: direct sorting of lucky groups
	sorted = g[0x4]-g[0x2] + g[0xE]-g[0xC];
	if(kDebug)	{
		printf("Sorting: %d%c\n", sorted/(1 + n/100), '%');
	}
	t_cur = clock();
	sort_lucky(p+g[0x2], p+g[0x4], bin);
	sort_lucky(p+g[0xC], p+g[0xE], bin);
	t_direct = t_cur = clock() - t_cur;
	printf("Stage 2: %.3f\n", kTime*t_cur);
	
	//stage-3: getting order of unlucky ones
	t_cur = clock();
	order_unlucky(bin,p,g);
	t_cur = clock() - t_cur;
	printf("Stage 3: %.3f\n", kTime*t_cur);
	
	//stage-4: radix post-sorting
	t_cur = clock();
	sort_stable_radix(p,n, q,m, bin-sizeof(dword));
	sort_stable_radix(p,n, q,m, bin-sizeof(word));
	t_cur = clock() - t_cur;
	printf("Stage 4: %.3f\n", kTime*t_cur);

	t_all = clock() - t_all;
	printf("Total time: %.3f\n", t_all*kTime);
	printf("Linear coef: %.3f\n", (t_all-t_direct)*(1000.f/n));

	if(kDebug)	{
		const int id = check_array(p,n,bin);
		printf("Suffix check: %d\n", n-id);
	}

	ff = fopen(argv[2],"wb");
	output_array(p,n,bin,ff);
	fclose(ff);

	free(bin-kOverrun);
	free(p); free(q);
	return 0;
}
