#include <time.h>
#include "archon.h"

static const char IDENT[] =
	"Archon4r0 (C)kvark, 2007";
int memory;
#define VERIFY

void printime(char *spre,clock_t time)	{
	#ifdef VERBOSE
	float ft = (clock()-time)*(1.0f/CLOCKS_PER_SEC);
	printf("%s time: %.3f sec\n",spre,ft);
	#endif	//VERBOSE
}

int main(int argc, char *argv[])	{
	#define CHECK(expr,cod) if(!(expr))	\
		{code=(cod); goto abort;}
	enum ArchonErrors	{
		AE_SUCCESS,AE_FORMAT,AE_MEMORY,
		AE_INT,AE_SOURCE,AE_DEST,AE_NULL
	};
	int code = 0; FILE *ff=NULL;
	#ifdef VERBOSE
	clock_t t0 = clock();
	static char *errors[AE_NULL] =	{ NULL,
		"Usage: archon4r0 [e|d] <source> <dest>",
		"Memory allocation error!",
		"Internal program error - sorry!",
		"Source has no read permission!",
		"Dest has no write permission!"
	};
	#endif	//VERBOSE
	setbuf(stdout,NULL);
	printf("%s\n",IDENT);
	CHECK(argc>=4,AE_FORMAT);
	
	ff = fopen(argv[2],"rb");
	CHECK(ff,AE_SOURCE);
	fseek(ff,0,SEEK_END);
	CHECK(geninit(ff),AE_MEMORY);
	fclose(ff);
	ff = fopen(argv[argc-1],"wb");
	CHECK(ff,AE_DEST);

	if(argv[1][0] == 'e')	{
		clock_t t1;
		CHECK(gencode(),AE_MEMORY);
		printf("memory: %d meg\n",memory>>20);
		printf("Processing...\t");
		
		//Suffix Array Construction
		t1 = clock();
		CHECK(compute()>=0,AE_INT);
		printime("SAC",t1);
		
		#ifdef VERIFY
		printf("Verifying...\t");
		t1 = verify();
		printf("\nresult: %s\n",t1?"bad":"good");
		#endif	//VERIFY
		
		CHECK(encode(ff)>=0,AE_INT);
		genprint();
	}else if(argv[1][0] == 'd')	{
		CHECK(decode(ff)>=0,AE_INT);
	}
	abort:
	if(ff) fclose(ff);
	genexit();
	#ifdef VERBOSE
	if(!code) printime("Total",t0);
	else printf("%s\n",errors[code]);
	#endif	//VERBOSE
	return code;
}
