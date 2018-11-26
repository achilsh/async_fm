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
     typedef void (*Callable)(void*);
     MyThread();
     virtual ~MyThread();
     
     /**
      * @brief: Start 
      *
      * @param runable
      *
      * Note that the given Runnable object must remain
      *   valid during the entire lifetime of the thread
      *
      * @return 
      */
     bool Start( std::shared_ptr<Runnable> runable );

     //接收函数指针和入参作为 线程执行入口
     bool Start(Callable target, void *pData = nullptr);

     void Join();
    private:
     void func();

    private:
     struct ThreadData
     {
         std::shared_ptr<Runnable> m_Runable;
         std::thread::id  m_ThreadId;
         std::thread m_Thread;
     };

     std::shared_ptr<ThreadData> m_ThreadData;
};


class Runnable
{
 public:
  Runnable() {} 
  virtual ~Runnable() {}
  virtual void Run() = 0;
};

class CallableHolder: public Runnable
{
 public:
  CallableHolder(MyThread::Callable callable, void *pData);
  virtual ~CallableHolder();
  virtual void Run(); 

 private:
  MyThread::Callable  m_Callable;
  void* m_pData;

};

}
#endif
