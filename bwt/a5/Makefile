COMPILE=gcc -Wall -std=c99

all:	main estimate route

main:
	${COMPILE} -o bin/main src/archon.c src/direct.c

estimate:
	${COMPILE} -o bin/estimate tool/estimate/estimate.c

route:
	${COMPILE} -o bin/route tool/route/route.c 
