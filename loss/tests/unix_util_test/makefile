CC = gcc
CXX = g++
CFLAG = -Wall -Wno-format -g -O2 -fPIC
CXXFLAG = -Wall -Wno-format -g -O0 -fPIC -std=c++11

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

LIB3RD_PATH = ../../../l3oss
LOSS_PATH = ../../../loss


VPATH = . 
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)

INC := $(INC) \
       -I /usr/include \
       -I $(LIB3RD_PATH)/include \
       -I $(LIB3RD_PATH)/include/libevent \
       -I $(LIB3RD_PATH)/include/mysql \
       -I $(LIB3RD_PATH)/include/jemalloc \
       -I $(LOSS_PATH)/src/unix_util \
       -I $(LOSS_PATH)/src/util \
       -I $(LOSS_PATH)/src  

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LOSS_PATH)/lib -lloss \
           -L$(LIB3RD_PATH)/lib \
           -llog4cplus \
           -ljwsmtp \
           -levent \
           -lev \
           -ljemalloc \
           -ljsoncpp \
           -lhiredis \
           -lmysqlclient\
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -ldl -lrt -liconv

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
S_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.s))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.s,%.o,$(S_SRCS)) 

TARGET= UnixUtilTest
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAG) -o $@ $^ $(LDFLAGS)
%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
