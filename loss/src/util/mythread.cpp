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

bool MyThread::Start(Callable target, void *pData)
{
    return  this->Start(std::make_shared<CallableHolder>(target, pData));
}


void MyThread::func()
{
    m_ThreadData->m_ThreadId == std::this_thread::get_id();
    if ( m_ThreadData->m_Runable )
    {
        m_ThreadData->m_Runable->Run();
    }
    m_ThreadData->m_Runable = nullptr;
}

bool MyThread::IsRunning()
{
    if (m_ThreadData)
    {
        if ( m_ThreadData->m_Runable )
        {
            return true;
        }
    }
    return false;
}

void MyThread::Join()
{
    m_ThreadData->m_Thread.join();
}

//////////////////////////////////////////////////////
CallableHolder::CallableHolder(MyThread::Callable callable, void *pData)
    :m_Callable( callable ), m_pData(pData)
{
}

CallableHolder::~CallableHolder()
{
}

void CallableHolder::Run()
{
    m_Callable(m_pData);
}

///////////////////////////////////////////////////////
ThreadTarget::ThreadTarget(CallBack method) :m_method(method)
{
}

ThreadTarget::ThreadTarget(const ThreadTarget& te):m_method(te.m_method)
{
}

ThreadTarget::~ThreadTarget()
{
}

ThreadTarget& ThreadTarget::operator = (const ThreadTarget& te)
{
    m_method = te.m_method;
    return *this;
}

void ThreadTarget::Run()
{
    m_method();
}


}
////////////////////////////////////////////////////////
#if 0

void test_func(void *pdata)  
{
    int *i_pData = (int*)pdata;
    if (i_pData)
    {
        (*i_pData)  *= 10;
        std::cout << "in func, x: " << *i_pData << std::endl;
    }
}

int main()
{
    MyThread  funcThread;
    int x = 10;
    funcThread.Start(test_func, &x);
    funcThread.Join();
    std::cout << "y: " << x << std::endl;

    std::shared_ptr<MyRunnable> Work(new MyRunnable(100));
    MyThread testThread;
    testThread.Start( Work );
    testThread.Join();
    return 0;
}
#endif
