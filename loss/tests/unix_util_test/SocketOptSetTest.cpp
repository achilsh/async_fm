#include "SocketOptSet.hpp"
//
#include "num2str.h"

#include "str2num.h"
#include "PidFile.h"
#include <unistd.h>
#include <string.h>
#include "LockQueue.h"
#include <unistd.h>
#include <thread>
#include "FreeLockQueue.h"

#include "RWMutex.h"
#include "Lock.h"

#include "Notification.h"
#include "NotificationCenter.h"
#include "Observer.h"
#include "test_notify.h"

using namespace loss;

struct QueueNode
{
    QueueNode():x(0),y(0) {}
    int x;
    int y;
    void SetX(int xx) 
    {
        x = xx;
    }
    void SetY(int yy)
    {
        y = yy;
    }
    void Print() 
    {
        std::cout << "x: " << x << ", y:" << y << std::endl;
    }
    std::string GetXY()
    {
        std::string xy;
        xy.append("x: " + std::to_string(x) + ", y: " + std::to_string(y));
        return  xy;
    }
};

class LockQeueTest
{
 public:
  LockQeueTest() 
  {

  }
  virtual ~LockQeueTest()
  {

  }
  void funcProduce()
  {
      std::cout << "produce run, pid: " <<  std::this_thread::get_id()  << std::endl;
      while(1)
      {
          for (int i = 0; i < 10; i++)
          {
              QueueNode node;
              node.SetX(i);
              node.SetY(i + 1000);
              std::cout << "produce set x: " << i << ", y: " << i+ 1000 << std::endl;
              m_NodeList.Put(node);
              //usleep(10000);
          }
      }
  }
  void funConsumer()
  {
      std::cout << "consumer run, pid: " <<  std::this_thread::get_id()  << std::endl;
      while(1)
      {
          QueueNode node;
          m_NodeList.Get(&node);
          //node.Print();
          std::cout << "consumer run, pid: " <<  std::this_thread::get_id()  
              << ", " << node.GetXY() << std::endl;
      }
  }
  void Run()
  {
      std::thread m_threadProducer(&LockQeueTest::funcProduce, this);
      sleep(10);

      std::thread m_threadConsumer1st(&LockQeueTest::funConsumer, this);
      std::thread m_threadConsumer2nd(&LockQeueTest::funConsumer, this);
      sleep(2);

      m_threadConsumer1st.join();
      m_threadConsumer2nd.join();
      m_threadProducer.join();
  }

 private:
  LockQueue<QueueNode> m_NodeList;
};

class FreeLockQueTest
{
 public: 
  FreeLockQueTest()
  {
  }
  virtual ~FreeLockQueTest()
  {
  }
  
  void funCon()
  {
      std::cout << "consume run, pid: " <<  std::this_thread::get_id()  << std::endl;
      while(1)
      {
          QueueNode node;
          if (false == m_NodeList.DeQue(&node))
          {
              std::cout << "consume run fail, pid: " << std::this_thread::get_id()
                  << ",mq size: " << m_NodeList.Size() <<  std::endl;;
              sleep(2);
              continue;
          }

          //node.Print();
          std::cout << "consume run, pid: " <<  std::this_thread::get_id()  
              << ", " << node.GetXY() << std::endl;
          //usleep(500000);
          usleep(40000);
          //sleep(3);
      }
  }
  void funcProd()
  {
      std::cout << "produce run, pid: " <<  std::this_thread::get_id()  << std::endl;
      int i = 0;
      while(1)
      {
          {
              QueueNode node;
              node.SetX(i);
              node.SetY(i + 1000);
              std::cout << "produce set x: " << i << ", y: " << i+ 1000 << std::endl;
              if (false == m_NodeList.EnQue(node))
              {
                  std::cout << "que is full, size: " << m_NodeList.Size() << std::endl;
                  sleep(1);
              }
              else
              {
                  std::cout << "produce, mq size: " << m_NodeList.Size() << std::endl;
                  //sleep(1);
                  //usleep(10);
                  usleep(10000);
              }
              i++;
          }
      }
  }
  
  void Run()
  {
    //std::thread m_Prod2nd(&FreeLockQueTest::funcProd, this);
    std::thread m_ConA(&FreeLockQueTest::funCon, this);
    std::thread m_ConB(&FreeLockQueTest::funCon, this);
    sleep(3);
    std::thread m_Prod1st(&FreeLockQueTest::funcProd, this);
    m_ConA.join();
    m_ConB.join();
    m_Prod1st.join();
   // m_Prod2nd.join();
  }


 private:
  FreeLockQue<QueueNode> m_NodeList;
};

class Test {
 public:
  Test();
  virtual ~Test();
  int main(int argc, char**argv);
 private:
  SettigSocketOpt m_Test;
  loss::RWMutex m_rwMutex;
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

void Num2str()
{
    uint32_t x = UINT32_MAX;  //UINT32_MIN ; UINT32_MAX
    char buf[10];
    char *ptr = itoa_u32(x,buf );
    std::cout << ptr <<", " << buf << std::endl;
    
    int32_t xx  = INT32_MAX; //INT32_MAX ; INT32_MIN
    ptr = itoa_32(xx, buf);
    std::string sBuf(buf);
    std::cout << ptr <<", " << sBuf << std::endl;

    int64_t i64x  = INT64_MAX; //INT64_MIN
    char i64buf[30];
    ptr = itoa_64(i64x, i64buf);
    sBuf.assign(i64buf, strlen(i64buf)+1);
    std::cout << ptr <<", " << sBuf << std::endl;
    
    uint64_t ui64x  =  UINT64_MAX; // 
    char ui64buf[40];
    ptr = itoa_u64(ui64x, ui64buf);
    sBuf.assign(ui64buf, strlen(ui64buf)+1);
    std::cout << ptr <<", " << sBuf << std::endl;
}


int Test::main(int argc, char**argv) {
    std::cout << "==================== notification ===================" << std::endl;

    NotificationCenter  nc;
    std::shared_ptr<TestObject>  tmpObj(new TestObject());
    Observer<TestObject, TestNotification> ob(tmpObj, &TestObject::DoNotifyMsg);
    nc.addObserver(ob);
    //post notify 
    nc.postNotification(std::make_shared<TestNotification>());
    
    std::cout << "has observer: " << nc.hasObserver(ob) << std::endl;
    std::cout << "========= Num2String test begin =========================" <<std::endl;
    Num2str();
    std::cout << "========= Num2String test end   =========================" <<std::endl;
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
    
    uint64_t uLL_host = 0x1234;
    uint64_t uLL_net = Comm::htonll(uLL_host);
    std::cout << "host val: " << std::hex << uLL_host << std::endl;
    std::cout << "net val: " << std::hex << uLL_net << std::endl;
    //
    uint64_t uLL_hnh = Comm::ntohll(uLL_net);
    std::cout << "host->net->host val: " << std::hex << uLL_hnh << std::endl;
    std::cout << "is little end: " << std::dec << Comm::LittleEnd  << std::endl;


    std::cout << "------- test rw lock --------" << std::endl;
    {
        ReadLock r(m_rwMutex);
        std::cout << "===== rlock ========" << std::endl;
    }
    {
       WriteLock w(m_rwMutex) ;
       std::cout << "====== wlock =======" << std::endl;
    }
    //
    //LockQeueTest testLockQueue;
    //testLockQueue.Run();
    //
    FreeLockQueTest testFreLockQue;
    testFreLockQue.Run();
    return 0; 
}

