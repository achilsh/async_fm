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
void thread_test()
{
    std::shared_ptr<MyRunnable> Work(new MyRunnable(100));
    MyThread testThread;
    testThread.Start( Work );
    testThread.Join();
}

}
