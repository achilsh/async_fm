#ifndef _Lock_queue_h_
#define _Lock_queue_h_

#include <mutex>
#include <condition_variable>
#include <list>

namespace loss
{

template<typename T>
class LockQueue
{
 public:
    LockQueue() {}
    virtual ~LockQueue() {}
    
    void Put(const T& item)
    {
        std::lock_guard<std::mutex> lock(m_MutexLock);
        m_QList.push_back(item);
        m_Cond.notify_one();
    }

    void Get(T* item)
    {
        std::unique_lock<std::mutex> lock(m_MutexLock);
        m_Cond.wait(lock, [this] {return !m_QList.empty();});
        *item = m_QList.front();
        m_QList.pop_front();
    }

    int Size() const
    {
        std::lock_guard<std::mutex> lock(m_MutexLock);
        return m_QList.size();
    }

 private:
    LockQueue(const LockQueue& item);
    LockQueue& operator=(const LockQueue& item);
    //
    std::mutex m_MutexLock;
    std::condition_variable m_Cond;
    std::list<T> m_QList;
};

}
#endif
