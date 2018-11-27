#include <iostream>
#include "Event.h"
#include "mythread.h" 
#include "ThreadPool.h"

using namespace std;
using namespace loss;


class TpMyRunnable: public Runnable
{
 public:
  TpMyRunnable(int x);
  virtual ~TpMyRunnable();
  virtual void Run();

 private:
  int m_iX;
};

TpMyRunnable::TpMyRunnable(int x): m_iX(x)
{

}
TpMyRunnable::~TpMyRunnable()
{

}

void TpMyRunnable::Run()
{
    while(true)
    {
        std::cout << "x: " << m_iX << std::endl;
        ::sleep(1);
    }
}

void test_thread_pool()
{
    std::shared_ptr<TpMyRunnable> Work(new TpMyRunnable(100));
    ThreadPool tp;
    tp.Start(Work);
    tp.Start(Work);
    tp.Start(Work);
    tp.Start(Work);

    tp.JoinThreadPool();

}
