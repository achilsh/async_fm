#ifndef _MYTHREAD_H_
#define _MYTHREAD_H_

#include <memory>
#include <thread>

#include <unistd.h>
#include <iostream>

using namespace std;

namespace loss 
{

class Runnable;

class MyThread
{
    public:
     MyThread();
     virtual ~MyThread();
     
     bool Start( std::shared_ptr<Runnable> runable );


     struct ThreadData
     {
         std::shared_ptr<Runnable> m_Runable;
         std::thread::id  m_ThreadId;
         std::thread m_Thread;
     };

     void Join();
    private:
     void func();

    private:
     std::shared_ptr<ThreadData> m_ThreadData;
};


class Runnable
{
 public:
  Runnable() {} 
  virtual ~Runnable() {}
  virtual void Run() = 0;
};


}
#endif
