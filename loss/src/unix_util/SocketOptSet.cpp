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

//////////////////////////////
bool Comm::IsLittleEnd() 
{
    uint16_t uiTestVal = 0x1234;
    char *p = (char*)&uiTestVal;
    if ((p[0] == 0x34) && (p[1] == 0x12))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool Comm::LittleEnd = Comm::IsLittleEnd();

uint64_t Comm::ntohll(uint64_t val) 
{
    return Comm::toswap(val);
}

uint64_t Comm::htonll(uint64_t val)
{
    return Comm::toswap(val);
}

uint64_t Comm::toswap(uint64_t in)
{
    if (LittleEnd == true)
    {
        /* Little endian, flip the bytes around until someone makes a
           faster/better  way to do this. */
        int64_t rv = 0;
        int8_t i = 0;
        for (;  i <8; i++)
        {
            rv = (rv <<8) | (in & 0xff);
            in >>= 8;
        }
        return rv;
    }
    else 
    {
        return in; /*  big-endian machines don't need byte swapping **/
    }
}

////
}
