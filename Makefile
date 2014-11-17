
DEBUG ?= -g
OPTIMIZATION ?= -O2
FLAG := $(DEBUG) $(OPTIMIZATION) -Wall -Wno-deprecated
MACRO := 

INC :=
LIB :=

LDFLAG := 
COMM_LIB :=

SRCS := ConHashRing.cpp TestConHash.cpp
OBJS := $(SRCS:.cpp=.o)

.SUFFIXES: .o .cpp
.cpp.o:
	g++ $(FLAG) -c -o $@ $< $(MACRO) $(INC)

TARGET = TestConHash

all : $(TARGET)

$(TARGET) : $(OBJS)
	g++ $(FLAG) -o $@ $^ $(LDFLAG) $(LIB) $(COMM_LIB)

.PHONY : clean
clean :
	-rm -f *.o $(TARGET)