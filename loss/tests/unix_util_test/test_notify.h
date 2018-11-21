#include <iostream>
#include "Notification.h"
#include "NotificationCenter.h"
#include "Observer.h"

//define notification 
using namespace loss;

class TestNotification: public Notification
{
 public:
  TestNotification() 
  {
  }
  virtual ~TestNotification() 
  {
  }
    
 public:
  void PrintTestNotify() 
  {
      std::cout << "this test notification mesg" << std::endl;
  }
};

//define object  test
class TestObject 
{
 public:
  TestObject() {} 
  virtual ~TestObject() {} 

  void DoNotifyMsg(TestNotification* msg) 
  {
      if (msg) 
      {
          msg->PrintTestNotify();
          std::cout << "notify name: " <<  msg->name() << std::endl;
      }
  }
};

#if 0
int main()
{
    NotificationCenter  nc;
    std::shared_ptr<TestObject>  tmpObj(new TestObject());
    Observer<TestObject, TestNotification> ob(tmpObj, &TestObject::DoNotifyMsg);
    nc.addObserver(ob);
    //post notify 
    nc.postNotification(std::make_shared<TestNotification>());
    
    std::cout << "has observer: " << nc.hasObserver(ob) << std::endl;
    return 0;
}
#endif
