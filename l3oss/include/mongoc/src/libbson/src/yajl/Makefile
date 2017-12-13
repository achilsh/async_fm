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

VPATH = . 

SUB_DIRS := $(VPATH)
#SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)

LOSS_PATH = .

INC := $(INC) \
       -I $(LOSS_PATH) \
       -I $(LOSS_PATH)/.. \
       -I $(LOSS_PATH)/../bson        


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ -DBSON_COMPILATION \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lc -ldl -lrt

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

#SO_DEP_PATH = .
#SO_DEP_CPP_SRCS = $(foreach dir, $(SO_DEP_PATH), $(wildcard $(dir)/*.cpp))
#OBJS = $(patsubst %.cpp,%.o,$(SO_DEP_CPP_SRCS))

TARGET = libyajl.so

all: $(TARGET)
libyajl.so: $(OBJS)
	$(CXX) -fPIE -rdynamic -shared -g -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS) 
	rm -f $(TARGET)
        
        
