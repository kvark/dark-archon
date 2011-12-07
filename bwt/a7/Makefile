CXX		=g++
CXXFLAGS	=-Wall -m32
LDLIBS		=-lstdc++
FILES		=src/main.cpp src/archon.cpp

all:	release

release:
	${CXX} -o bin/main ${FILES} -O2 -fomit-frame-pointer -DNDEBUG

debug:
	${CXX} -o bin/main_d ${FILES} -g3

profile:
	${CXX} -o bin/main_p ${FILES} -O2 -g -DNDEBUG
