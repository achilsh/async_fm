CC = gcc
CXX = g++
CFLAGS = -g -O0 -Wall
CPPFLAGS =  -D_GNU_SOURCE=1 -fPIC  -std=c++11 -DREDIS_TEST 
CXXFLAG = -O0 -Wall -ggdb -m64 -Wl,--export-dynamic -std=c++11 -DREDIS_TEST


ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif
LIB3RD_PATH = ../../l3oss
LOSS_PATH = ../../loss
STARSHIP_PATH=../../Starship
CNFAGENT_PATH = .

VPATH = $(CNFAGENT_PATH)/src
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I /usr/include \
			 -I $(STARSHIP_PATH)/include \
       -I $(CNFAGENT_PATH)/include  \
       -I $(LIB3RD_PATH)/include/libevent \
       -I $(LIB3RD_PATH)/include/mysql \
       -I $(LIB3RD_PATH)/include/boost  \
       -I $(LIB3RD_PATH)/include \
       -I $(LOSS_PATH)/src/LibComm/include \
       -I $(LOSS_PATH)/src/CnfComm/include \
       -I $(LOSS_PATH)/src/RedisClient/include \
       -I $(LOSS_PATH)/src/Shm/include \
       -I $(LOSS_PATH)/src  

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LOSS_PATH)/lib -lloss -L$(LIB3RD_PATH)/lib \
           -llog4cplus \
           -lqlibc \
           -levent \
           -lev \
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

TARGET = cnf_agent_daemon

all: $(TARGET)

cnf_agent_daemon: $(OBJS)
	$(CXX) $(CXXFLAG) -o $@ $(OBJS) $(LDFLAGS) $(INC)
	mv -f $@ $(CNFAGENT_PATH)/bin/

%.o:%.cpp
	$(CXX) $(INC) $(CPPFLAGS) $(CFLAGS) $(DEFINE_SERVER) -c  -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CPPFLAGS) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CFLAGS) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
	rm -f $(CNFAGENT_PATH)/bin/$(TARGET)
        
        
