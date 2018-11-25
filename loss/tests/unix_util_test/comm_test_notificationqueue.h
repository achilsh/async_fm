#pragma once

#include <iostream>
#include "Notification.h"
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



