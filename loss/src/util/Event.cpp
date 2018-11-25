#include "Event.h"
#include <iostream>

namespace loss 
{

Event::Event(bool isAutoReset):m_isAuto(isAutoReset), m_State(false)
{
}

Event::~Event()
{
}

void Event::Set()
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    m_State = true;
    m_Cond.notify_all();
}

bool Event::Wait()
{
    std::unique_lock<std::mutex> lock(m_Mtx);
    m_Cond.wait(lock, [this] {return m_State;});
    if ( m_isAuto )
    {
        m_State = false;
    }
    return true;
}

bool Event::Wait(long ts_ms)
{
   return WaitImpl(ts_ms); 
}

bool Event::WaitImpl(long ts_ms)
{
    std::unique_lock<std::mutex> lock(m_Mtx);
    bool bRet = m_Cond.wait_for(lock, std::chrono::milliseconds(ts_ms), [this] {return m_State;});
    if (bRet == true && m_isAuto )
    {
        m_State = false;
    }
    return bRet ;
}

bool Event::TryWait(long ts_ms)
{
    return WaitImpl(ts_ms);
}

void Event::Reset()
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    m_State = false;
}

}
