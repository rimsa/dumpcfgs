CC=gcc
CFLAGS=-g -ggdb -O2 -Wall

CXX=g++
CXXFLAGS=-g -ggdb -O2 -Wall -std=c++11 -I"${CPATH}"

LD=ld
LDFLAGS=-L"${LIBRARY_PATH}" -ldyninstAPI -linstructionAPI -lparseAPI

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
