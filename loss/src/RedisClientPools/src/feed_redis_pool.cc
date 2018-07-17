#include "feed_redis_pool.h"
#include <string.h>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

FS_RedisConnPool::FS_RedisConnPool(int32_t _max_conn_nums, int32_t _max_idle_tm,
                                   const std::string& _host,
                                   unsigned _port, unsigned _db_index):
conn_in_use_nums_(0), conn_max_nums_(_max_conn_nums),
redis_idle_max_tm_(_max_idle_tm),
str_host_(_host), ui_port_(_port), 
database_index_(_db_index){
  //
}
FS_RedisConnPool::~FS_RedisConnPool() {
  ClearAll();
}
redisContext* FS_RedisConnPool::Grub() {
  redisContext* rc = NULL;
  {
    std::unique_lock<std::mutex> lock(conn_in_use_mutex_);
    while(conn_in_use_nums_ > conn_max_nums_) {
        fprintf(stderr, "conn num is on max_conn_nums: %d, now wait to other phtread release free redis conn\n",
                conn_in_use_nums_);
        // LOG(WARNING) << "conn num is on, max_conn_nums: " << conn_in_use_nums_ 
        //    << ", now wait to other phtread release free redis conn";
        conn_in_use_mutx_cond_.wait(lock);
    }
    rc = RedisConnPool::Grub();
    if (rc != NULL) {
      ++conn_in_use_nums_;
      fprintf(stdout,"get free redis conn, now used index: %d\n", conn_in_use_nums_ );
      //VLOG(100) << "get free redis conn, now used index: " << conn_in_use_nums_;
    }
  }
  return rc;
}

void FS_RedisConnPool::ReleaseConn(const redisContext* _rc, bool _is_bad) {
  std::unique_lock<std::mutex> lock(conn_in_use_mutex_);
  RedisConnPool::ReleaseConn(_rc);
  
  if(_is_bad) {
    RedisConnPool::RemoveConn(_rc);
  }
  --conn_in_use_nums_;
  fprintf(stdout, "release redis conn, now redis conn nums: %d\n", conn_in_use_nums_);

  //VLOG(100) << "release redis conn, now redis conn nums: " << conn_in_use_nums_;
  conn_in_use_mutx_cond_.notify_one();
}

void FS_RedisConnPool::ReleaseConn(const redisContext* _rc) {
  std::unique_lock<std::mutex> lock(conn_in_use_mutex_);
  RedisConnPool::ReleaseConn(_rc);
  
  --conn_in_use_nums_;
  fprintf(stdout, "release redis conn, now redis conn nums: %d,Line: %d", conn_in_use_nums_, __LINE__);
  //VLOG(100) << "release redis conn, now redis conn nums: " << conn_in_use_nums_;
  conn_in_use_mutx_cond_.notify_one();
}

redisContext* FS_RedisConnPool::CreateRedis() {
  int32_t try_nums = 3;
  redisContext* rc = NULL;
  while(try_nums--) {
    rc = redisConnect(str_host_.c_str(), ui_port_);
    if (rc == NULL || rc->err != REDIS_OK) {
     // LOG(ERROR) << "create conn redis obj failed, ip:port " << str_host_ 
      //    << "," << ui_port_;
      if(rc) {
        redisFree(rc);
        rc = NULL;
      }
      continue;
    }
    if (database_index_ > 0) {
      redisReply *reply = (redisReply*)redisCommand(rc, "SELECT %d", database_index_);
      if (reply == NULL) {
       // LOG(ERROR) << "cmd select " << database_index_ << ",failed";
        redisFree(rc);
        rc = NULL;
        continue;
      } else {
        freeReplyObject(reply);
        break;
      }
    }
  }
  if (try_nums < 0) {
    return NULL;
  }
  return rc;
}

void FS_RedisConnPool::DestroyRedis(redisContext* _rc) {
  if (_rc != NULL) {
    redisFree(_rc);
  }
}

unsigned FS_RedisConnPool::GetMaxIdleTime() {
  return redis_idle_max_tm_;
}
//

FS_RedisInterface::FS_RedisInterface(int32_t _max_conn_nums, 
                                     int32_t _max_idle_tm, 
                                     const std::string& _host,
                                     unsigned _port, unsigned _db_index)
:FS_RedisConnPool(_max_conn_nums, _max_idle_tm, _host, _port, _db_index) {
  //
}

bool FS_RedisInterface::FreeReplyObj(redisReply* _preply) {
  if (_preply == NULL) {
    return true;
  }
  freeReplyObject(_preply);
  return true;
}
bool FS_RedisInterface::Ping(std::string& _ret) {
  redisReply *reply = NULL;
  redisContext* rc = NULL;
  rc = Grub();
  if (rc == NULL) {
   // LOG(ERROR) << "fetch redis conn obj failed for cmd: PING";
    return false;
  }
  //CmdToString("PING");
  reply = (redisReply *)redisCommand(rc, "PING");
  if (reply == NULL ) {
    ReleaseConn(rc, false);
    return false;
  }
  bool ret = true;
  do {
    if (reply->type == REDIS_REPLY_ERROR) {
     // LOG(ERROR) << "ping err, err msg: " << reply->str;
      ret = false;
      break;
    }
  } while (0);
  
  std::stringstream ss;
  ss << "PING " << reply->str;
  _ret = ss.str();
  FreeReplyObj(reply);
  ReleaseConn(rc, false);
  return ret;
}

bool FS_RedisInterface::CheckReplySucc(redisReply* _preply, redisContext* _rc, 
                                     const std::string& _cmd) {
  if(_preply == NULL) {
     // LOG(ERROR) << "cmd: " << _cmd << " err, err msg: " << _rc->errstr;
      ReleaseConn(_rc, true);
      return false;
  }
  if (_preply->type == REDIS_REPLY_ERROR) {
   // LOG(ERROR) << "cmd: " << _cmd << " err, err msg: " << _preply->str;
    fprintf(stderr, "cmd: %s, err msg: %s", _cmd.c_str(), _preply->str);
    ReleaseConn(_rc, false);
    FreeReplyObj(_preply);
    return false;
  }
  return true;
}
//
bool FS_RedisInterface::CheckRetInt(redisReply* _preply,
                                    redisContext* _rc, const std::string& _cmd) {
  if (_preply == NULL) {
    if (_rc != NULL) {
      ReleaseConn(_rc, false);
    }
    return false;
  }
  if (_preply->type != REDIS_REPLY_INTEGER) {
   // LOG(ERROR) << "cmd: " << _cmd <<" ret type err, succ ret type: " 
    //    << REDIS_REPLY_INTEGER;
    ReleaseConn(_rc, false);
    FreeReplyObj(_preply);
    return false;
  }
  return true;
}

bool FS_RedisInterface::CheckRetString(redisReply* _preply, 
                                       redisContext* _rc, const std::string& _cmd) {
  if (_preply == NULL) {
    if (_rc != NULL) {
      ReleaseConn(_rc, false);
    }
    return false;
  }
  if (_preply->type != REDIS_REPLY_STRING) {
   // LOG(ERROR) << "cmd: " << _cmd <<" ret type err, succ ret type: " 
    //    << REDIS_REPLY_STRING;
    ReleaseConn(_rc, false);
    FreeReplyObj(_preply);
    return false;
  }
  return true;
}

bool FS_RedisInterface::CheckRetNil(redisReply* _preply, 
                                    redisContext* _rc, const std::string& _cmd) {
  if (_preply == NULL) {
    if (_rc != NULL) {
      ReleaseConn(_rc, false);
    }
    return false;
  }
  if (_preply->type == REDIS_REPLY_NIL) {
   // LOG(ERROR) << "CMD: " << _cmd << " ret is nil";
    ReleaseConn(_rc, false);
    FreeReplyObj(_preply);
    return false;
  }
  return true;
}

bool FS_RedisInterface::CheckRetArr(redisReply* _preply,
                                    redisContext* _rc, const std::string& _cmd) {
  if (_preply == NULL) {
    if (_rc != NULL) {
      ReleaseConn(_rc, false);
    }
    return false;
  }
  //
  if (_preply->type != REDIS_REPLY_ARRAY) {
    //LOG(ERROR) << "cmd: " << _cmd <<" ret type err, succ ret type: " 
     //   << REDIS_REPLY_ARRAY;
    ReleaseConn(_rc, false);
    FreeReplyObj(_preply);
    return false;
  }
  return true;
}

bool FS_RedisInterface::CheckStatusOK(redisReply* _preply, redisContext* _rc,
                                     const std::string& _cmd) {
  if (_preply == NULL) {
    if (_rc != NULL) {
      ReleaseConn(_rc, false);
    }
    return false;
  }
  if (_preply->type != REDIS_REPLY_STATUS || strcmp(_preply->str, "OK") != 0) {
    //LOG(ERROR) << "CMD: " << _cmd << " ret status not ok, err msg: " << _preply->str;
    ReleaseConn(_rc, false);
    FreeReplyObj(_preply);
    return false;
  }
  return true;
}
bool FS_RedisInterface::GetOneStringFromArr(std::string& _ret, redisReply* _reply) {
  if (_reply == NULL) {
    return false;
  }
  if (_reply->type == REDIS_REPLY_ERROR || _reply->type == REDIS_REPLY_NIL ||
      _reply->type != REDIS_REPLY_STRING) {
    //VLOG(100) << "Reply type: " << _reply->type << ",Reply err msg: " << _reply->str; 
    return false;
  }
  _ret.assign(_reply->str, _reply->len);
  return true;
}
bool FS_RedisInterface::Del(const std::string& _key) {
  redisReply *reply = NULL;
  redisContext* rc = NULL;
  if (_key.empty()) {
   // LOG(ERROR) << "key is empty for DEL cmd";
    return false;
  }
  rc = Grub();
  if (NULL == rc) {
   // LOG(ERROR) << "get redis obj failed for cmd: DEL";
    return false;
  }
  std::stringstream ss;
  ss << "DEL " << _key;
  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  //CmdToString(ss.str());

  size_t args_size = 2;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "DEL";
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();

  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
 if (false == CheckReplySucc(reply, rc, "DEL")) {
   return false;
 }
 if (false == CheckRetInt(reply, rc, "DEL")) {
   return false;
 }

 ReleaseConn(rc, false);
 FreeReplyObj(reply);
 return true;
}

bool FS_RedisInterface::ExpireTm(const std::string& _key, const unsigned _exp_tm) {
  redisReply *reply = NULL;
  redisContext* rc = NULL;
  if (_key.empty()) {
   // LOG(ERROR) << "key is empty for DEL cmd";
    return false;
  }
  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: expire";
    return false;
  }
  std::stringstream ss;
  ss << "EXPIRE " << _key << " "  << _exp_tm;
  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  //CmdToString(ss.str());

  size_t args_size = 3; 
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "EXPIRE";
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size(); 
  ss.str("");
  ss << _exp_tm;
  std::shared_ptr<std::string> tmexp_str = std::make_shared<std::string>(
      ss.str());
  argv[2] = tmexp_str->c_str();
  arglen[2] = tmexp_str->size();

  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "EXPIRE")) {
    return false;
  }
  if (false == CheckRetInt(reply, rc, "EXPIRE")) {
    return false;
  }

  ReleaseConn(rc, false);
  FreeReplyObj(reply);
  return true;
}

bool FS_RedisInterface::HSet(const std::string& _key, const std::string& _field, 
                             const std::string& _val) {
  if (_key.empty()) {
    return false;
  }
  if (_field.empty() || _val.empty()) {
    //LOG(ERROR) << "field or val is empty";
    return false;
  }

  std::stringstream ss;
  ss << "HSET " << _key << " " << _field << " " << _val;

  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  //CmdToString(ss.str());
  redisReply *reply = NULL;
  redisContext* rc = NULL;

  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: HSET";
    return false;
  }
  size_t args_size = 4;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HSET";
  arglen[0] = strlen(argv[0]);

  argv[1] = cmd_key_str->c_str();
  arglen[1] =cmd_key_str->size(); 
  
  std::shared_ptr<std::string> field_str = std::make_shared<std::string>(
      _field);
  argv[2] = field_str->c_str();
  arglen[2] = field_str->size(); 
  std::shared_ptr<std::string> val_str = std::make_shared<std::string>(
      _val);
  argv[3] = val_str->c_str();
  arglen[3] = val_str->size(); 

  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HSET")) {
    return false;
  }
  if (false == CheckRetInt(reply, rc, "HSET")) {
    return false;
  }
  ReleaseConn(rc, false);
  FreeReplyObj(reply);
  return true;
}

bool FS_RedisInterface::HGet(const std::string& _key, 
                             const std::string& _field, std::string* _val) {
  if(_key.empty()) {
    return false;
  }
  if (_field.empty() || _val == NULL) {
    return false;
  }

  std::stringstream ss;
  ss << "HGET " << _key << " " << _field;

  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  //CmdToString(ss.str());
  redisReply *reply = NULL;
  redisContext* rc = NULL;
  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: HGET";
    return false;
  }

  size_t args_size = 3;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HGET";
  arglen[0] = strlen(argv[0]);

  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();

  std::shared_ptr<std::string> field_str = std::make_shared<std::string>(
      _field);
  argv[2] = field_str->c_str();
  arglen[2] = field_str->size();

  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HGET")) {
    return false;
  }
  if (false == CheckRetNil(reply, rc, "HGET")) {
    return false;
  } 
  if (false == CheckRetString(reply, rc, "HGET")) {
    return false;
  }
  //
  _val->assign(reply->str, reply->len);
  ReleaseConn(rc, false);
  FreeReplyObj(reply);

  return true;
}

bool FS_RedisInterface::HMSet(const std::string& _key, 
                              const std::map<std::string, std::string>& _field_val) {
  if (_key.empty() || _field_val.empty()) {
    return false;
  }
  std::stringstream ss;
  ss << "HMSET " << _key << "  ";
  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);

  for (auto it = _field_val.begin(); it != _field_val.end(); ++it) {
    if (it->first.empty() || it->second.empty()) {
      return false;
    }
    ss << it->first << "  " << it->second << "  ";
  }
  
  redisReply *reply = NULL;
  redisContext* rc = NULL;
  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: HMSET";
    return false;
  }
  //CmdToString(ss.str());
  size_t args_size = _field_val.size()*2 + 2;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HMSET";
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();
  size_t i = 2;

  std::vector<std::shared_ptr<std::string>> vField_DataVal;
  for (auto it = _field_val.begin(); it != _field_val.end(); ++it, ++i) {
    std::shared_ptr<std::string> field = std::make_shared<std::string>(it->first);
    argv[2*(i-1)] = field->c_str();
    arglen[2*(i-1)] = it->first.size();
    
    vField_DataVal.push_back(field);
    //
    std::shared_ptr<std::string> data_val = std::make_shared<std::string>(it->second);
    argv[2*(i-1)+1] = data_val->c_str();
    arglen[2*(i-1)+1] = it->second.size();
    
    vField_DataVal.push_back(data_val);
  }

  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HMSET")) {
    return false;
  }
  if (false == CheckStatusOK(reply, rc, "HMSET")) {
    return false;
  }
  ReleaseConn(rc, false);
  FreeReplyObj(reply);
  return true;
}

bool FS_RedisInterface::HMGet(const std::string& _key, 
                              std::map<std::string, std::string>& _val) {
  if (_key.empty() || _val.empty()) {
    return false;
  }
  std::vector<std::string> v_keys;
  std::stringstream ss;
  ss << "HMGET " << _key << " ";
  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  
  for (auto it = _val.begin(); it != _val.end(); ++it) {
    v_keys.push_back(it->first);
    ss << it->first << " ";
  }

  redisReply *reply = NULL;
  redisContext* rc = NULL;
  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: HMGET";
    return false;
  }

  //CmdToString(ss.str());
  size_t args_size = _val.size() + 2;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HMGET";
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();
  size_t i = 2;
  std::vector<std::shared_ptr<std::string> > vFields;
  for (auto it = _val.begin(); it != _val.end(); ++it, ++i) 
  {
    std::shared_ptr<std::string> field = std::make_shared<std::string>(it->first);
    argv[i] = field->c_str();
    arglen[i] = it->first.size();
    
    vFields.push_back(field);
  }

  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*)redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HMGET")) {
    return false;
  }
  if (false == CheckRetNil(reply, rc, "HMGET")) {
    return false;
  }
  if (false == CheckRetArr(reply, rc, "HMGET")) {
    return false;
  }
  //
  for (size_t index = 0; index < reply->elements; ++index) {
    std::string ret_val;
    const std::string& key_index = v_keys[index];
    if (false == GetOneStringFromArr(ret_val, reply->element[index])) {
      _val[key_index] = ""; //if is emtpy, it may return true.. TODO
      continue;
    }
    _val[key_index] = ret_val;
  }

  FreeReplyObj(reply);
  ReleaseConn(rc);
  return true;
}

bool FS_RedisInterface::HGetAll(const std::string& _key,
                                std::map<std::string, std::string>& _val) {
  if (_key.empty()) {
    return false;
  }
  _val.clear();
  std::stringstream ss;
  ss << "HGETALL " << _key;

  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  redisReply *reply = NULL;
  redisContext* rc = NULL;
  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: HGETALL";
    return false;
  }
  

  size_t args_size = 2;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HGETALL";
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();

  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HGETALL")) {
    return false;
  }
  if (false == CheckRetNil(reply, rc, "HGETALL")) {
    return false;
  }
  if (false == CheckRetArr(reply, rc, "HGETALL")) {
    return false;
  }

  std::vector<std::string> key_v;
  std::vector<std::string> val_v;
  for (size_t index = 0; index < reply->elements/2; ++index) {
    std::string key_index;
    if (false == GetOneStringFromArr(key_index, reply->element[index*2])) {
      continue;
    }
    key_v.push_back(key_index);

    std::string val_str;
    if (false == GetOneStringFromArr(val_str, reply->element[index*2+1])) {
      val_v.push_back("");
      continue;
    }
    val_v.push_back(val_str);
  }

  unsigned index_key = 0, index_val = 0;
  for (; index_key < key_v.size() && index_val < val_v.size(); ++index_key,
       ++index_val) {
    const std::string& key_val  = key_v[index_key];
    const std::string& val_val = val_v[index_val];
    if (val_val.empty()) {
      continue;
    }
    _val[key_val] = val_val;
  }

  FreeReplyObj(reply);
  ReleaseConn(rc);
  return true;
}

bool FS_RedisInterface::HVals(const std::string& _key, 
                              std::vector<std::string>& _val) {
  if (_key.empty()) {
    return false;
  }
  _val.clear();

  std::stringstream ss;
  ss << "HVALS  " << _key;

  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);
  redisReply *reply = NULL;
  redisContext* rc = NULL;

  rc = Grub();
  if (NULL == rc) {
    //LOG(ERROR) << "get redis obj failed for cmd: HVALS";
    return false;
  }

  size_t args_size = 2;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HVALS";  
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();
  //
  //CmdToString(ss.str());
  std::string out_str;
  unsigned ilen= 0;
  for (auto it = argv.begin(); it != argv.end(); ++it, ++ilen) {
    out_str.append(*it, arglen[ilen]);
    out_str += " ";
  }
  //VLOG(100) << "cmd: " << out_str;
  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HVALS")) {
    return false;
  }

  if (false == CheckRetNil(reply, rc, "HVALS")) {
    return false;
  }
  if (false == CheckRetArr(reply, rc, "HVALS")) {
    return false;
  }
  for (size_t index = 0; index < reply->elements; ++index) {
    std::string ret_val;
    if (false == GetOneStringFromArr(ret_val, reply->element[index])) {
      break;
    }
    _val.push_back(ret_val);
  }

  FreeReplyObj(reply);
  ReleaseConn(rc);
  return true;
}

bool FS_RedisInterface::HDel(const std::string& _key, 
                             const std::vector<std::string>& _fields) {
  if (_key.empty() || _fields.empty()) {
    return false;
  }
  
  std::stringstream ss;
  ss << "HDEL " << _key << "  ";

  std::shared_ptr<std::string> cmd_key_str = std::make_shared<std::string>(
      _key);

  for (auto it = _fields.begin(); it != _fields.end(); ++it) {
    ss << *it << "  ";
  }

  redisReply *reply = NULL;
  redisContext* rc = NULL;

  rc = Grub();
  if (rc == NULL) {
    //LOG(ERROR) << "get redis obj failed for cmd: HDEL";
    return false;
  }

  //CmdToString(ss.str());
  size_t args_size = _fields.size() + 2;
  std::vector<const char*> argv(args_size);
  std::vector<size_t> arglen(args_size); 
  argv[0] = "HDEL";
  arglen[0] = strlen(argv[0]);
  argv[1] = cmd_key_str->c_str();
  arglen[1] = cmd_key_str->size();
  size_t i = 2;
  std::vector<std::shared_ptr<std::string> > vFields;
  for (auto it = _fields.begin(); it != _fields.end(); ++it, ++i) {
    std::shared_ptr<std::string> field = std::make_shared<std::string>(*it);
    argv[i] = it->c_str();
    arglen[i] = it->size();

    vFields.push_back(field);
  }

  reply = (redisReply*) redisCommandArgv(rc, args_size, &(argv[0]), &(arglen[0]));
  if (false == CheckReplySucc(reply, rc, "HDEL")) {
    return false;
  }
  if (false == CheckRetInt(reply, rc, "HDEL")) {
    return false;
  }
  
  FreeReplyObj(reply);
  ReleaseConn(rc);
  return true;
}

void FS_RedisInterface::CmdToString(const std::string& _cmd_str) {
  if (_cmd_str.empty()) {
    return ;
  }
  //VLOG(100) << "redis cmd: " << _cmd_str;
}
//
