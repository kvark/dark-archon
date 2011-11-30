/*	main.c
 *	Archon entry point.
*	project Archon (C) Dzmitry Malyshau, 2011
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "bwt.h"
#include "order.h"


//	Global options	//

struct Options	{
	const char *nameIn,*nameOut;
	void (*fOrder)(int);
	unsigned radPow;
};

struct FunParam	{
	const char *name;
	void (*fun)(int);
}static const 	pFun[] =	{
	{"none",	order_none	},
	{"freq",	order_freq	},
	{"greedy",	order_greedy	},
	{"matrix",	order_matrix	},
	{"topo",	order_topo	},
	{"bubble",	order_bubble	},
	{NULL,		NULL			}
};

struct Options read_command_line(const int ac, const char *av[])	{
	struct Options o = { NULL, NULL, order_none, 20 };
	int i,j;
	for(i=1; i!=ac; ++i)	{
		const char *par = av[i];
		if( par[0]=='-' )	{
			assert( !par[2] );
			switch(par[1])	{
			case 'o':
				par = av[++i];
				assert(i!=ac);
				for(j=0; pFun[j].name && strcmp(par,pFun[j].name); ++j);
				o.fOrder = pFun[j].fun;
				break;
			default:
				printf("Unknow parameter: %s\n",par);
			}
		}else
		if(!o.nameIn)	{
			o.nameIn = par;
		}else if(!o.nameOut)	{
			o.nameOut = par;
		}else	{
			printf("Excess argument: %s\n",par);
		}
	}
	return o;
}


//	Main entry point	//

int main(const int argc, const char *argv[])	{
	time_t t0 = 0; FILE *fx = NULL;
	int i,N; struct Options opt;
	
	printf("Var BWT: Radix+BeSe\n");
	printf("archon <f_in> <f_out> [-o alphabet_order]\n");
	opt = read_command_line(argc,argv);
	if( !opt.nameIn || !opt.nameOut )	{
		printf("Error: IO undefined\n");
		return -1;
	}
	if( !opt.fOrder )	{
		printf("Error: unknown order function\nTry one of these: ");
		for(i=0; pFun[i].name; ++i)
			printf("%s%s", i?", ":"", pFun[i].name);
		printf("\n");
		return -2;
	}

	// read input & allocate memory
	fx = fopen( opt.nameIn, "rb" );
	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	bwt_init( N, opt.radPow );
	fseek(fx,0,SEEK_SET);
	bwt_read(fx);
	fclose(fx); fx = NULL;
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10, bwt_memory()>>10 );

	t0 = clock();
	bwt_transform();

	printf("SufSort completed: %.3f sec\n",
		(clock()-t0)*1.f / CLOCKS_PER_SEC );
	
	fx = fopen( opt.nameOut, "wb" );
	bwt_write(fx);
	fclose(fx); fx = NULL;

	bwt_exit();
	printf("Done\n");
	return 0;
}
