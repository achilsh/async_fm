#ifndef _REDIS_CONN_POOL_H_
#define _REDIS_CONN_POOL_H_

#include "hiredis/hiredis.h"
#include <mutex>
#include <list>


class RedisConnPool 
{
  public:
    RedisConnPool() {}
    virtual ~RedisConnPool() {}
    virtual redisContext* Grub();

    struct RedisConnInfo {
      redisContext* conn_;
      time_t last_used_;
      bool in_use_;
      RedisConnInfo(redisContext* c): 
      conn_(c), last_used_(time(NULL)), in_use_(true) {
      }
      bool operator < (const RedisConnInfo& _item) const {
        const RedisConnInfo lhs = *this;
        return lhs.in_use_ == _item.in_use_?
        lhs.last_used_ < _item.last_used_ : lhs.in_use_;
      }
    };
    typedef std::list<RedisConnInfo> PoolT;
    typedef PoolT::iterator PoolIt;
  public:
    bool PoolIsEmpty() { return redis_pool_.empty(); }
    void RemoveConn(const redisContext* _rc);
    void RemoveAllUnusedConn() { ClearAll(false); }

  protected:
    virtual void ReleaseConn(const redisContext* _rc);
    virtual redisContext* CreateRedis() = 0;
    virtual void DestroyRedis(redisContext*) = 0;
    virtual unsigned GetMaxIdleTime() = 0;
    size_t size() const { return redis_pool_.size(); }
    void ClearAll(bool _all = true);

  private:
    //
    redisContext* FindMRU();
    void RemoveConn(const PoolIt& it);
    void RemoveOldConn();
  private:
    PoolT  redis_pool_;
    std::mutex redis_mutex_;
};
#endif
