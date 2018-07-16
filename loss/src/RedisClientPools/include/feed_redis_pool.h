#ifndef REDIS_POOL_H 
#define REDIS_POOL_H 

#include "feed_redis_conn_pool.h"
#include <condition_variable>
#include <mutex>
#include <map>
#include <vector>


class FS_RedisConnPool: public RedisConnPool {
 public:
  FS_RedisConnPool(int32_t _max_conn_nums, int32_t _max_idle_tm, 
                   const std::string& _host,
                   unsigned _port, unsigned _db_index);
  virtual ~FS_RedisConnPool();
  virtual redisContext* Grub();
  void ReleaseConn(const redisContext* _rc, bool _is_bad);
  //
 protected:
  virtual redisContext* CreateRedis();
  virtual void ReleaseConn(const redisContext* _rc);
  virtual void DestroyRedis(redisContext* _rc);
  virtual unsigned GetMaxIdleTime();
 private:
  int32_t conn_in_use_nums_;
  int32_t conn_max_nums_;
  int32_t redis_idle_max_tm_;
  //
  std::string str_host_;
  unsigned ui_port_;
  unsigned database_index_;

  std::mutex conn_in_use_mutex_;
  std::condition_variable conn_in_use_mutx_cond_;
};

//
class FS_RedisInterface: public FS_RedisConnPool {
 public:
  FS_RedisInterface(int32_t _max_conn_nums, int32_t _max_idle_tm, 
                    const std::string& _host,
                    unsigned _port, unsigned _db_index);
  virtual ~FS_RedisInterface() {}
  //key interface
  bool Ping(std::string& _ret);
  bool Del(const std::string& _key);
  bool ExpireTm(const std::string& _key, const unsigned _exp_tm);
  //hash iterface.
  bool HSet(const std::string& _key, const std::string& _field, 
            const std::string& _val);
  bool HGet(const std::string& _key, 
            const std::string& _field, std::string* _val);
  bool HMSet(const std::string& _key, 
             const std::map<std::string, std::string>& _field_val);
  bool HMGet(const std::string& _key,
             std::map<std::string, std::string>& _val);
  bool HGetAll(const std::string& _key,
               std::map<std::string, std::string>& _val);
  bool HVals(const std::string& _key, std::vector<std::string>& _val);
  bool HDel(const std::string& _key, const std::vector<std::string>& _fields);
 private:
  bool FreeReplyObj(redisReply* _preply);
  bool CheckRetInt(redisReply* _preply,
                   redisContext* _rc, const std::string& _cmd);
  bool CheckRetString(redisReply* _preply,
                      redisContext* _rc, const std::string& _cmd);
  bool CheckRetNil(redisReply* _preply, 
                   redisContext* _rc, const std::string& _cmd);
  bool CheckRetArr(redisReply* _preply, 
                   redisContext* _rc, const std::string& _cmd);
  bool CheckReplySucc(redisReply* _preply,
                      redisContext* _rc, const std::string& _cmd);  
  bool CheckStatusOK(redisReply* _preply, 
                     redisContext* _rc, const std::string& _cmd);  
  bool GetOneStringFromArr(std::string& _ret, redisReply* _reply);
  void CmdToString(const std::string& _cmd_str);
};
//
#endif
