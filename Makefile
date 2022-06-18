CC=gcc
CFLAGS=-g -ggdb -O2 -Wall

CXX=g++
CXXFLAGS=-g -ggdb -O2 -Wall -std=c++11 -I"$(shell spack location -i dyninst)/include" -I"$(shell spack location -i tbb)/include" -I"$(shell spack location -i boost)/include"

LD=ld
LDFLAGS=-L"$(shell spack location -i dyninst)/lib" -I"$(shell spack location -i tbb)/lib" -I"$(shell spack location -i boost)/lib"  -ldyninstAPI -linstructionAPI -lparseAPI

RM=rm
RMFLAGS=-rf

OBJS=dumpcfgs.o
OUTPUT=dumpcfgs

$(OUTPUT):	$(OBJS)
		$(CXX) -o $@ $(OBJS) $(LDFLAGS)

dumpcfgs.o:

clean:
	$(RM) $(RMFLAGS) $(OBJS) $(OUTPUT)

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
