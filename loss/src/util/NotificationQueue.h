/**
 * @file: NotificationQueue.h
 * @brief: 
 *      NotiﬁcationQueue can be used to send notiﬁcations asynchronously from
 *      one thread to another.
 *
 *
 * @author:  wusheng Hu
 * @version: 
 * @date: 2018-11-22
 */


#ifndef _NOTIFICATIONQUEUE_H_
#define _NOTIFICATIONQUEUE_H_

#include <mutex>
#include <deque>
#include <memory>
#include <condition_variable>
#include "Notification.h"


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

/////////////////////////////////////////////////////

class NotificationQueue 
{
 public:
  NotificationQueue();
  virtual ~NotificationQueue();
  void EnqueueNotification( std::shared_ptr<Notification> ptrNf );
  void EnqueueUrgentNotification( std::shared_ptr<Notification> ptrNf );

  std::shared_ptr<Notification> DequeueNotification();
  std::shared_ptr<Notification> WaitDequeueNotification();

  std::shared_ptr<Notification> WaitDequeueNotification(long ts_ms);

  void WakeUpAll();
  bool Empty() const;
  int Size() const;
  void Clear();
  bool HasIdleEndNodeThreads() const;

 private:
  struct WaitNode
  {
      WaitNode() : m_Node(nullptr) { }
      virtual ~WaitNode() { } 

      std::shared_ptr<Notification> m_Node;
      Event                         m_WaitEvent;
  };

 private:
  mutable std::mutex m_Mtx;
  std::deque<std::shared_ptr<Notification>>  m_qNotification;
  std::deque<std::shared_ptr<WaitNode>> m_WaitQueue;
};

}
#endif
