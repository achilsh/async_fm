#include <iostream>
#include <ctime>
#include "ThreadPool.h"
#include "mythread.h"
#include "Event.h"

namespace loss
{

class ThreadNode: public Runnable
{
 public:
  ThreadNode();
  virtual ~ThreadNode();
  void Start();
  void Start(std::shared_ptr<Runnable> runable);
  bool IsIdle();
  int  IdleTime();
  void Join();
  void ActivateThread();
  void ReleaseThread();
  virtual void Run();

 private:
  volatile bool             m_bIdle;
  volatile std::time_t      m_tIdleTime;
  std::shared_ptr<Runnable> m_pTarget;

  std::mutex    m_ThreadNodeMutex;
  MyThread      m_MyThread;
  ////////////////////////////
  Event         m_ThreadStarted;
  Event         m_TargetReady;
  Event         m_TargetCompleted;
};

ThreadNode::ThreadNode(): m_bIdle(true), m_tIdleTime(0), 
    m_pTarget(nullptr), m_TargetCompleted(false)
{
    m_tIdleTime = std::time(nullptr);
}

ThreadNode::~ThreadNode()
{
}

void ThreadNode::Start()
{
    std::shared_ptr<Runnable> ptrRunnable(this);
    m_MyThread.Start(ptrRunnable);
    m_ThreadStarted.Wait();
}

void ThreadNode::Start(std::shared_ptr<Runnable> runable)
{
    std::lock_guard<std::mutex> lock( m_ThreadNodeMutex );
    if ( m_pTarget != nullptr )
    {
        return ;
    }
    m_pTarget = runable;
    m_TargetReady.Set();
}

bool ThreadNode::IsIdle()
{
    std::lock_guard<std::mutex> lock(m_ThreadNodeMutex);
    return m_bIdle;
}

int ThreadNode::IdleTime()
{
    std::lock_guard<std::mutex> lock(m_ThreadNodeMutex);
    return (int) (time(nullptr) - m_tIdleTime);
}

void ThreadNode::Join()
{ 
    std::shared_ptr<Runnable> ptrWork(nullptr);
    {
        std::lock_guard<std::mutex> lock(m_ThreadNodeMutex);
        ptrWork = m_pTarget;
    }

    if ( ptrWork )
    {
        m_TargetCompleted.Wait();
    }
}

void ThreadNode::ActivateThread()
{
    std::lock_guard<std::mutex> lock(m_ThreadNodeMutex);
    if (m_bIdle == false)
    {
        return ;
    }
    m_bIdle = false;
    m_TargetCompleted.Reset();
}

void ThreadNode::ReleaseThread()
{
    {
        std::lock_guard<std::mutex> lock(m_ThreadNodeMutex);
        m_pTarget = nullptr;
    }

    if (m_MyThread.IsRunning())
    {
        m_TargetReady.Set();
    }
    //TODO:
    m_MyThread.Join();
}

void ThreadNode::Run()
{
    m_ThreadStarted.Set();
    while(true)
    {
        m_TargetReady.Wait();
        m_ThreadNodeMutex.lock();

        if ( m_pTarget ) //nullptr is release thread
        {
            std::shared_ptr<Runnable> pTarget = m_pTarget;
            m_ThreadNodeMutex.unlock();

            try
            {
                pTarget->Run();
            }
            catch(const std::exception& ex)
            {
                //TODO:
            }
            
            //thread work process exit, now work done. set thread to be idle
            std::lock_guard<std::mutex> lock(m_ThreadNodeMutex);
            m_pTarget = nullptr;

            m_tIdleTime = std::time(nullptr);
            m_bIdle = true;
        }
        else
        {
            m_ThreadNodeMutex.unlock();
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////
ThreadPool::ThreadPool(int iMinCapacity, int iMaxCapacity, int iIdleTime)
                        : m_iAdjustThreshold(0), m_iMinCapacity(iMinCapacity), 
                        m_iMaxCapacity(iMaxCapacity), m_iIdleTime(iIdleTime)
{
    if (iMinCapacity < 1 || m_iMaxCapacity < iMinCapacity || m_iIdleTime <= 0)
    {
        return ;
    }

    for (int iIndex = 0; iIndex < iMinCapacity; ++iIndex)
    {
        std::shared_ptr<ThreadNode> ptrThreadNode = CreateThreadNode();
        if (!ptrThreadNode)
        {
            StopThreadPool();
            return ;
        }
        m_vThreads.push_back( ptrThreadNode );
        ptrThreadNode->Start();
    }
}

ThreadPool::~ThreadPool()
{
    this->StopThreadPool();
}


void ThreadPool::AddCapacity(int iAddNums) 
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    if (m_iMaxCapacity + iAddNums >= m_iMinCapacity)
    {
        return ;
    }
    m_iMaxCapacity += iAddNums;
    //////////////////////  
    AdjustThreadPool();
}

int  ThreadPool::Capacity( ) const
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    return m_iMaxCapacity;
}

int ThreadPool::UsedThreadNums() const
{
    int iCount = 0;
    std::lock_guard<std::mutex> lock(m_mMutex);
    for ( const auto& one: m_vThreads )
    {
        if (one && !( one->IsIdle() ))
        {
            ++iCount;
        }
    }
    return iCount;
}

int ThreadPool::AllocatedSize() const
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    return (int)m_vThreads.size();
}

int ThreadPool::AvailableThreadNums() const
{
    int iCount = 0;
    std::lock_guard<std::mutex> lock(m_mMutex);
    for (const auto& one: m_vThreads)
    {
        if (one && one->IsIdle())
        {
            ++iCount;
        }
    }

    return (int) (iCount + m_iMaxCapacity - m_vThreads.size());
}

void ThreadPool::Start(std::shared_ptr<Runnable> target)
{
    std::shared_ptr<ThreadNode> ptrThreadNode(nullptr);
    ptrThreadNode = this->GetThreadNode();
    if ( ptrThreadNode )
    {
        ptrThreadNode->Start(target);
    }
}

void ThreadPool::StopThreadPool()
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    for (auto& one: m_vThreads)
    {
        if (one)
        {
            one->ReleaseThread();
        }
    }
    m_vThreads.clear();
}

void ThreadPool::JoinThreadPool()
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    for (auto& one: m_vThreads)
    {
        if (one)
        {
            one->Join();
        }
    }
    this->AdjustThreadPool();
}

void ThreadPool::CollectThreadPool()
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    AdjustThreadPool();
}

std::shared_ptr<ThreadNode> ThreadPool::GetThreadNode()
{
    std::lock_guard<std::mutex> lock(m_mMutex);
    if ( ++m_iAdjustThreshold == 32 )
    {
        AdjustThreadPool();
    }

    std::shared_ptr<ThreadNode> ptrThreadNode(nullptr);
    for (auto& one: m_vThreads)
    {
        if (!one)
        {
            continue;
        }
        //
        if (one->IsIdle())
        {
            ptrThreadNode = one;
        }
    }

    if (!ptrThreadNode)
    {
        if (m_vThreads.size() < m_iMaxCapacity)
        {
            ptrThreadNode = CreateThreadNode();
            try 
            {
                ptrThreadNode->Start();
                m_vThreads.push_back(ptrThreadNode);
            } 
            catch(const std::exception& ex)
            {
                // TODO:
                ptrThreadNode  = nullptr;
                return ptrThreadNode;
            }
        }
        else
        {
           //TODO: 
            ptrThreadNode = nullptr;
            return ptrThreadNode;
        }
    }
    /////////
    ptrThreadNode->ActivateThread();
    return ptrThreadNode;
}

std::shared_ptr<ThreadNode> ThreadPool::CreateThreadNode()
{
    return std::make_shared<ThreadNode>();
}

void ThreadPool::AdjustThreadPool()
{
    m_iAdjustThreshold = 0;
    if ( m_vThreads.size() <= m_iMinCapacity )
    {
        return ;
    }

    typedef std::vector<std::shared_ptr<ThreadNode>> VecThreadType;
    VecThreadType   vIdleThreads;
    VecThreadType   vActiveThreads;
    VecThreadType   vExpireThreads;

    vIdleThreads.reserve(m_vThreads.size());
    vActiveThreads.reserve(m_vThreads.size());

    for ( auto& one: m_vThreads )
    {
        if (one && one->IsIdle())
        {
            if (one->IdleTime() < m_iIdleTime)
            {
                vIdleThreads.push_back(one);
            }
            else
            {
                vExpireThreads.push_back(one);
            }
        }
        else
        {
            vActiveThreads.push_back(one);
        }
    }
    
    int iNumActiveThread = (int)vActiveThreads.size();
    int iLimitNums       =  (int)vIdleThreads.size() + iNumActiveThread;
    if (iLimitNums < m_iMinCapacity) 
    {
        iLimitNums = m_iMinCapacity;
    }

    vIdleThreads.insert(vIdleThreads.end(), vExpireThreads.begin(), vExpireThreads.end());
    m_vThreads.clear();

    for ( auto &one: vIdleThreads )
    {
        if (!one)
        {
            continue;
        }

        if ( iNumActiveThread < iLimitNums )
        {
            m_vThreads.push_back(one);
            iNumActiveThread++;
        }
        else 
        {
            one->ReleaseThread();
        }
    }
    m_vThreads.insert(m_vThreads.end(), vActiveThreads.begin(), vActiveThreads.end());
}



}
