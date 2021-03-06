DEFINE_SERVER = -DNODE_TYPE_ACCESS
CC = gcc
CXX = g++
CFLAGS = $(DEFINE_SERVER) -g -O0 #-Wall
CPPFLAGS =  -D_GNU_SOURCE=1 -D_REENTRANT -fPIC -DNODE_BEAT=10.0  -std=c++11
#CXXFLAG = -g -O2 -Wall -fdiagnostics-show-caret -Wextra -pedantic -Wstrict-aliasing -Wstrict-overflow -Woverloaded-virtual -Wold-style-cast -Wc++0x-compat -Wc++11-compat -Wundef -Wshadow -Wformat -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wno-variadic-macros -fvisibility=hidden
CXXFLAG = -O0 -Wall -ggdb -m64 -Wl,--export-dynamic -std=c++11 # -lrt -std=c++11 -pthread  #-static-libgcc -static-libstdc++



ifeq ($(unit_test),y)
CPPFLAGS += -DUNIT_TEST
endif

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
PROTO_PATH = ../../proto
STARSHIP_PATH = ../../Starship

VPATH = .
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(LIB3RD_PATH)/include \
       -I $(LOSS_PATH)/src \
       -I $(PROTO_PATH)/src \
       -I $(STARSHIP_PATH)/src 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(STARSHIP_PATH)/lib -lstarship \
           -L$(LOSS_PATH)/lib -lloss \
           -L$(LIB3RD_PATH)/lib -lmysqlclient_r \
           -L$(LIB3RD_PATH)/lib -lprotobuf \
           -L$(LIB3RD_PATH)/lib -lhiredis \
           -L$(LIB3RD_PATH)/lib -ljsoncpp \
           -L$(LIB3RD_PATH)/lib -lqlibc \
           -L$(LIB3RD_PATH)/lib -lcryptopp \
           -L$(LIB3RD_PATH)/lib -llog4cplus \
           -L$(LIB3RD_PATH)/lib -lev \
					 -L$(LIB3RD_PATH)/lib -ljemalloc \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lz -ldl -lrt

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

TARGET = AccessTest

all: $(TARGET)

AccessTest: $(OBJS)
	$(CXX) $(CXXFLAG) $(DEFINE_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)

%.o:%.cpp
	$(CXX) $(INC) $(CPPFLAGS) $(CFLAGS) $(DEFINE_SERVER) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CPPFLAGS) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CFLAGS) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
