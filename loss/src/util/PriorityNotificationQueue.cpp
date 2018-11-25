#include "PriorityNotificationQueue.h"
#include <iostream>

namespace loss
{

PriorityNotificationQueue::PriorityNotificationQueue()
{
}

PriorityNotificationQueue::~PriorityNotificationQueue()
{
}

void PriorityNotificationQueue::EnqueueNotification( int priority, std::shared_ptr<Notification> ptrNf )
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    if (m_WaitQueue.empty())
    {
        std::cout << "add node " << std::endl;
        m_qNotification.insert(std::multimap<int,std::shared_ptr<Notification>>::value_type( priority,ptrNf ));
        return ;
    }
    std::cout << "waiting consumer queue not empty, add node to queue" << std::endl;

    std::shared_ptr<WaitNode> pWi = m_WaitQueue.front();
    m_WaitQueue.pop_front();
    pWi->m_Node = ptrNf;
    pWi->m_WaitEvent.Set();
}

std::shared_ptr<Notification> PriorityNotificationQueue::DequeueNotification()
{
    std::shared_ptr<Notification> ptrNf(nullptr);
    if ( !m_qNotification.empty() )
    {
        std::multimap<int,std::shared_ptr<Notification>>::iterator it = m_qNotification.begin();
         ptrNf = it->second;
         m_qNotification.erase(it);
    }
    return ptrNf;
}

std::shared_ptr<Notification> PriorityNotificationQueue::WaitDequeueNotification()
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

std::shared_ptr<Notification> PriorityNotificationQueue::WaitDequeueNotification(long ts_ms)
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

void PriorityNotificationQueue::WakeUpAll() 
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    for( auto one: m_WaitQueue )
    {
        one->m_WaitEvent.Set();
    }
    m_WaitQueue.clear();

}

bool PriorityNotificationQueue::Empty() const
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    return m_qNotification.empty();
}

int PriorityNotificationQueue::Size() const
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    return static_cast<int>(m_qNotification.size());
}

void PriorityNotificationQueue::Clear() 
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    m_qNotification.clear();
}

bool PriorityNotificationQueue::HasIdleEndNodeThreads() const
{
    std::lock_guard<std::mutex> lock(m_Mtx);
    return !(m_WaitQueue.empty());
}

}
