COMPILE=gcc -Wall -std=c99 -m32

all:	main tools

main:
	${COMPILE} -o bin/main src/archon.c src/direct.c


tools:	estimate route

estimate:
	${COMPILE} -o bin/estimate tool/estimate/estimate.c

route:
	${COMPILE} -o bin/route tool/route/route.c 
