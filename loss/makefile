CCACHE_B=../l3oss/bin/ccache
CC = $(shell command -v $(CCACHE_B) >/dev/null 2>&1 && echo "$(CCACHE_B) gcc" || echo "gcc")
CXX = $(shell command -v $(CCACHE_B) >/dev/null 2>&1 && echo "$(CCACHE_B) g++" || echo "g++")
CFLAG = -Wall -Wno-format -g -O2 -fPIC 
CXXFLAG = -Wall -Wno-format -g -O0 -fPIC -Wno-deprecated-declarations  -std=c++11 -DREDIS_DEBUG #-DDEBUG_TEST

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
       -I $(LIB3RD_PATH)/include/jwsmtp \
       -I $(LIB3RD_PATH)/include/boost  \
       -I $(LIB3RD_PATH)/include/thrift  \
       -I $(LIB3RD_PATH)/include/thrift/transport  \
       -I $(LOSS_PATH)/src/LibComm/include \
       -I $(LOSS_PATH)/src/MySqlDb/include \
       -I $(LOSS_PATH)/src/RedisClient/include \
       -I $(LOSS_PATH)/src/CnfComm/include \
       -I $(LOSS_PATH)/src/PushEmail/include \
       -I $(LOSS_PATH)/src/Shm/include \
       -I $(LOSS_PATH)/src/thrift_util \
       -I $(LOSS_PATH)/src/LibCoroutine/include \
       -I $(LOSS_PATH)/src/RedisClientPools/include \
       -I $(LOSS_PATH)/src

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LIB3RD_PATH)/lib -lprotobuf \
           -lqlibc \
           -ltcmalloc_minimal \
           -llog4cplus \
           -levent \
           -lev \
           -ljsoncpp \
           -levent \
           -lhiredis \
           -ltcmalloc_minimal \
           -lthrift \
           -ljwsmtp \
           -L$(LIB3RD_PATH)/lib/boost -lboost_thread-mt \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -ldl -lrt -liconv

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
S_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.s))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.s,%.o,$(S_SRCS)) 

all:  Ls  Ts
TESTSPATH = $(shell find $(LOSS_PATH) -name 'test*' -type d)

.PHONY : Ls Ts
Ts:
	@for dir in $(TESTSPATH); do echo "making in $$dir"; ( cd $$dir && make -f makefile -e CC='$(CXX)' CFLAG='$(CFLAG)'); done  

Ls: $(OBJS)
	$(CXX) -shared -Wl,-soname,libloss.so -o libloss.so $(OBJS) -lc -lrt -ldl $(LDFLAGS)
	mv libloss.so $(LOSS_PATH)/lib/
	cp $(LOSS_PATH)/lib/* $(LIB3RD_PATH)/lib/
	#ar -crv libloss.a $(OBJS)
	#ranlib libloss.a



%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
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
	rm -f $(Ls)
	rm -f $(LOSS_PATH)/lib/libloss.so
	@for dir in $(TESTSPATH); do echo "making in $$dir"; ( cd $$dir && make clean -f makefile -e CC='$(CXX)' CFLAG='$(CFLAG)'); done 
  
