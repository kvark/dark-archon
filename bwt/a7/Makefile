CXX		=g++
CXXFLAGS	=-Wall -m32
LDLIBS		=-lstdc++
FILES		=src/main.cpp src/archon.cpp

all:	main main_d

main:
	${CXX} -o bin/main ${FILES} -O2 -fomit-frame-pointer -DNDEBUG

main_d:
	${CXX} -o bin/main_d ${FILES} -g3
