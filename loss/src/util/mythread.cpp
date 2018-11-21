#include "mythread.h"
#include <functional>

namespace loss 
{

MyThread::MyThread()
{
    m_ThreadData = std::make_shared<ThreadData>(); 
}

MyThread::~MyThread()
{
}

bool MyThread::Start( std::shared_ptr<Runnable> runable )
{
    m_ThreadData->m_Runable =  runable;
    m_ThreadData->m_Thread = std::thread( std::bind( &MyThread::func, this)  );
    return true;
}


void MyThread::func()
{
    m_ThreadData->m_ThreadId == std::this_thread::get_id();
    m_ThreadData->m_Runable->Run();
}

void MyThread::Join()
{
    m_ThreadData->m_Thread.join();
}

}

////////////////////////////////////////////////////////
#if 0
int main()
{
    std::shared_ptr<MyRunnable> Work(new MyRunnable(100));
    MyThread testThread;
    testThread.Start( Work );
    testThread.Join();
    return 0;
}
#endif
