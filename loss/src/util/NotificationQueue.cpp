#include "NotificationQueue.h"
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
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

NotificationQueue::NotificationQueue()
{
}

NotificationQueue::~NotificationQueue() 
{
}

void NotificationQueue::EnqueueNotification( std::shared_ptr<Notification> ptrNf )
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    if (m_WaitQueue.empty())
    {
        std::cout << "add node " << std::endl;
        m_qNotification.push_back(ptrNf);
        return ;
    }
    std::cout << "waiting consumer queue not empty, add node to queue" << std::endl;

    std::shared_ptr<WaitNode> pWi = m_WaitQueue.front();
    m_WaitQueue.pop_front();
    pWi->m_Node = ptrNf;
    pWi->m_WaitEvent.Set();
}

void NotificationQueue::EnqueueUrgentNotification( std::shared_ptr<Notification> ptrNf )
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    if (m_WaitQueue.empty())
    {
        m_qNotification.push_front(ptrNf);
        return ;
    }
    std::shared_ptr<WaitNode> pWi = m_WaitQueue.front();
    m_WaitQueue.pop_front();
    pWi->m_Node = ptrNf;
    pWi->m_WaitEvent.Set();
}

std::shared_ptr<Notification> NotificationQueue::DequeueNotification()
{
    //std::lock_guard<std::mutex> lock(m_Mtx);
    std::shared_ptr<Notification> ptrNf(nullptr);
    if ( !m_qNotification.empty() )
    {
         ptrNf = m_qNotification.front();
        m_qNotification.pop_front();
    }
    return ptrNf;
}

std::shared_ptr<Notification> NotificationQueue::WaitDequeueNotification()
{
    std::shared_ptr<Notification> ptrNf;
    std::shared_ptr<WaitNode> ptrWnode;
    {
        std::lock_guard<std::mutex> lock(m_Mtx);
        ptrNf = this->DequeueNotification();
        if ( ptrNf ) return ptrNf;

        std::cout << "queue is empty" << std::endl;
        ptrWnode = std::make_shared<WaitNode>();
        m_WaitQueue.push_back(ptrWnode);
    }
    ptrWnode->m_WaitEvent.Wait();
    ptrNf = ptrWnode->m_Node;
    return ptrNf;
}

std::shared_ptr<Notification> NotificationQueue::WaitDequeueNotification(long ts_ms)
{
    std::shared_ptr<Notification> ptrNf(nullptr);
    std::shared_ptr<WaitNode> ptrWnode(nullptr);
    {
        std::lock_guard<std::mutex> lock(m_Mtx);
        ptrNf = this->DequeueNotification();
        if ( ptrNf ) return ptrNf;

        ptrWnode = std::make_shared<WaitNode>();
        m_WaitQueue.push_back(ptrWnode);
    }

    if ( ptrWnode->m_WaitEvent.TryWait(ts_ms) )
    {
        ptrNf = ptrWnode->m_Node;
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_Mtx);
        ptrNf = ptrWnode->m_Node;
        for (auto it = m_WaitQueue.begin(); it != m_WaitQueue.end(); ++it)
        {
            if ( *it == ptrWnode )
            {
                m_WaitQueue.erase(it);
                break;
            }
        }
    }
    return ptrNf;
}

void NotificationQueue::WakeUpAll() 
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    for( auto one: m_WaitQueue )
    {
        one->m_WaitEvent.Set();
    }
    m_WaitQueue.clear();

}

bool NotificationQueue::Empty() const
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    return m_qNotification.empty();
}

int NotificationQueue::Size() const
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    return static_cast<int>(m_qNotification.size());
}

void NotificationQueue::Clear() 
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    m_qNotification.clear();
}

bool NotificationQueue::HasIdleEndNodeThreads() const
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    return !(m_WaitQueue.empty());
}

}
