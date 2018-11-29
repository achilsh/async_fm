#include "ThreadTimer.h"

namespace loss
{


ThreadTimer::ThreadTimer( long lStartInterval, long lPeriodcInterval )
    :m_lStartInterval(lStartInterval), m_lPeriodicInterval(lPeriodcInterval),
    m_lSkippedTimes(0), p_TmrCallBack(nullptr)
{
}


ThreadTimer::~ThreadTimer()
{
    //todo:
}

void ThreadTimer::Start(std::shared_ptr<TimerCallbackInterface> method, ThreadPool& tp)
{
    long  lNextCallTm = 0;
    lNextCallTm += m_lStartInterval * 1000;

    std::lock_guard<std::mutex> lck(m_Mutex);
    if ( p_TmrCallBack )
    {
        return ;
    }

    m_iClockTimeNextCall = lNextCallTm;
    p_TmrCallBack = method;
    m_evWakeUp.Reset();

    tp.Start(*this);
}

void ThreadTimer::Stop()
{
    std::lock_gurad<std::mutex> lck(m_Mutex);
    if (p_TmrCallBack)
    {
        m_lPeriodicInterval = 0;
        m_Mutex.unlock();

        m_evWakeUp.Set();
        m_evDone.Wait();
    
        m_Mutex.lock();
        p_TmrCallBack = nullptr;
    }
}

void ThreadTimer::Restart()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if ( p_TmrCallBack )
    {
        m_evWakeUp.Set();
    }
}

void ThreadTimer::Restart(long ms)
{
    if ( ms <0 )
    {
        return ;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (p_TmrCallBack)
    {
        m_lPeriodicInterval = ms;
        m_evWakeUp.Set();
    }
}

long ThreadTimer::GetStartInterval() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_lStartInterval;
}

void ThreadTimer::SetStartInterval(long ms)
{
    if ( ms < 0 )
    {
        return ;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_lStartInterval = ms;
}

long ThreadTimer::GetPeriodicInterval() const 
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_lPeriodicInterval;

}

void ThreadTimer::SetPeriodicInterval(long ms)
{
    if (ms < 0)
    {
        return ;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_lPeriodicInterval = ms;
}

void ThreadTimer::Run()
{
    long now_tm = time(NULL);
    long tm_interval = 0;
    do 
    {
        long sleep_tm(0);
        do
        {
            now_tm = time(NULL);
            sleep_tm =  (m_iClockTimeNextCall - now_tm )/1000;
            if ( sleep_tm  < 0 )
            {
                if ( tm_interval == 0 )
                {
                    sleep_tm = 0;
                    break;
                }
                m_iClockTimeNextCall +=  tm_interval *1000;
                ++m_lSkippedTimes;
            } 
        } while(sleep_tm < 0);

        if (m_evWakeUp.TryWait(sleep_tm))
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_iClockTimeNextCall = time(NULL);
            tm_interval = m_lPeriodicInterval;
        }
        else
        {
            p_TmrCallBack->Invoke(*this);
            std::lock_guard<std::mutex> lock(m_Mutex);
            tm_interval = m_lPeriodicInterval;
        }
        m_iClockTimeNextCall += tm_interval * 1000;
        m_lSkippedTimes = 0;

    } while( tm_interval > 0 );
    m_evDone.Set();
}

long ThreadTimer::Skipped() const
{
    return m_lSkippedTimes;
}

///////////////////////////////////////////////////

TimerCallbackInterface::TimerCallbackInterface()
{
    
}

TimerCallbackInterface::TimerCallbackInterface(const TimerCallbackInterface& callback) 
{

}

TimerCallbackInterface::~TimerCallbackInterface() 
{

}

TimerCallbackInterface& TimerCallbackInterface::operator = (const TimerCallbackInterface& tmrInterface)
{
    return *this;
}



}
