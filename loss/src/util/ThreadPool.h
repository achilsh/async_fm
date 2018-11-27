#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <string>
#include <vector>
#include <memory>
#include <mutex>


namespace loss
{

class Runnable;
class ThreadNode;

class ThreadPool
{
 public:
  ThreadPool(int iMinCapacity = 2, int iMaxCapacity = 16,
             int iIdleTime = 60);
  virtual ~ThreadPool();

  void AddCapacity(int iAddNums);
  int  Capacity( ) const;

  /**
   * @brief: UsedThreadNums 
   *        获取已分配,非空闲的线程个数
   * @return 
   */
  int UsedThreadNums() const;

  /**
   * @brief: AllocatedSize
   *    
   * @return  返回已经分配的线程节点数, 线程队列的长度
   */
  int AllocatedSize() const;

  /**
   * @brief: AvailableThreadNums 
   *        返回可用线程节点数,包括空闲线程节点和未分配的线程节点数
   * @return 
   */
  int AvailableThreadNums() const;

  void Start(std::shared_ptr<Runnable> target);

  void StopThreadPool();
  void JoinThreadPool();

  /**
   * @brief: CollectThreadPool()
   *   Stops and removes no longer used threads from the thread pool.
   */
  void CollectThreadPool();

 protected:
  std::shared_ptr<ThreadNode> GetThreadNode();
  std::shared_ptr<ThreadNode> CreateThreadNode();

  void AdjustThreadPool();

 private:
  ThreadPool(const ThreadPool&  tp);
  ThreadPool& operator = (const ThreadPool& tp);

  int                                       m_iAdjustThreshold;

  int                                       m_iMinCapacity;
  int                                       m_iMaxCapacity;
  int                                       m_iIdleTime;
  mutable std::mutex                        m_mMutex;
  std::vector<std::shared_ptr<ThreadNode>>  m_vThreads;
};

}

#endif
