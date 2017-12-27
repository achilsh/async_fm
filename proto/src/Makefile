CC = gcc
CXX = g++
CXXFLAG = -Wall -Wno-format -g -O0 -fPIC 

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif
LIB3RD_PATH = ../../l3oss
STARSHIP_PATH = ../../Starship
PROTO_PATH = .


VPATH = $(PROTO_PATH)
        
SUB_DIRS := $(VPATH)
#SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(STARSHIP_PATH)/include \
       -I $(LIB3RD_PATH)/include 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LIB3RD_PATH)/lib -lprotobuf \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lc -ldl -lrt

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

SO_PATH = . 
PB_OBJS = $(patsubst %.pb.cc,%.pb.o,$(CC_SRCS))

TARGET = libim_proto.so

all: $(TARGET)


$(TARGET): $(OBJS) 
	$(CXX) -fPIE -rdynamic -shared -g -o $@ $^ $(LDFLAGS)
	cp $(TARGET) ../../loss/lib

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS) 
	rm -f $(TARGET)
        
        
