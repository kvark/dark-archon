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
	// variables
	const char *name_in, *name_out;
	enum KeyConfig key_conf;
	unsigned rad_pow;
	// functors
	void (*f_order)(int);
	int	(*f_read)(FILE *const);
	void (*f_transform)();
	void (*f_write)(FILE *const);
};

struct OrderFunParam	{
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

struct KeyFunParam	{
	const char *name;
	enum KeyConfig config;
}static const pKey[] =	{
	{"byte",	KEY_BYTE},
	{"fix",		KEY_FIXED},
	{"var",		KEY_VARIABLE},
	{NULL,		KEY_NONE},
};


struct Options read_command_line(const int ac, const char *av[])	{
	struct Options o = { NULL, NULL, KEY_BYTE, 16, order_none, bwt_read, bwt_transform, bwt_write };
	int i,j;
	for(i=1; i!=ac; ++i)	{
		const char *par = av[i];
		if( par[0]=='-' )	{
			assert( !par[2] );
			switch(par[1])	{
			case 'o':
				par = av[++i]; assert(i!=ac);
				for(j=0; pFun[j].name && strcmp(par,pFun[j].name); ++j);
				o.f_order = pFun[j].fun;
				break;
			case 'c':
				par = av[++i]; assert(i!=ac);
				for(j=0; pKey[j].name && strcmp(par,pKey[j].name); ++j);
				o.key_conf = pKey[j].config;
				break;
			case 'r':
				par = av[++i]; assert(i!=ac);
				sscanf( par, "%d", &o.rad_pow );
				break;
			case 'u':
				o.rad_pow = 0;
				o.key_conf = KEY_UNPACK;
				o.f_read = unbwt_read;
				o.f_transform = unbwt_transform;
				o.f_write = unbwt_write;
				break;
			default:
				printf("Unknow parameter: %s\n",par);
			}
		}else
		if(!o.name_in)	{
			o.name_in = par;
		}else if(!o.name_out)	{
			o.name_out = par;
		}else	{
			printf("Excess argument: %s\n",par);
		}
	}
	return o;
}


//	Main entry point	//

static void get_time(time_t *const t)	{
	*t = clock();
	//time(t);
}

static double get_diff(time_t t1, time_t t2)	{
	//return difftime(t1,t2);
	return (t1-t2)*1.f / CLOCKS_PER_SEC;
}


int main(const int argc, const char *argv[])	{
	time_t t0=0,t1=0,t2=0; double elapsed = 0.f;
	FILE *fx = NULL; int i,N;
	struct Options opt;
	
	printf("Archon6 var-len suffix sorter\n"
		"format: archon <fIn> <fOut> [-u | <options>]\n"
		"where options are:\n"
		"\t-r <iRadixPower>\n"
		"\t-o <sAlphabetOrder>\n"
		"\t-c <sCompressionType>\n"
		);

	opt = read_command_line(argc,argv);
	if( !opt.name_in || !opt.name_out )	{
		printf("Error: IO undefined\n");
		return -1;
	}
	if( !opt.f_order )	{
		printf("Error: unknown order function\nTry one of these: ");
		for(i=0; pFun[i].name; ++i)
			printf("%s%s", i?", ":"", pFun[i].name);
		printf("\n");
		return -2;
	}
	if( opt.key_conf == KEY_NONE )	{
		printf("Error: unknown key configuration\nTry one of these: ");
		for(i=0; pKey[i].name; ++i)
			printf("%s%s", i?", ":"", pKey[i].name);
		printf("\n");
		return -2;
	}

	// read input & allocate memory
	fx = fopen( opt.name_in, "rb" );
	if(!fx)	{
		printf("Error: unable to open input file!\n");
		return -3;
	}
	fseek(fx,0,SEEK_END);
	N = ftell(fx);
	bwt_init( N, opt.rad_pow, opt.key_conf );
	fseek(fx,0,SEEK_SET);
	get_time(&t0);
	opt.f_read(fx);
	fclose(fx); fx = NULL;
	printf("Loaded: %d kb; Allocated %d kb\n", N>>10, bwt_memory()>>10 );

	get_time(&t1);
	opt.f_transform();
	get_time(&t2);	elapsed = get_diff(t2,t1);
	printf("Transformation completed: %.3f sec\n", elapsed);
	
	fx = fopen( opt.name_out, "wb" );
	if(!fx)	{
		printf("Error: unable to open output file!\n");
		return -3;
	}
	opt.f_write(fx);
	fclose(fx); fx = NULL;

	get_time(&t2);	elapsed = get_diff(t2,t0);
	printf("Transform+IO time: %.3f sec\n", elapsed);

	bwt_exit();
	printf("Done\n");
	return 0;
}
