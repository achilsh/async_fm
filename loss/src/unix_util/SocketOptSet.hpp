/**
 * @file: SocketOptSet.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-15
 */
#ifndef __SOCKETOPT_SET_HPP__
#define __SOCKETOPT_SET_HPP__

#include <string>
#include <map>
#include <iostream>
#include <sstream>


#ifdef __cplusplus
    extern "C" {
#endif 

#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
    }
#endif


namespace loss{

struct SocketOpt {
    SocketOpt():optName(0), fd(0),level(0),optVal(NULL),optLen(0) {
    }
    ~SocketOpt(){
        optName = fd = level = optLen = 0;
        optVal = NULL;
    }
    void SetOptName(int iName) {
        optName = iName;
    }
    void SetOptFd(int fd) {
        this->fd = fd;
    }
    void SetOptLevel(int level) {
        this->level = level;
    }
    void SetOptVal(void* val) {
        optVal = val;
    }
    void SetOptLen(socklen_t len) {
        optLen = len;
    }
    void Clear() {
        optName = 0;
        fd = 0; 
        level = 0;
        optVal = NULL;
        optLen = 0;
    }

    //
    int optName;
    int fd;
    int level;
    void *optVal;
    socklen_t optLen;
};

typedef std::map<int,SocketOpt>::iterator SockOptIter; 

class SettigSocketOpt {
    public:
     SettigSocketOpt();
     virtual ~SettigSocketOpt();
     //
     SockOptIter AddNameOpt(int iVal);
     bool AddFdOpt(int iOptName, int iVal);
     bool AddLevelOpt(int iOptName, int iVal);
     bool AddValOpt(int iOptName, void* optVal);
     bool AddLenOpt(int iOptName,socklen_t optVal);
     //
     bool AddOpt(int iOptName, struct SocketOpt& optVal);
     //
     bool SetOpt();
     //
     std::string GetErrMsg() { return m_ErrMsg; }
    private:
     std::map<int,SocketOpt> m_NameSocketOpt;
     std::string m_ErrMsg;
};

}

#endif

