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

CREAT_THRIFT_BIN=../l3oss/bin/thrift

LIB3RD_PATH = ../l3oss
STARSHIP_PATH = ../Starship
PROTO_PATH = ./gen-cpp


VPATH = $(PROTO_PATH)
        
SUB_DIRS := $(VPATH)
#SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(STARSHIP_PATH)/include \
       -I $(LIB3RD_PATH)/include/ \
       -I $(PROTO_PATH)

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LIB3RD_PATH)/lib -lthrift -ltcmalloc_minimal  \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lc -ldl -lrt

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

SO_PATH = . 
PB_OBJS = $(patsubst %.pb.cc,%.pb.o,$(CC_SRCS))

all: $(TARGET)

TARGET = cpp

cpp: src/*.thrift
	for i in $^; do $(CREAT_THRIFT_BIN) -nowarn -o ./ -gen cpp $$i;  done 
	find gen-cpp -name '*.skeleton.cpp' -exec rm {} \;
	find gen-cpp -name '*_constants*' -exec rm {} \;  
	sed -i 's/int32_t cseqid = 0;/int32_t cseqid = rand();/g' `grep 'int32_t cseqid = 0;' -rl ./gen-cpp`  

clean: 
	rm -rf gen-cpp
