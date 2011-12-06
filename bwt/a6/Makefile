COMPILE=gcc -Wall -std=c99 -g3 -m32

all:	main reodrer compress

main:
	${COMPILE} -o bin/main src/main.c src/order.c src/coder.c src/bwt.c src/huff.c

reodrer:
	${COMPILE} -Isrc -o bin/reorder tool/reorder/reorder.c

compress:
	${COMPILE} -Isrc -o bin/compress tool/compress/compress.c
