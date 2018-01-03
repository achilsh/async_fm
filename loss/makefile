CC = gcc
CXX = g++
CFLAG = -Wall -Wno-format -g -O2 -fPIC 
CXXFLAG = -Wall -Wno-format -g -O2 -fPIC -std=c++11 -DREDIS_DEBUG

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

LIB3RD_PATH = ../l3oss
LOSS_PATH = .


VPATH = $(LOSS_PATH)/src
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)

INC := $(INC) \
       -I /usr/include \
       -I $(LIB3RD_PATH)/include \
       -I $(LIB3RD_PATH)/include/libevent \
       -I $(LIB3RD_PATH)/include/mysql \
       -I $(LIB3RD_PATH)/include/boost  \
       -I $(LOSS_PATH)/src/LibComm/include \
       -I $(LOSS_PATH)/src/MySqlDb/include \
       -I $(LOSS_PATH)/src/RedisClient/include \
       -I $(LOSS_PATH)/src/CnfComm/include \
       -I $(LOSS_PATH)/src/Shm/include \
       -I $(LOSS_PATH)/src  

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LIB3RD_PATH)/lib  -Wl,-Bstatic -lprotobuf -Wl,-Bdynamic \
           -lqlibc \
           -llog4cplus \
           -levent \
           -lev \
           -ljsoncpp \
           -levent \
           -lhiredis \
           -ltcmalloc_minimal \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -ldl -lrt -liconv

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
S_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.s))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.s,%.o,$(S_SRCS)) 

all:  LIBRARYS Ts
TESTSPATH = $(shell find $(LOSS_PATH) -name 'test*' -type d)
Ts:
	@for dir in $(TESTSPATH); do echo "making in $$dir"; ( cd $$dir && make -f makefile -e CC='$(CXX)' CFLAG='$(CFLAG)'); done  

LIBRARYS: $(OBJS)
	ar -crv libloss.a $(OBJS)
	ranlib libloss.a
	$(CXX) -shared -Wl,-soname,libloss.so -o libloss.so $(OBJS) -lc -lrt -ldl $(LDFLAGS)
	mv libloss.a $(LOSS_PATH)/lib/
	mv libloss.so $(LOSS_PATH)/lib/
	cp $(LOSS_PATH)/lib/* $(LIB3RD_PATH)/lib/



%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
%.o:%.c
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
#%.o:%.c
	#$(CC) $(INC) $(CFLAG) -c -o $@ $< 
#%.o:%.s
	#$(CC) $(INC) $(CFLAG) -c -o $@ $< 
clean:
	rm -f $(OBJS)
	rm -f $(LIBRARYS)
	rm -f $(LOSS_PATH)/lib/*
	@for dir in $(TESTSPATH); do echo "making in $$dir"; ( cd $$dir && make clean -f makefile -e CC='$(CXX)' CFLAG='$(CFLAG)'); done 
  
