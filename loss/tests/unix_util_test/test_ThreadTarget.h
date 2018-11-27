#pragma once

#include "mythread.h"
#include <iostream>

using namespace std;

namespace loss
{
    class MyObj
    {
     public:
      MyObj() {}
      virtual ~MyObj() {}
      static void doSomething() { std::cout << "static doSomething() " << std::endl; }
    };
    
    void test_ThreadTargetInterface()
    {
        std::shared_ptr<ThreadTarget> target = std::make_shared<ThreadTarget>(&MyObj::doSomething);
        MyThread mythread;
        mythread.Start(target);
        mythread.Join();
    }
}

