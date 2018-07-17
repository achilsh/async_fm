#ifndef _redispool_test_h_
#define _redispool_test_h_

#include "feed_redis_pool.h"
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <vector>
#include <memory>
#include <string>

class RedisPoolTest
{
 public:
  RedisPoolTest();
  virtual ~RedisPoolTest();

  void StartPool();

  void RunRedisTest(const std::string& sKey);

private:
  int m_iThreadNums;
  int m_iRedisPoolNums;
  std::shared_ptr<FS_RedisInterface> m_RedisInterface;
  std::vector<std::thread> m_threadVec;
};
#endif
