#include <algorithm> 
#include <functional> 
#include "feed_redis_conn_pool.h"


template <typename ConnInfo> 
class TooOld : std::unary_function<ConnInfo, bool> {
 public:
  TooOld(unsigned _tmax):min_age_(time(NULL) - _tmax) {
  }
  bool operator()(const ConnInfo& _conn_info) const {
    return !(_conn_info.in_use_) && _conn_info.last_used_ <= min_age_;
  }

 private:
  time_t min_age_;
};

void RedisConnPool::ClearAll(bool _all) {
  std::lock_guard<std::mutex> lck(redis_mutex_);
  PoolIt it = redis_pool_.begin();
  while(it != redis_pool_.end()) {
    if (_all || !it->in_use_) {
      RemoveConn(it++);
    } else {
      ++it;
    }
  }
}

redisContext* RedisConnPool::FindMRU() {
  PoolIt mru_item = std::max_element(redis_pool_.begin(), redis_pool_.end());
  if (mru_item != redis_pool_.end() && !mru_item->in_use_) {
    mru_item->in_use_ = true;
    return mru_item->conn_;
  } else {
    return NULL;
  }
}

redisContext* RedisConnPool::Grub() {
  std::lock_guard<std::mutex> lck(redis_mutex_);
  RemoveOldConn();
  redisContext* mru_item = FindMRU();
  if (mru_item != NULL) {
    return mru_item;
  } else {
    redis_pool_.push_back(RedisConnInfo(CreateRedis()));
    return redis_pool_.back().conn_;
  }
}

void RedisConnPool::ReleaseConn(const redisContext* _rc) {
  std::lock_guard<std::mutex> lck(redis_mutex_);
  for (auto it = redis_pool_.begin(); it != redis_pool_.end(); ++it) {
    if (it->conn_ == _rc) {
      it->in_use_ = false;
      it->last_used_ = time(NULL);
      break;
    }
  }
}

void RedisConnPool::RemoveConn(const redisContext* _rc) {
  std::lock_guard<std::mutex> lck(redis_mutex_);
  for (auto it = redis_pool_.begin(); it != redis_pool_.end(); ++it) {
    if (it->conn_ == _rc) {
      RemoveConn(it);
      return ;
    }
  }
}

void RedisConnPool::RemoveConn(const PoolIt& it) {
  DestroyRedis(it->conn_);
  it->conn_ = NULL;
  redis_pool_.erase(it);
}


void RedisConnPool::RemoveOldConn() {
  TooOld<RedisConnInfo> too_old(GetMaxIdleTime());
  auto  it = redis_pool_.begin();
  while((it = std::find_if(it, redis_pool_.end(), too_old)) != redis_pool_.end()) {
    RemoveConn(it++);
  }
}
//
