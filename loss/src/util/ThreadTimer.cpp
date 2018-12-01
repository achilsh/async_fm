#include "ThreadTimer.h"

namespace loss
{

ThreadTimer::ThreadTimer( long lStartInterval, long lPeriodcInterval )
    :m_lStartInterval(lStartInterval), m_lPeriodicInterval(lPeriodcInterval),
    m_lSkippedTimes(0), p_TmrCallBack(nullptr)
{
    std::cout << "call ThreadTimer::ThreadTimer()" << std::endl;
}


ThreadTimer::~ThreadTimer()
{
    //std::cout << "call ~ThreadTimer() -> Stop() " << std::endl;
    //this->Stop();
}

void ThreadTimer::Start(std::shared_ptr<TimerCallbackInterface> method, ThreadPool* tp)
{
    time_point<system_clock, duration<int, std::micro>>  nextInvocation = time_point_cast<duration<int, std::micro>>(system_clock::now());
    duration<int, std::micro> startIntervalDur(m_lStartInterval*1000);
    nextInvocation += startIntervalDur;

    std::lock_guard<std::mutex> lck(m_Mutex);
    if ( p_TmrCallBack )
    {
        return ;
    }

    m_tmPointNextCall = nextInvocation;
    p_TmrCallBack = method;
    m_evWakeUp.Reset();

    std::cout << "this: " << this << std::endl;
    std::shared_ptr<Runnable> pThreadTimerNode( this ); 
    tp->Start(pThreadTimerNode);
}

void ThreadTimer::Stop()
{
    std::lock_guard<std::mutex> lck(m_Mutex);
    if (p_TmrCallBack)
    {
        m_lPeriodicInterval = 0;
        m_Mutex.unlock();

        std::cout << "set wakeup notify" << std::endl;
        m_evWakeUp.Set();
        std::cout << "wait done notify" << std::endl;
        m_evDone.Wait();
        std::cout << "stop ThreadTimer succ" << std::endl;
    
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
    time_point<system_clock, duration<int,std::micro>> now_tm_micro = time_point_cast<duration<int, std::micro>>(system_clock::now());
    long tm_interval_ms = 0;
    do 
    {
        long sleep_tm_ms(0);
        do {

            now_tm_micro =  time_point_cast<duration<int, std::micro>>(system_clock::now());
            duration<int, std::micro> diff_tm = m_tmPointNextCall - now_tm_micro;
            sleep_tm_ms = diff_tm.count()/1000;
            if ( sleep_tm_ms < 0 )
            {
                if ( tm_interval_ms == 0 )
                {
                    sleep_tm_ms = 0;
                    break;
                }

                duration<int, std::micro> addDurTm( tm_interval_ms * 1000 );
                m_tmPointNextCall += addDurTm;
                ++m_lSkippedTimes;
            } 
        } while(sleep_tm_ms < 0);

        if (m_evWakeUp.TryWait(sleep_tm_ms))
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_tmPointNextCall = time_point_cast<duration<int, std::micro>>(system_clock::now());
            tm_interval_ms = m_lPeriodicInterval;
            usleep(1);
        }
        else
        {
            p_TmrCallBack->Invoke(*this);
            std::lock_guard<std::mutex> lock(m_Mutex);
            tm_interval_ms = m_lPeriodicInterval;
        }

        duration<int, std::micro> Interval_duration(tm_interval_ms * 1000);
        m_tmPointNextCall += Interval_duration;
        m_lSkippedTimes = 0;

    } while( tm_interval_ms > 0 );

    std::cout << "set evDone notify" << std::endl;
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
