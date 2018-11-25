#ifndef _EVENT_H_
#define _EVENT_H_

#include <mutex>
#include <deque>
#include <map>
#include <memory>
#include <condition_variable>

namespace loss
{
    //封装信号量，实现通知事件
    class Event
    {
     public:
      Event(bool isAutoReset = true);
      virtual ~Event();
      void Set();
      bool Wait();

      /**
       * @brief: Wait 
       *
       * @param ts_ms
       *
       * @return: true => wait() return succ, false => timeout
       */
      bool Wait(long ts_ms);
      bool TryWait(long ts_ms);
      void Reset(); 

     private:
      bool WaitImpl(long ts_ms);
      Event(const Event &);
      Event& operator = (const Event &);
     private:
      bool                      m_isAuto;
      volatile bool             m_State;
      std::mutex                m_Mtx;
      std::condition_variable   m_Cond;
    };

}

#endif
