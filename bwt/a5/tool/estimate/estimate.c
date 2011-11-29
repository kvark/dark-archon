#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#define ORDER	4

int main()	{
	const int n = 1<<20;
	typedef unsigned char byte;
	byte last;
	int fc[1<<ORDER] = {0};
	int i,mask;
	
	srand(0);
	last = 0xFF; mask=0;
	for(i=0; i!=ORDER+n; ++i)	{
		byte cur = rand();
		int bit = (cur>last ? 1 : 0);
		if(cur==last && last<0x80) bit=1;
		last = cur;
		mask = (mask + mask + bit) & ((1<<ORDER)-1);
		assert(mask < (1<<ORDER));
		if(i>=ORDER) fc[mask]++;
	}
	
	printf("Count:\n");
	for(i=0; !(i>>ORDER); ++i)	{
		int j;
		for(j=0; j!=ORDER; ++j)	{
			int code = (i>>(ORDER-1-j))&1;
			putchar(code+'0');
		}
		printf(":\t%d%c\n", fc[i]*100/n, '%');
	}
	
	return 0;
}
