#include <memory.h>
//special case for BSD
#include <malloc.h>

#define NDEBUG
#define DEAD	10

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

int ankinit(int);
void ankexit();
void ankprint(void(*)(char*,int,int,int,int));

int compare(int,int,int*);
void ray(int*,int,int);
int sufcheck(int*,int,char);
void getbounds(int,int**,int**);

// caution: REVER uses 'x' & 'z' values
#define REVER(px,pz,tm)	{ x=px,z=pz;		\
	while(x+1 < z) tm=*x,*x++=*--z,*z=tm;	\
}
#define GETMEM(num,type)			\
	(memory += (num)*sizeof(type),		\
	(type*)malloc((num)*sizeof(type)))
#define FREE(ptr) if(ptr) free(ptr)
