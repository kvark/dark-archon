#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <assert.h>

#define NDEBUG 1
#define VERBLEV	1

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

int InitAll(int,uchar,FILE*,int*);
void Reset();
void EndAll();
void EncodeBlock(uchar *,uchar *,int);
uint DecodeBlock(uchar *,uchar *);

#define MENC	0x01
#define MDEC	0x02

#define p2b(pt) (*(ushort *)(pt))
#define p4b(pt) (*(ulong *)(pt))
#define BADSUF(pid) (pid[0]>pid[1] && pid[0]>=pid[-1])

uchar* substring(int);
void getlimits(int,int**,int**);
int subequ(int*,int*,int);
int poksize(int);
int *findsuf(int,int);
ushort getcurpok();
int getdeep(uchar*);

int StartHelp(int);
void StopHelp();
int isab(uchar*,uchar*);
void ray(int*,int*,uchar*);