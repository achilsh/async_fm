#include <iostream>
#include "Notification.h"
#include "NotificationQueue.h"
#include "mythread.h"
#include <unistd.h>

using namespace loss;
using namespace std;

class MyNotification: public Notification
{
 public:
  MyNotification( int iData ) { m_iData = iData; }
  virtual ~MyNotification() {}

  int GetData()
  {
      return m_iData;
  }

 private:
  int m_iData;
};


class MyWork: public  Runnable
{
 public:
  MyWork(const std::string& sName, NotificationQueue& nQueue)
      :m_sName( sName ), m_nQueue(nQueue) 
  {
  }
  virtual ~MyWork() {}

  void Run() 
  {
      while(true)
      {
          std::cout << "consumer Run: " << std::endl;
          std::shared_ptr<Notification> ptrNf = m_nQueue.WaitDequeueNotification();
          if (ptrNf)
          {
              std::shared_ptr<MyNotification> shptrMyNf = std::dynamic_pointer_cast<MyNotification>( ptrNf );
              if ( shptrMyNf != nullptr )
              {
                  std::cout << "consumer get val: " << shptrMyNf->GetData() << std::endl;
                  usleep(1000);
              }
          }
          else
          {
              std::cout << "thread run work() exit" << std::endl;
              break;
          }
      }
  }

 private:
  std::string m_sName;
  NotificationQueue& m_nQueue;
};


//////////////////////////////////////////////////////////////////////////
void Test_Interface()
{
    NotificationQueue oneNotificationQueue;
    std::shared_ptr<Runnable> w1( new  MyWork("w1", oneNotificationQueue) );
    std::shared_ptr<Runnable> w2( new  MyWork("w2", oneNotificationQueue) );

    MyThread t1, t2;

    t1.Start(w1);
    t2.Start(w2);
    sleep(1);

    std::cout << "begin to add notification to queue" << std::endl;
    for ( int i= 0; i < 500; i++ )
    {
        oneNotificationQueue.EnqueueNotification( std::make_shared<MyNotification>(i) );
        usleep(2000);
    }
    while( !oneNotificationQueue.Empty() ) sleep(1);

    oneNotificationQueue.WakeUpAll();
    t1.Join();
    t2.Join();
}



