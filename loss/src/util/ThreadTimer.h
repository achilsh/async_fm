#ifndef _THREADTIMER_H_
#define _THREADTIMER_H_

/**
* This class implements a thread-based timer.
* A timer starts a thread that first waits for a given start interval.
* Once that interval expires, the timer callback is called repeatedly
* in the given periodic interval. If the interval is 0, the timer is only
* called once.
* The timer callback method can stop the timer by setting the 
* timer's periodic interval to 0.

* The timer callback runs in its own thread, so multithreading
* issues (proper synchronization) have to be considered when writing 
* the callback method.

* The exact interval at which the callback is called depends on many 
* factors like operating system, CPU performance and system load and
* may differ from the specified interval.

* The time needed to execute the timer callback is not included
* in the interval between invocations. For example, if the interval
* is 500 milliseconds, and the callback needs 400 milliseconds to
* execute, the callback function is nevertheless called every 500
* milliseconds. If the callback takes longer to execute than the
* interval, the callback function will not be called until the next
* proper interval. The number of skipped invocations since the last
* invocation will be recorded and can be obtained by the callback
* by calling skipped().

* The timer thread is taken from a thread pool, so
* there is a limit to the number of available concurrent timers.
***/

#include <memory>
#include <mutex>
#include <chrono>

#include "Event.h"
#include "ThreadPool.h"
#include "mythread.h"

using namespace std::chrono;

namespace loss
{

class TimerCallbackInterface;

class ThreadTimer: public Runnable
{
 public:
  /**
   * @brief: ThreadTimer 
   *    Creates a new timer object. StartInterval and periodicInterval
   *    are given in milliseconds. If a lPeriodcInterval of 0 is specified,
   *    the callbacke will only be called once, after lStartInterval expires
   *
   * @param lStartInterval, milliseconds 
   * @param lPeriodcInterval, milliseconds
   */
  ThreadTimer( long lStartInterval = 0, long lPeriodcInterval = 0 );
  virtual ~ThreadTimer();
  /**
   * @brief: Start 
   *    Start the timer
   *    create the TimerCallback as follows:
   *    std::shared_ptr<TimerCallback<MyClass>> callback(new (*this, &MyClass::OnTimer));
   *    timer.Start( callbacke, tp )
   *
   * @param method
   * @param tp
   */
  void Start(std::shared_ptr<TimerCallbackInterface> method, ThreadPool* tp);

  /**
   * @brief: Stop 
   *    Stops the timer. If the callback method is currently running,
   *    it will be allowed to finish first.
   *
   *    WARNING: Never call this method from within the callback method,
   *    as a deadlock would result. To stop the timer from within the callback
   *    method, call Restart(0)
   *
   */
  void Stop();

  /**
   * @brief: Restart 
   *    Restarts the periodic interval. If the callback method is already
   *    running, nothing will happen.
   */
  void Restart();

  /**
   * @brief: Restart 
   *    Sets a new periodic interval and restarts the timer. An interval of
   *    ms(0) will stop the timer.
   *    
   * @param ms
   */
  void Restart(long ms);
  long GetStartInterval() const;

  /**
   * @brief: SetStartInterval 
   *    Sets the start interval. Will only be effective before Start() is
   *    called.
   * @param ms
   */
  void SetStartInterval(long ms);
  long GetPeriodicInterval() const;

  /**
   * @brief: SetPeriodicInterval 
   *    Sets the periodic interval. If the timer is already running 
   *    the new interval will be effective when the current interval expires
   * @param ms
   */
  void SetPeriodicInterval(long ms);

  /**
   * @brief: Skipped 
   *    Returns the number of skipped invocations since the last invocation.
   *    Skipped invocations happen if the timer callback function takes 
   *    longer to execute than the timer interval.
   * @return 
   */
  long Skipped() const;

 protected:
  virtual void Run();

 private:
  volatile long  m_lStartInterval;    //ms
  volatile long  m_lPeriodicInterval; //ms

  Event          m_evWakeUp;
  Event          m_evDone;
  long           m_lSkippedTimes;

  std::shared_ptr<TimerCallbackInterface>  p_TmrCallBack;
  time_point<system_clock, duration<int,std::micro>>  m_tmPointNextCall;
  mutable std::mutex       m_Mutex;

  ThreadTimer(const ThreadTimer&);
  ThreadTimer& operator = (const ThreadTimer&);
};


/**
 * @brief:  This is the base class for all instantiations of the TimerCallback
 * template.
 *  
 */
class TimerCallbackInterface
{
 public:
  TimerCallbackInterface();
  TimerCallbackInterface(const TimerCallbackInterface& callback);
  virtual ~TimerCallbackInterface();

  TimerCallbackInterface& operator = (const TimerCallbackInterface& tmrInterface);
  virtual void Invoke( ThreadTimer& tmr ) const  = 0;
  virtual std::shared_ptr<TimerCallbackInterface> Clone() const = 0;
};

/**
 * @brief: 
 *
 *  This template class implements an adapter that sits between
 *  a Timer and an object's method invoked by the timer.
 *  See the Timer class for information on how
 *  to use this template class.
 *
 */

template<class C>
class TimerCallback: public TimerCallbackInterface
{
 public:
  typedef void (C::*Callback)(ThreadTimer&);

  TimerCallback(std::shared_ptr<C> obj, Callback method): m_pObj(obj), m_Method(method) {
  }

  TimerCallback(const TimerCallback& tmrcallback): m_pObj(tmrcallback.m_pObj), m_Method(tmrcallback.m_Method) 
  {
  }

  virtual ~TimerCallback()
  {
  }

  TimerCallback& operator = ( const TimerCallback& tmrcallback )
  { 
      if (&tmrcallback != this)
      {
          m_pObj = tmrcallback.m_pObj;
          m_Method = tmrcallback.m_Method;
      }
      return *this;
  }

  virtual void Invoke(ThreadTimer& tmr) const
  {
      if (m_pObj)
      {
          (m_pObj.get()->*m_Method)(tmr);
      }
  }

  virtual std::shared_ptr<TimerCallbackInterface> Clone() const 
  {
    return std::make_shared<TimerCallback>(*this);
  }
 private:
  TimerCallback();
  std::shared_ptr<C>    m_pObj;
  Callback              m_Method;
};

/////////////////////////////////////////////////////////////////

}
#endif
