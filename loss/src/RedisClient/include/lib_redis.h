/**
 * @file: lib_redis.h
 * @brief: 定义redis访问接口，包括同步和异步方式 
 * @author:  wusheng Hu
 * @version: 
 * @date: 2017-12-05
 */


#ifndef _LIB_REDIS_H_
#define _LIB_REDIS_H_

#include <functional>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <lib_string.h>

#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"

using namespace LIB_COMM;
namespace LIB_REDIS {

enum ReplyType
{
  STRING    = 1,
  ARRAY     = 2,
  INTEGER   = 3,
  NIL       = 4,
  STATUS    = 5,
  ERROR     = 6
};

//将redis 返回值用容器管理起来
class Reply
{
 public:
  ReplyType Type() const {return type_;}
  long long Integer() const {return integer_;}
  const std::string& Str() const {return str_;}
  const std::vector<Reply>& Elements() const {return elements_;}

  Reply(redisReply *reply = NULL):type_(ERROR), integer_(0)
  {
    if (reply == NULL)
      return;

    type_ = static_cast<ReplyType>(reply->type);
    switch(type_) {
      case ERROR:
      case STRING:
      case STATUS:
        str_ = std::string(reply->str, reply->len);
        break;
      case INTEGER:
        integer_ = reply->integer;
        break;
      case ARRAY:
        for (size_t i = 0; i < reply->elements; ++i){
          elements_.push_back(Reply(reply->element[i]));
        }
        break;
      default:
        break;
    }
  }

  ~Reply(){}

  void Print() const {
    if (Type() == NIL) {
      printf("NIL.\n");
    }
    if (Type() == STRING) {
      printf("STRING:%s\n", Str().c_str());
    }
    if (Type() == ERROR) {
      printf("ERROR:%s\n", Str().c_str());
    }
    if (Type() == STATUS) {
      printf("STATUS:%s\n", Str().c_str());
    }
    if (Type() == INTEGER) {
      printf("INTEGER:%lld\n", Integer());
    }
    if (Type() == ARRAY) {
      const std::vector<Reply>& elements = Elements();

      for (size_t j = 0; j != elements.size(); j++) {
        printf("%lu) ", j);
        elements[j].Print();
      }
    }
  }

 private:
  ReplyType           type_;
  std::string         str_;
  long long           integer_;
  std::vector<Reply>  elements_;
};

/**定义redis 同步访问接口***/
class Client: boost::noncopyable
{
 public:
  Client():host_("localhost"),port_(6379),pwd_(""),db_(0)
       ,connect_timeout_(3000),snd_rcv_timeout_(5000),rc_(NULL) 
  {
  }
  Client(const std::string& host, int32_t port,
         const std::string& pwd = "",  int32_t db = 0)
      :host_(host),port_(port),pwd_(pwd),db_(db)
       ,connect_timeout_(3000),snd_rcv_timeout_(5000),rc_(NULL) 
  {
  }
  int32_t Init(const std::string& host, int32_t port,
               const std::string& pwd = "",  int32_t db = 0);
  ~Client() 
  {
    Close();
  }
  //一般使用该接口。
  int32_t Command(const std::vector<std::string> &send, Reply &reply);

  const std::string& ErrMsg();
  void SetHost(const std::string& host);
  void SetPort(int32_t port);

  void SetPass(const std::string& pass);

  void SetDB(int32_t db);

  int32_t GetDB();
  int32_t SetTimeout(int64_t connect_timeout, int64_t snd_rcv_timeout);
  void GetTimeout(int64_t& connect_timeout, int64_t& snd_rcv_timeout);
 
 private:
  static const std::string& ErrorToStr(int32_t errorno);
 
 private:
  class ReplyHandle {//could also use shared_ptr instead
   public:
    ReplyHandle(redisReply *raw_reply)
        :raw_reply_(raw_reply) 
    {

    }
    ~ReplyHandle()
    {
      if (likely(NULL != raw_reply_)) {
        freeReplyObject(raw_reply_);
        raw_reply_ = NULL;
      }
    }
    redisReply* Reply() {
      return raw_reply_;
    }
   private:
    redisReply *raw_reply_;
  };

 private:
  int32_t Ping();
  int32_t Auth();
  int32_t Select();
  int32_t ConnectImpl();
  int32_t SetRedisContextTimeout();
  int32_t Connect();
  void Close();

 private:
  std::string     host_;
  int32_t         port_;
  std::string     pwd_;
  int32_t         db_;

  int64_t         connect_timeout_;//ms
  int64_t         snd_rcv_timeout_;//ms

  std::string     errmsg_;
  redisContext    *rc_;
};

//包装了同步redis 接口
class ServiceBase
{
 public:
  ServiceBase(Client& client) : client_(client), old_db_(-1){}
  ~ServiceBase(){}

  const std::string& Errmsg();
  void RecordErrmsg(const std::string& errmsg);
  bool SetDB(int32_t db);
  bool ResetDB();  
 
 protected:
  bool SelectDB(int32_t db);
 
 protected:
  std::string errmsg_;
  Client& client_;
  int32_t old_db_;
};

//所有的redis数据结构定义
class RedisHash : public ServiceBase
{
 public:
  RedisHash(Client& client) : ServiceBase(client){}
  ~RedisHash(){}

  bool hGet(const std::string& hash, const std::string& field, std::string& value);

  bool hSet(const std::string& hash, const std::string& field, const std::string& value);

  bool hDel(const std::string& hash, const std::string& field);

  bool hKeys(const std::string& hash, std::vector<std::string>& item_vec);

  bool hGetAll(const std::string& hash, std::vector<std::string>& item_vec);

  bool hMGet(const std::string& hash, std::vector<std::string>& vec_keys, std::map<std::string, std::string>& map_values);
};

class RedisList : public ServiceBase
{
 public:
  RedisList(Client& client) : ServiceBase(client){}
  ~RedisList(){}

  bool lPop(const std::string& list, std::string& value);

  bool rPop(const std::string& list, std::string& value);
  
  bool lPush(const std::string& list, const std::string& value);

  bool RPopLPush(const std::string& list_src, const std::string& list_dst, std::string& value);
  
  bool LRange(const std::string& key, int32_t start, int32_t stop, std::vector<std::string>& item_vec);
  
  bool LTrim(const std::string& key, int32_t start, int32_t stop);

  bool rPush(const std::string& list, const std::string& value);

  bool lLen(const std::string& list, int64_t& len);

  bool Remove(const std::string& list, const std::string& item);
  
  bool BRPop(const std::string& list, std::string& value, int64_t second_wait = 0); //0 for block indefinitely;

  bool BLPop(const std::string& list, std::string& value, int64_t second_wait = 0); //0 for block indefinitely

  bool BRPopLPush(const std::string& list_src, const std::string& list_dst, std::string& value, int64_t second_wait = 0);//0 for block indefinitely

 protected:
  bool BPop(const std::string& list, const std::string& cmd, std::string& value, int64_t second_wait);

  bool Pop(const std::string& list, const std::string& cmd, std::string& value);
 private:
  bool PopImpl(const std::vector<std::string>& send, std::string& key, std::string& value);

  bool PushImpl(const std::string& list, const std::string& cmd, const std::string& value);
  
};

class RedisSets : public ServiceBase
{
 public:
  RedisSets(Client& client) : ServiceBase(client){}
  ~RedisSets() {}

  bool Smembers(const std::string& sKey, std::vector<std::string>& members) ; 

  bool Sadd(const std::string& sets, const std::string& value);
  bool Sadd(const std::string& sets, const std::vector<std::string>& valuelist);
  
  bool Spop(const std::string& sets, std::string& value);
  bool Srem(const std::string& sets, std::vector<std::string>& members);
};

class RedisKey : public ServiceBase
{
 public:
  explicit RedisKey(Client& client) : ServiceBase(client) {}
  RedisKey(const std::string& key_prefix, Client& client) : ServiceBase(client),namespace_(key_prefix) {}
  ~RedisKey(){}

  void SetNamespace(const std::string& key_prefix);
  std::string GetNamespace() const;

  bool Del(const std::string& key);

  bool Exists(const std::string& key) ;
  
  bool Keys(const std::string& sKey, std::vector<std::string>& sKeyRet) ; 

  std::string Get(const std::string& key);
  
  bool Get(const std::string& key, std::string& value);

  bool TimeToLive(const std::string& key, int64_t& val);

  bool Set(const std::string& key, const std::string& value);

  bool Set(const std::string& key, const std::string& value, int64_t second);

  int32_t Setnx(const std::string& key, const std::string& value, int64_t second);

  bool Setex(const std::string& key, const std::string& value, const std::string& second); 

  bool Expire(const std::string& key, const std::string& second);

  bool Setnx(const std::string& key, const std::string& val);

  bool Incrby(const std::string& key, const std::string& val, int64_t& result);

  bool Expireat(const std::string& key, const std::string time_stamp);
  
  bool Incr(const std::string& key, int64_t& val);
  
  bool IncrBy(const std::string& key, int64_t delta, int64_t& val);
  
  bool Decr(const std::string& key, int64_t& val);
  
  bool DecrBy(const std::string& key, int64_t delta, int64_t& val);

  bool Mset(const std::map<std::string,std::string>& mpKV);
 protected:
  std::string GetRealKey(const std::string& key) ;
 private:
  std::string     namespace_;
};

class RedisWatch : public ServiceBase
{
 public:
  RedisWatch(Client& client) : ServiceBase(client) {}
  bool Watch(const std::string& key); 
  ~RedisWatch() {}
  
  bool Watch(const std::vector<std::string>& key_vec);
  bool UnWatch();
};

class RedisTransaction : public ServiceBase
{
 public:
  RedisTransaction(Client& client, const bool autoStart = true) : ServiceBase(client),in_trans_(false) {
    if (autoStart) Start();
  }
  ~RedisTransaction() {
    Rollback();
  }
  bool Start() ; 
  bool Commit() ;
  bool Rollback(); 
 protected:
  bool ExecCmd(const std::string& cmd,Reply& reply);
 private:
  bool in_trans_;
};

//同步调用redis 接口，实现发布功能
class RedisPublisher : public ServiceBase {
 public:
  RedisPublisher(Client& client) : ServiceBase(client) {}
  virtual ~RedisPublisher() {}
  int32_t Publish(const std::string& channel, const std::string& msg); 
};


class RedisLock
{
 public:
  RedisLock(Client& client,const std::string& lock_name)
      : client_(client), lock_name_(lock_name), have_locked_(false)
  {

  }

  ~RedisLock()
  {
  }

  bool TryLock(const std::string& lock_info);
  
  bool Unlock();
  
  bool ForceUnlock();
  
  bool GetLockInfo(std::string& lock_info);
  
 private:
  RedisKey    client_;
  std::string             lock_name_;
  bool                    have_locked_;
};

class RedisScopedLock
{
 public:
  RedisScopedLock(Client& client, const std::string& lock_name)
      : lock_(client, lock_name)
  {

  }
  ~RedisScopedLock()
  {
    lock_.Unlock();
  }

  bool TryLock(const std::string& lock_info);
  bool Unlock();
  bool GetLockInfo(std::string& lock_info); 
 private:
  RedisLock   lock_;
};

class RedisZset : public ServiceBase
{
 public:
  RedisZset(Client& client) : ServiceBase(client){}
  ~RedisZset() {}
  bool Zadd(const std::string& key, const std::string& member, const int64_t& score);
  
  bool Zrem(const std::string& key, const std::string& member);
  
  bool Zincrby(const std::string& key, const std::string& member, const int64_t& score);

  bool ZincrbyV1(const std::string& key, const std::string& member, const int64_t& score, int64_t& new_score );

  bool Zrange(const std::string& key, const int32_t start, const int32_t end, 
              std::vector<std::string>& members, std::vector<std::string>& scores);

  bool ZrangeByScore(const std::string& key, const int64_t& start, 
                     const int64_t& end, std::vector<std::string>& members,
                     uint32_t offset = 0, uint32_t count = 0 );

  bool ZrangeByScoreV1(const std::string& key, const int64_t& start, 
                       const int64_t& end, std::vector<std::string>& members,
                       std::vector<std::string>& scores, 
                       uint32_t offset = 0, uint32_t count = 0 );
  bool ZrevrangeByScore(const std::string& key, const int64_t& start, 
                        const int64_t& end, std::vector<std::string>& members);

};

//异步redis 客户端实现
typedef std::function<int32_t (Reply&)> AsyncHandler;

class AsyncClient {
 private:
  AsyncClient(const AsyncClient&);
  AsyncClient& operator = (const AsyncClient&);

 protected:
  //using libevent as net event proc
  void InitEvent() ; 
  void FreeEvent();  
  bool AttachEvent(); 
  void DetachEvent(); 
  void LoopEvent();  

 public:
  AsyncClient(const std::string& host = "localhost", int32_t port = 6379,
              const std::string& pwd = "",  int32_t db = 0) : event_(NULL) , m_IsConn(0), m_InitEvent(false){
    Init(host, port, pwd, db);
  }

  int32_t Init(const std::string& host, int32_t port,
               const std::string& pwd,  int32_t db);
  
  int32_t Init(const std::string& host, int32_t port,
               const std::string& pwd,  int32_t db, void *even_base);
  

  bool AsyncCommand(const std::vector<std::string>& cmd_vec, AsyncHandler async_handler);

  ~AsyncClient() {
    FreeEvent();
    Close();
  }
  const char* Errmsg(); 
  void SetErrmsg(const std::string& msg); 
  void ClearErrMsg();  
 protected:
  bool AsyncCommandImpl(const std::vector<std::string>& cmd_vec, AsyncHandler async_handler);

 protected:

  static void OnMessage(struct redisAsyncContext *ac, void *reply_data, void *privdata); 

  int32_t OnAuth(Reply& reply);  

  int32_t OnSelect(Reply& reply); 
  int32_t OnPing(Reply& reply);  

 private:
  void InitConnect(); 

  int32_t Auth();  

  int32_t Ping(); 

  int32_t Select(); 

  int32_t AsyncConnect();

  static void  OnAsyncConnect(const struct redisAsyncContext *ac, int status) ; 

  static void OnAsyncDisconnect(const struct redisAsyncContext *ac, int status); 

  void* Event() ; 

  void Clear();  

  void SetConnected();
  void Close();

 public:
  bool ConnectedOk(); 
  int  ConnectStatus();
 private:
  std::string         host_;
  int32_t             port_;
  std::string         pwd_;
  int32_t             db_;

  AsyncHandler        async_handler_;

  std::string         errmsg_;
  redisAsyncContext  *rc_;

  void               *event_;
  uint32_t           m_IsConn; /**0: dis-conn, 1: connning, 2: conned **/
  bool               m_InitEvent;
};

class SubRetProcBase {
 public:
  SubRetProcBase (const std::string& sch)
      :m_sCh(sch) {} 
  virtual ~SubRetProcBase () {}

  virtual bool operator()(const std::string& sCh,
                          const std::string& sSubRet) = 0;
 public:
  void SetLog(const log4cplus::Logger &log) {m_oLogger = log; }
  log4cplus::Logger GetTLog() { return m_oLogger; }
  std::string GetChName() { return m_sCh; }
 private:
  log4cplus::Logger m_oLogger;
  std::string m_sCh;
};

typedef std::function<int32_t (const std::string&, const std::string&)> SubscribeHandler; //订阅后收到消息的处理接口。
class RedisSubscriber {
 public:
  int32_t AsyncHandlerWrapper(Reply& reply);  

  int32_t AsyncHandlerWrapperMulti(Reply& reply);  

 protected:
  void DiscardMessage(Reply& reply); 

 public:
  RedisSubscriber(AsyncClient& client) : client_(client) {
    //
  }
  //订阅就是调用redis异步client 发送订阅命令
  //访问方法:订阅一个频道
  bool Subscribe(const std::string& channel, SubscribeHandler handler);  

  //订阅多个频道
  bool Subscribe(const std::vector<std::string>& channel_vec, SubscribeHandler handler); 

  //订阅多个，并且每个的订阅结果处理接口都可以自定义
  bool Subscribe(const std::map<std::string, SubRetProcBase*>& mpCmdHandler) ; 

  const char* Errmsg(); 

 protected:
  bool SubscribeImpl();  

 private:
  AsyncClient&                client_;
  SubscribeHandler            handler_;
  std::vector<std::string>    cmd_vec_;
  std::map<std::string, SubRetProcBase*> mp_sub_ret_proc;
 
};

//
} // redis 


#endif
