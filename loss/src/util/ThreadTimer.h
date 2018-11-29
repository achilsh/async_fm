#ifndef _THREADTIMER_H_
#define _THREADTIMER_H_

/**
This class implements a thread-based timer.
A timer starts a thread that first waits for a given start interval.
Once that interval expires, the timer callback is called repeatedly
in the given periodic interval. If the interval is 0, the timer is only
called once.
The timer callback method can stop the timer by setting the 
timer's periodic interval to 0.

The timer callback runs in its own thread, so multithreading
issues (proper synchronization) have to be considered when writing 
the callback method.

The exact interval at which the callback is called depends on many 
factors like operating system, CPU performance and system load and
may differ from the specified interval.

The time needed to execute the timer callback is not included
in the interval between invocations. For example, if the interval
is 500 milliseconds, and the callback needs 400 milliseconds to
execute, the callback function is nevertheless called every 500
milliseconds. If the callback takes longer to execute than the
interval, the callback function will not be called until the next
proper interval. The number of skipped invocations since the last
invocation will be recorded and can be obtained by the callback
by calling skipped().

The timer thread is taken from a thread pool, so
there is a limit to the number of available concurrent timers.
***/

namespace loss
{

class ThreadTimer: protected Runnable
{
 public:
  ThreadTimer( long lStartInterval = 0, long lPeriodcInterval = 0 );
  virtual ~ThreadTimer();
  void Start(std::shared_ptr<TimerCallbackInterface> method, ThreadPool& tp);
  void Stop();
  void Restart();
  void Restart(long ms);
  long GetStartInterval() const;
  void SetStartInterval(long ms);
  long GetPeriodicInterval() const;
  void SetPeriodicInterval(long ms);
  long Skipped() const;

 protected:
  virtual void Run();

 private:
  volatile long  m_lStartInterval;
  volatile long  m_lPeriodicInterval;

  Event          m_evWakeUp;
  Event          m_evDone;
  long           m_lSkippedTimes;

  std::shared_ptr<TimerCallbackInterface>  p_TmrCallBack;
  //TODO:
  long                     m_iClockTimeNextCall;
  mutable std::mutex       m_Mutex;

  ThreadTimer(const ThreadTimer&);
  ThreadTimer& operator = (const ThreadTimer&);
};

class TimerCallbackInterface
{
 public:
  TimerCallbackInterface();
  TimerCallbackInterface(const TimerCallbackInterface& callback);
  virtual ~TimerCallbackInterface();

  TimerCallbackInterface& operator = (const TimerCallbackInterface& tmrInterface);
  virtual void Invoke( ThreadTimer& tmr ) const  = 0;
  //TODO:
};

/**
*  This template class implements an adapter that sits between
*  a Timer and an object's method invoked by the timer.
*  See the Timer class for information on how
*  to use this template class.
*
 * */

template<class C>
class TimerCallback: public TimerCallbackInterface
{
 public:
  typedef void (C::*Callback)(ThreadTimer& );

  TimerCallback(std::shared_ptr<C> obj, Callback method): m_pObj(obj), m_Method(method) {
  }

  TimerCallback(const TimerCallback& tmrcallback): m_pObj(tmrcallback.m_pObj),
    m_Method(tmrcallback.m_Method) {
  }
  virtual ~TimerCallback()
  {  }
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
          (m_pObj->*m_Method)(tmr);
      }
  }

 private:
  TimerCallback();
  std::shared_ptr<C>    m_pObj;
  Callback              m_Method;
};
/////////////////////////////////////////////////////////////////

}
#endif
