COMPILE=gcc -Wall -std=c89

all:	main test alpha

main:
	${COMPILE} -o bin/main src/main.c src/order.c src/coder.c

test:
	${COMPILE} -o bin/test src/test.c

alpha:
	${COMPILE} -o bin/alpha src/alpha.c
