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
     bool IsRunning();
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


/**
 * @brief: 
 *  定义类C: 的普通成员函数: Callback作为线程函数的入口。该类既可以是一个普通的类，
 *   也可是包含线程类的 类。
 *   使用介绍:
 *
 *   class Gtest 
 *   {
 *    public:
 *       void func()
 *       {
 *           std::cout << "this Gtest::func() " << std::endl;
 *       }
 *   };
 *
    std::shared_ptr<Gtest> gtst(new Gtest());
 *
    std::shared_ptr<RunnableAdapter<Gtest>> rungtest = std::make_shared<RunnableAdapter<Gtest>>(gtst, &Gtest::func);
    MyThread  adaptThread;
    adaptThread.Start(rungtest);
    // 或者

    class GGTest 
    {
       public:
        void func()
        {
           std::cout << "this is ...." << std::endl;
        }

        void start()
        {
            std::shared_ptr<RunnableAdapter<GGTest>> ra = 
                     std::make_shared<RunnableAdapter<GGTest>>(this, GGTest::func);
            pthread_Instance = new MyThread(); 
            pthread_Instance->Start(ra)
        }

        private:
         MyThread *pthread_Instance;
    };
 *
 * @tparam C
 */
template <class C>
class RunnableAdapter: public Runnable
{
 public:
  typedef void (C::*Callback)();

  RunnableAdapter(std::shared_ptr<C> object, Callback method)
      :m_pObject(object), m_Method(method) 
  {
  }
  virtual ~RunnableAdapter() {}

  RunnableAdapter(const RunnableAdapter& ra): m_pObject(ra.m_pObject), m_Method(ra.m_Method) {} 
  RunnableAdapter& operator = (const RunnableAdapter& ra)
  {
      m_pObject = ra.m_pObject;
      m_Method = ra.m_Method;
      return *this;
  }
    
  virtual void Run()
  {
     (m_pObject.get()->*m_Method)();
  }

 private:
  std::shared_ptr<C>  m_pObject;
  Callback m_Method;
};


/**
 * @brief: 
 *      使用类的静态成员函数或者普通函数作为线程入口.
 *      eg:
 *      Thread myThread;
 *
 *      class MyObj
 *      {
 *         static void doSomething();
 *      };
 *
 *      ThreadTarget ra(&MyObj::doSomething);
 *      myThread.Start(ra);
 *
 *      or 
 *      void DoSomething(void) {  }
 *      ThreadTarget ra(DoSomething);
 *      myThread.Start(ra);
 */

class ThreadTarget: public Runnable
{
 public:
  typedef void (*CallBack)();
  ThreadTarget(CallBack method);
  ThreadTarget(const ThreadTarget& te);
  virtual ~ThreadTarget();
  ThreadTarget& operator = (const ThreadTarget& te);

  virtual void Run();
 private:
  CallBack  m_method;
};


}
#endif
