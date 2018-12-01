#pragma once
#include "ThreadTimer.h"
using namespace std;
using namespace loss;


class TimercallbackWork
{
 public:
  TimercallbackWork( int x  ) : m_iX(x) {}
  virtual ~TimercallbackWork() {}

  void onTimer( ThreadTimer&  timer  ) 
  {
      std::cout << "ontimer(), x: " << m_iX << std::endl;

  }

 private:
  int m_iX;

};

void test_thread_timer()
{   
    ThreadPool tp(1,1);
    {
#if 1 
        ThreadTimer tt( 1000/*ms*/, 1000/*ms*/ );
    
        std::shared_ptr<TimercallbackWork> tmr_work(new TimercallbackWork(10));
    
        std::shared_ptr<TimerCallbackInterface> tmr(new TimerCallback<TimercallbackWork>(tmr_work, &TimercallbackWork::onTimer));
        tt.Start(tmr, &tp);
    
        sleep(2);
        std::cout << "after 5 times, stop the timer" << std::endl;
        tt.Stop();
        std::cout << "after stop timer, stop thread pool " << std::endl;
#endif
    }
    
    
    tp.StopThreadPool();
    tp.JoinThreadPool();
}
