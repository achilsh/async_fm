#include <iostream>
#include "mythread.h"

namespace  loss
{

class MyRunnable: public Runnable
{
 public:
  MyRunnable(int x);
  virtual ~MyRunnable();
  virtual void Run();

 private:
  int m_iX;
};

MyRunnable::MyRunnable(int x): m_iX(x)
{

}
MyRunnable::~MyRunnable()
{

}

void MyRunnable::Run()
{
    while(true)
    {
        std::cout << "x: " << m_iX << std::endl;
        ::sleep(1);
    }
}

void test_func(void *pdata)  
{
    int *i_pData = (int*)pdata;
    if (i_pData)
    {
        (*i_pData)  *= 10;
        std::cout << "in func, x: " << *i_pData << std::endl;
    }
}
void thread_test()
{
    MyThread  funcThread;
    int x = 10;
    funcThread.Start(test_func, &x);
    funcThread.Join();
    std::cout << "y: " << x << std::endl;

    std::shared_ptr<MyRunnable> Work(new MyRunnable(100));
    MyThread testThread;
    testThread.Start( Work );
    testThread.Join();
}

}
