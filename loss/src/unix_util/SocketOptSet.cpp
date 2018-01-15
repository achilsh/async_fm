#include "SocketOptSet.hpp"

namespace loss {

SettigSocketOpt::SettigSocketOpt():m_ErrMsg("") {
}
SettigSocketOpt::~SettigSocketOpt() {
}

SockOptIter SettigSocketOpt::AddNameOpt(int iVal) {
    SockOptIter it;
    it = m_NameSocketOpt.find(iVal);
    if (it == m_NameSocketOpt.end()) {
        SocketOpt oneOpt;
        oneOpt.SetOptName(iVal);
        std::pair<SockOptIter,bool> ret = m_NameSocketOpt.insert(std::pair<int,SocketOpt>(iVal, oneOpt));   
        return ret.first;
    }
    return it;
}

bool SettigSocketOpt::AddFdOpt(int iOptName, int iVal) {
    SockOptIter it = AddNameOpt(iOptName);
    it->second.fd = iVal;
    return true;
}

bool SettigSocketOpt::AddLevelOpt(int iOptName, int iVal) {
    SockOptIter it = AddNameOpt(iOptName);
    it->second.level = iVal;
    return true;
}

bool SettigSocketOpt::AddValOpt(int iOptName, void* optVal) {
    SockOptIter it = AddNameOpt(iOptName);
    it->second.optVal = optVal;
    return true;
}

bool SettigSocketOpt::AddLenOpt(int iOptName,socklen_t optVal) {
    SockOptIter it = AddNameOpt(iOptName);
    it->second.optLen = optVal;
    return true;
}

bool SettigSocketOpt::AddOpt(int iOptName, struct SocketOpt& optVal) {
    SockOptIter it = AddNameOpt(iOptName);
    it->second = optVal;
    return true;
}

bool SettigSocketOpt::SetOpt() {
    SockOptIter it;
    m_ErrMsg.clear();
    bool bRet = true;
    for (it = m_NameSocketOpt.begin(); it != m_NameSocketOpt.end(); ++it) {
        if (0 > ::setsockopt(it->second.fd, it->second.level,
                             it->second.optName,it->second.optVal,
                             it->second.optLen)) {
#ifdef SOCKETOPT_TEST
            std::cout << "set socketopt fail, optname: " << it->second.optName << std::endl;
#endif
            std::stringstream ios;
            ios << "err msg: " <<  strerror(errno) << ",err no: " << errno;
            m_ErrMsg.append(ios.str());
            m_ErrMsg.append(" ");
            bRet = false;
        }
    }
    return bRet;
}

////
}
