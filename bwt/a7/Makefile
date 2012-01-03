CXX		=g++
CXXFLAGS	=-Wall -m32
LDLIBS		=-lstdc++
FILES		=src/main.cpp src/archon.cpp

release:
	${CXX} -o bin/main ${FILES} -O2 -fomit-frame-pointer -DNDEBUG -DNO_SQUEEZE

debug:
	${CXX} -o bin/main_d ${FILES} -g3

valgrind:
	${CXX} -o bin/main_p ${FILES} -O2 -fno-inline -DNDEBUG -g

gprof:
	${CXX} -o bin/main_g ${FILES} -O2 -fno-inline -DNDEBUG -pg

all:	release debug valgrind gprof
