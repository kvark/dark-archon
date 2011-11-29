COMPILE=gcc -Wall -std=c99

all:	main reodrer compress

main:
	${COMPILE} -o bin/main src/main.c src/order.c src/coder.c src/bwt.c src/huff.c

reodrer:
	${COMPILE} -o bin/test tool/reorder/reorder.c

compress:
	${COMPILE} -o bin/alpha tool/compress/compress.c
