#include "SocketOptSet.hpp"
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

    return 0; 
}

