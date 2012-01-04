CXX		=g++
CXXFLAGS	=-Wall -m32
LDLIBS		=-lstdc++
FILES		=src/main.cpp src/archon.cpp
RFLAGS		=-O3 -DNDEBUG

release:
	${CXX} -o bin/main ${FILES} ${RFLAGS} -fomit-frame-pointer

debug:
	${CXX} -o bin/main_d ${FILES} -g3

valgrind:
	${CXX} -o bin/main_p ${FILES} ${RFLAGS} -g

gprof:
	${CXX} -o bin/main_g ${FILES} ${RFLAGS} -pg

all:	release debug valgrind gprof
