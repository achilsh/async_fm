#include "SocketOptSet.hpp"
//
#include "str2num.h"
#include "PidFile.h"
#include <unistd.h>

using namespace loss;

class Test {
 public:
  Test();
  virtual ~Test();
  int main(int argc, char**argv);
 private:
  SettigSocketOpt m_Test;
};

int main(int argc, char** argv) {
    Test test;
    return test.main(argc, argv);
}

Test::Test() {
}

Test::~Test() {
}

template<typename T>
void PrintSrcDst(bool ret, const char* src, T* pdst)
{
    if (ret == true)
        std::cout << src <<  ", " << *pdst << std::endl;
    else 
        std::cout << "happend err " << std::endl;
}


int Test::main(int argc, char**argv) {
    int fd = 5600;
    m_Test.AddNameOpt(SO_KEEPALIVE);
    m_Test.AddFdOpt(SO_KEEPALIVE,fd);
    m_Test.AddLevelOpt(SO_KEEPALIVE,SOL_SOCKET);
    int iKeepAlive = 1;
    m_Test.AddValOpt(SO_KEEPALIVE, &iKeepAlive);
    m_Test.AddLenOpt(SO_KEEPALIVE, sizeof(iKeepAlive));
    
    //
    m_Test.AddNameOpt(TCP_KEEPIDLE);
    m_Test.AddFdOpt(TCP_KEEPIDLE,fd);
    m_Test.AddLevelOpt(TCP_KEEPIDLE,IPPROTO_TCP);
    
    int iKeepIdle = 1000;
    m_Test.AddValOpt(TCP_KEEPIDLE, &iKeepIdle);
    m_Test.AddLenOpt(TCP_KEEPIDLE, sizeof(iKeepIdle));

    m_Test.SetOpt();
    std::cout << m_Test.GetErrMsg() << std::endl;
    
    //
    uint64_t nParam1 = 0;
    const char *strParam1 = "1988901313";
    bool ret  = SafeStrToNum::StrToULL(strParam1, &nParam1);
    PrintSrcDst(ret, strParam1, &nParam1);
    
    int64_t nParam2 = 0;
    const char *strParam2 = "-9212312313131";
    ret = SafeStrToNum::StrToLL(strParam2, &nParam2);
    PrintSrcDst(ret, strParam2, &nParam2);

    uint32_t nParam3 = 0;
    const char *strParam3 = "4859681";
    ret = SafeStrToNum::StrToUL(strParam3, &nParam3);
    PrintSrcDst(ret, strParam3, &nParam3);

    int32_t nParam4 = 0;
    const char *strParam4 = "-4859681";
    ret = SafeStrToNum::StrToL(strParam4, &nParam4);
    PrintSrcDst(ret, strParam4, &nParam4);

    double nParam5 = 0;
    const char *strParam5 = "81313.08131";
    ret = SafeStrToNum::StrToD(strParam5, &nParam5);
    PrintSrcDst(ret, strParam5, &nParam5);

    float nParam6 = 0;
    const char *strParam6 = "-9234.8131";
    ret = SafeStrToNum::StrToF(strParam6, &nParam6);
    PrintSrcDst(ret, strParam6, &nParam6);

    Pidfile pidfile("./pid.log");
    ret = pidfile.SaveCurPidInFile();
    if (ret == false)
    {
        std::cout << "save pid file fail" << std::endl;
    } else 
    {
        std::cout << "save pid file succ " << std::endl;
    }
    ///sleep(100);
    return 0; 
}

