#include "lib_redis.h"

namespace LIB_REDIS {

int32_t Client::Init(const std::string& host, int32_t port,
             const std::string& pwd,  int32_t db)
{
  host_   = host;
  port_   = port;
  pwd_    = pwd;
  db_     = db;
  return 0;
}

int32_t Client::Command(const std::vector<std::string> &send, Reply &reply)
{
  int32_t     ret         = 0;
  redisReply  *c_reply    = NULL;
  if (NULL == rc_) {
    Connect();
  }

  if (0 != Ping()) { //reconnect again.
    if (0 != Connect()) {
      Close();
      return -1;
    }
  }

  std::vector<const char*> argv;
  std::vector<size_t> argvlen;
  argv.reserve(send.size());
  argvlen.reserve(send.size());

  for(std::vector<std::string>::const_iterator it = send.begin();
      it != send.end();
      ++it) {
    argv.push_back(it->c_str());
    argvlen.push_back(it->size());
  }

  c_reply = (redisReply *)redisCommandArgv(rc_, static_cast<int>(send.size()), argv.data(), argvlen.data());
  if (c_reply == NULL) {
    ret = rc_->err;
    errmsg_.clear();
    errmsg_.append("Command:");
    errmsg_.append("[").append(ErrorToStr(ret)).append("]");
    errmsg_.append(std::string(rc_->errstr));
    return ret;
  }

  reply = Reply(c_reply);
  freeReplyObject(c_reply);

  return ret;
}

const std::string& Client::ErrMsg()
{
  return errmsg_;
}

void Client::SetHost(const std::string& host)
{
  host_ = host;
  return;
}

void Client::SetPort(int32_t port)
{
  port_ = port;
  return;
}

void Client::SetPass(const std::string& pass)
{
  pwd_ = pass;
  return;
}

void Client::SetDB(int32_t db)
{
  db_ = db;
  return;
}

int32_t Client::GetDB()
{
  return db_;
}

int32_t Client::SetTimeout(int64_t connect_timeout, int64_t snd_rcv_timeout)
{
  connect_timeout_ = connect_timeout;
  snd_rcv_timeout_ = snd_rcv_timeout;

  return SetRedisContextTimeout();
}

void Client::GetTimeout(int64_t& connect_timeout, int64_t& snd_rcv_timeout)
{
  connect_timeout = connect_timeout_;
  snd_rcv_timeout = snd_rcv_timeout_;
}

const std::string& Client::ErrorToStr(int32_t errorno)
{
  static std::string redis_err_io         = "REDIS_ERR_IO";
  static std::string redis_err_eof        = "REDIS_ERR_EOF";
  static std::string redis_err_protocol   = "REDIS_ERR_PROTOCOL";
  static std::string redis_err_oom        = "REDIS_ERR_OOM";
  static std::string redis_err_other      = "REDIS_ERR_OTHER";

  if (errorno == REDIS_ERR_IO) {
    return redis_err_io;
  }
  else if (errorno == REDIS_ERR_EOF) {
    return redis_err_eof;
  }
  else if (errorno == REDIS_ERR_PROTOCOL) {
    return redis_err_protocol;
  }
  else if (errorno == REDIS_ERR_OOM) {
    return redis_err_oom;
  }
  else {
    return redis_err_other;
  }
}

int32_t Client::Ping()
{
  if (unlikely(NULL == rc_))
    return -1;

  ReplyHandle reply_handle((redisReply*)redisCommand(rc_, "ping"));
  if (unlikely(NULL == reply_handle.Reply())) {
    return -1;
  }
  if (unlikely(ERROR == reply_handle.Reply()->type)) {
    errmsg_ = rc_->errstr;
    return -2;
  }
  if (unlikely(std::string("PONG") != std::string(reply_handle.Reply()->str) &&
               std::string("pong") != std::string(reply_handle.Reply()->str))) {
    errmsg_ = rc_->errstr;
    return -3;
  }

  return 0;
}

int32_t Client::Auth()
{
  if (unlikely(NULL == rc_))
    return -1;
  if (unlikely(pwd_.empty()))
    return 0;

  ReplyHandle reply_handle(((redisReply*)redisCommand(rc_, "auth %s", pwd_.c_str())));
  if (unlikely(NULL == reply_handle.Reply())) {
    return -1;
  }
  if (unlikely(ERROR == reply_handle.Reply()->type)) {
    errmsg_ = rc_->errstr;
    return -2;
  }
  if (unlikely(std::string("OK") != std::string(reply_handle.Reply()->str) &&
               std::string("ok") != std::string(reply_handle.Reply()->str))) {
    errmsg_ = rc_->errstr;
    return -3;
  }

  return 0;
}

int32_t Client::Select()
{
  if (unlikely(NULL == rc_))
    return -1;
  ReplyHandle reply_handle((redisReply*)redisCommand(rc_, "select %d", db_));
  if (unlikely(NULL == reply_handle.Reply())) {
    return -1;
  }
  if (unlikely(ERROR == reply_handle.Reply()->type)) {
    errmsg_ = rc_->errstr;
    return -2;
  }
  if (unlikely(std::string("OK") != std::string(reply_handle.Reply()->str) &&
               std::string("ok") != std::string(reply_handle.Reply()->str))) {
    errmsg_ = rc_->errstr;
    return -3;
  }

  return 0;
}

int32_t Client::ConnectImpl()
{
  struct timeval tv_conn;
  tv_conn.tv_sec  = connect_timeout_/1000;        //ms convert to second
  tv_conn.tv_usec = 1000*(connect_timeout_%1000); //ms convert to microsecond

  rc_ = redisConnectWithTimeout(host_.c_str(), port_, tv_conn);
  if (unlikely(rc_ == NULL || rc_->err != REDIS_OK)) {
    errmsg_ = rc_->errstr;
    return -1;
  }

  redisEnableKeepAlive(rc_);//ignore fail

  return 0;
}

int32_t Client::SetRedisContextTimeout()
{
  if (unlikely(NULL == rc_))
    return -1;
  struct timeval tv_rw;
  tv_rw.tv_sec  = snd_rcv_timeout_/1000;          //ms convert to second
  tv_rw.tv_usec = 1000*(snd_rcv_timeout_%1000);   //ms convert to microsecond
  if (unlikely(0 != redisSetTimeout(rc_, tv_rw))) {
    errmsg_ = rc_->errstr;
    return -1;
  }

  return 0;
}

int32_t Client::Connect()
{
  Close();

  if (unlikely(0 != ConnectImpl()))
    return -1;

  if (unlikely(0 != SetRedisContextTimeout() ||
               /***0 != Auth() || **/
               0 != Select())) {
    Close();
    return -2;
  }

  return 0;
}

void Client::Close()
{
  if (rc_ != NULL) {
    redisFree(rc_);
    rc_ = NULL;
  }
}
/////
const std::string& ServiceBase::Errmsg()
{
  return errmsg_;
}

void ServiceBase::RecordErrmsg(const std::string& errmsg)
{
  errmsg_ = errmsg;
}

bool ServiceBase::SetDB(int32_t db) {
  old_db_ = client_.GetDB();
  client_.SetDB(db);
  return SelectDB(db);
}

bool ServiceBase::ResetDB() {
  if (old_db_ != -1) {
    client_.SetDB(old_db_);
    return SelectDB(old_db_);
  }

  return true;
}

bool ServiceBase::SelectDB(int32_t db)
{
  std::vector<std::string> send;
  send.push_back("select");
  send.push_back(std::to_string(static_cast<long long>(db)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}

//////
bool RedisHash::hGet(const std::string& hash, const std::string& field, std::string& value)
{
  std::vector<std::string> send;
  send.push_back("hget");
  send.push_back(hash);
  send.push_back(field);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  value = reply.Str();
  return true;
}

bool RedisHash::hSet(const std::string& hash, const std::string& field, const std::string& value)
{
  std::vector<std::string> send;
  send.push_back("hset");
  send.push_back(hash);
  send.push_back(field);
  send.push_back(value);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

bool RedisHash::hDel(const std::string& hash, const std::string& field)
{
  std::vector<std::string> send;
  send.push_back("hdel");
  send.push_back(hash);
  send.push_back(field);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if ( reply.Integer() == 1 ) {
    return true;
  }

  RecordErrmsg("member not exist");
  return false;
}

bool RedisHash::hKeys(const std::string& hash, std::vector<std::string>& item_vec)
{
  std::vector<std::string> send;
  send.push_back("hkeys");
  send.push_back(hash);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  item_vec.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i != element_vec.size(); ++i) {
    item_vec.push_back(element_vec[i].Str());
  }
  return true;
}

bool RedisHash::hGetAll(const std::string& hash, std::vector<std::string>& item_vec)
{
  std::vector<std::string> send;
  send.push_back("hgetall");
  send.push_back(hash);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  item_vec.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i != element_vec.size(); ++i) {
    item_vec.push_back(element_vec[i].Str());
  }

  return true;
}

bool RedisHash::hMGet(const std::string& hash, std::vector<std::string>& vec_keys, std::map<std::string, std::string>& map_values)
{
  std::vector<std::string> send;
  send.push_back("hmget");
  send.push_back(hash);
  for (std::vector<std::string>::iterator iter = vec_keys.begin(); iter != vec_keys.end(); ++iter)
  {
    send.push_back(*iter);
  }

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  const std::vector<Reply>& element_vec = reply.Elements();
  if (element_vec.size() != vec_keys.size())
  {
    RecordErrmsg("result size is not equal keys size!");
    return false;
  }
  for (size_t i = 0, len = element_vec.size(); i < len; ++i) {
    map_values.insert(make_pair(vec_keys[i], element_vec[i].Str()));
  }

  return true;
}

////////////
bool RedisList::lPop(const std::string& list, std::string& value)
{
  return Pop(list, "lpop", value);
}

bool RedisList::rPop(const std::string& list, std::string& value)
{
  return Pop(list, "rpop", value);
}

bool RedisList::lPush(const std::string& list, const std::string& value)
{
  return PushImpl(list, "lpush", value);
}

bool RedisList::RPopLPush(const std::string& list_src, const std::string& list_dst, std::string& value)
{
  std::vector<std::string> send;
  send.push_back("RPopLPush");
  send.push_back(list_src);
  send.push_back(list_dst);

  std::string key;
  return PopImpl(send, key, value);
}

bool RedisList::LRange(const std::string& key, int32_t start, int32_t stop, std::vector<std::string>& item_vec)
{
  std::vector<std::string> send;
  send.push_back("LRange");
  send.push_back(key);
  send.push_back(std::to_string(static_cast<long long>(start)));
  send.push_back(std::to_string(static_cast<long long>(stop)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  item_vec.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i != element_vec.size(); ++i) {
    item_vec.push_back(element_vec[i].Str());
  }
  return true;
}

bool RedisList::LTrim(const std::string& key, int32_t start, int32_t stop)
{
  std::vector<std::string> send;
  send.push_back("LTrim");
  send.push_back(key);
  send.push_back(std::to_string(static_cast<long long>(start)));
  send.push_back(std::to_string(static_cast<long long>(stop)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  if (reply.Type() != STATUS || reply.Str() != "OK") {
    RecordErrmsg("STATUS [OK] result expected");
    return false;
  }
  return true;
}

bool RedisList::rPush(const std::string& list, const std::string& value)
{
  return PushImpl(list, "rpush", value);
}

bool RedisList::lLen(const std::string& list, int64_t& len)
{
  std::vector<std::string> send;
  send.push_back("llen");
  send.push_back(list);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() != INTEGER) {
    RecordErrmsg(reply.Str());
    return false;
  }

  len = (int64_t)reply.Integer();
  return true;
}

bool RedisList::Remove(const std::string& list, const std::string& item)
{
  std::vector<std::string> send;
  send.push_back("LREM");
  send.push_back(list);
  send.push_back("-1");    //remove 1 from tail to head
  send.push_back(item);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() != INTEGER) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if (0 == (int64_t)reply.Integer()) {//nothing removed
    RecordErrmsg("item not found");
  }
  return true;
}

bool RedisList::BRPop(const std::string& list, std::string& value, int64_t second_wait)//0 for block indefinitely
{
  return BPop(list, "BRPOP", value, second_wait);
}

bool RedisList::BLPop(const std::string& list, std::string& value, int64_t second_wait)//0 for block indefinitely
{
  return BPop(list, "BLPOP", value, second_wait);
}

bool RedisList::BRPopLPush(const std::string& list_src, const std::string& list_dst, std::string& value, int64_t second_wait)//0 for block indefinitely
{
  std::vector<std::string> send;
  send.push_back("BRPopLPush");
  send.push_back(list_src);
  send.push_back(list_dst);
  send.push_back(std::to_string(static_cast<long long>(second_wait)));

  std::string key;
  return PopImpl(send, key, value);
}

bool RedisList::BPop(const std::string& list, const std::string& cmd, std::string& value, int64_t second_wait)
{
  std::vector<std::string> send;
  send.push_back(cmd);
  send.push_back(list);
  send.push_back(std::to_string(static_cast<long long>(second_wait)));

  std::string key;//ignored for single list pop
  return PopImpl(send, key, value);
}

bool RedisList::Pop(const std::string& list, const std::string& cmd, std::string& value)
{
  std::vector<std::string> send;
  send.push_back(cmd);
  send.push_back(list);

  std::string key;//ignored for single list pop
  return PopImpl(send, key, value);
}

bool RedisList::PopImpl(const std::vector<std::string>& send, std::string& key, std::string& value)
{
  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if (reply.Type() == STRING) {
    value = reply.Str();
    return true;
  }

  if (reply.Type() == NIL) {//list is empty or timeout is reached
    key = value = "";
    return true;
  }

  if (reply.Type() == ARRAY) {
    const std::vector<Reply>& reply_vec = reply.Elements();
    if (reply_vec.size() != 2) {
      RecordErrmsg("reply_vec_size is less than 2");
      return false;
    }
    key     = reply_vec[0].Str();
    value   = reply_vec[1].Str();
    return true;
  }

  RecordErrmsg("wrong reply type");
  return false;
}

bool RedisList::PushImpl(const std::string& list, const std::string& cmd, const std::string& value)
{
  std::vector<std::string> send;
  send.push_back(cmd);
  send.push_back(list);
  send.push_back(value);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}
///////////////

bool RedisSets::Smembers(const std::string& sKey, std::vector<std::string>& members) {
  std::vector<std::string> send;
  send.push_back("SMEMBERS");
  send.push_back(sKey);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0, len = element_vec.size(); i < len; ++i) {
    members.push_back(element_vec[i].Str());
  }
  return true;
}

bool RedisSets::Sadd(const std::string& sets, const std::string& value)
{
  std::vector<std::string> send;
  send.push_back("sadd");
  send.push_back(sets);
  send.push_back(value);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}

//todo: test
bool RedisSets::Sadd(const std::string& sets, const std::vector<std::string>& valuelist) {
  if (sets.empty() || valuelist.empty()) {
    return false;
  }
  
  std::vector<std::string> send;
  send.push_back("sadd");
  send.push_back(sets);
  send.insert(send.end(), valuelist.begin(), valuelist.end());

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}

bool RedisSets::Spop(const std::string& sets, std::string& value)
{
  std::vector<std::string> send;
  send.push_back("spop");
  send.push_back(sets);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  value = reply.Str();

  return true;
}

//todo:  test
bool RedisSets::Srem(const std::string& sets, std::vector<std::string>& members) {
  if (members.empty() || sets.empty()) {
    return true;
  }
  
  std::vector<std::string> send;
  send.push_back("srem");
  send.push_back(sets);
  send.insert(send.end(), members.begin(), members.end());

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

////////////
void RedisKey::SetNamespace(const std::string& key_prefix) {
  namespace_ = key_prefix; 
}

std::string RedisKey::GetNamespace() const {
  return namespace_; 
}

bool RedisKey::Del(const std::string& key)
{
  std::vector<std::string> send;
  send.push_back("del");
  send.push_back(GetRealKey(key));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

bool RedisKey::Exists(const std::string& key) {
  std::vector<std::string> send;
  send.push_back("EXISTS");
  send.push_back(GetRealKey(key));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

bool RedisKey::Keys(const std::string& sKey, std::vector<std::string>& sKeyRet) {
  std::vector<std::string> send;
  send.push_back("KEYS");
  send.push_back(sKey);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0, len = element_vec.size(); i < len; ++i) {
    sKeyRet.push_back(element_vec[i].Str());
  }
  return true;
}

std::string RedisKey::Get(const std::string& key)
{
  std::string result;
  Get(key, result);
  return result;
}

bool RedisKey::Get(const std::string& key, std::string& value)
{
  std::vector<std::string> send;
  send.push_back("get");
  send.push_back(GetRealKey(key));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  value = reply.Str();
  return true;
}

bool RedisKey::TimeToLive(const std::string& key, int64_t& val)
{
  std::vector<std::string> send;
  send.push_back("TTL");
  send.push_back(GetRealKey(key));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() != INTEGER) {
    RecordErrmsg(reply.Str());
    return false;
  }

  val = reply.Integer();
  return true;
}

bool RedisKey::Set(const std::string& key, const std::string& value)
{
  std::vector<std::string> send;
  send.push_back("set");
  send.push_back(GetRealKey(key));
  send.push_back(value);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

bool RedisKey::Set(const std::string& key, const std::string& value, int64_t second)
{
  std::vector<std::string> send;
  send.push_back("set");
  send.push_back(GetRealKey(key));
  send.push_back(value);
  send.push_back("ex");
  send.push_back(std::to_string(static_cast<long long>(second)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

int32_t RedisKey::Setnx(const std::string& key, const std::string& value, int64_t second)
{
  std::vector<std::string> send;
  send.push_back("set");
  send.push_back(GetRealKey(key));
  send.push_back(value);
  send.push_back("nx");
  send.push_back("ex");
  send.push_back(std::to_string(static_cast<long long>(second)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return -1;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return -1;
  }

  if (NIL == reply.Type())
  {
    return 1;
  }

  return 0;
}

bool RedisKey::Setex(const std::string& key, const std::string& value, const std::string& second)
{
  std::vector<std::string> send;
  send.push_back("setex");
  send.push_back(GetRealKey(key));
  send.push_back(second);
  send.push_back(value);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  return true;
}

bool RedisKey::Expire(const std::string& key, const std::string& second)
{
  std::vector<std::string> send;
  send.push_back("expire");
  send.push_back(GetRealKey(key));
  send.push_back(second);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  int64_t val = reply.Integer();
  if (val == 0) {
    RecordErrmsg("expire reply return 0");
    return false;
  }

  return true;
}

bool RedisKey::Setnx(const std::string& key, const std::string& val)
{
  std::vector<std::string> send;
  send.push_back("setnx");
  send.push_back(GetRealKey(key));
  send.push_back(val);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  int64_t result = reply.Integer();
  if (result != 1) {
    RecordErrmsg("setnx return 0");
    return false;
  }

  return true;
}

bool RedisKey::Incrby(const std::string& key, const std::string& val, int64_t& result)
{
  std::vector<std::string> send;
  send.push_back("incrby");
  send.push_back(GetRealKey(key));
  send.push_back(val);
  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  if (reply.Type() != INTEGER) {
    RecordErrmsg(reply.Str());
    return false;
  }
  result = reply.Integer();
  return true;
}

bool RedisKey::Expireat(const std::string& key, const std::string time_stamp)
{
  std::vector<std::string> send;
  send.push_back("expireat");
  send.push_back(GetRealKey(key));
  send.push_back(time_stamp);
  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  if (reply.Type() != INTEGER) {
    RecordErrmsg(reply.Str());
    return false;
  }
  if (reply.Integer() == 0) {
    RecordErrmsg("expireat return 0, key does not exist or the timeout could not be set");
    return false;
  }
  return true;
}

bool RedisKey::Incr(const std::string& key, int64_t& val)
{
  return IncrBy(key, 1, val);
}

bool RedisKey::IncrBy(const std::string& key, int64_t delta, int64_t& val)
{
  std::vector<std::string> send;
  send.push_back("INCRBY");
  send.push_back(GetRealKey(key));
  send.push_back(std::to_string(static_cast<long long>(delta)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() != INTEGER) {
    RecordErrmsg(reply.Str());
    return false;
  }

  val = reply.Integer();
  return true;
}

bool RedisKey::Decr(const std::string& key, int64_t& val)
{
  return DecrBy(key, 1, val);
}

bool RedisKey::DecrBy(const std::string& key, int64_t delta, int64_t& val)
{
  std::vector<std::string> send;
  send.push_back("DECRBY");
  send.push_back(GetRealKey(key));
  send.push_back(std::to_string(static_cast<long long>(delta)));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == INTEGER) {
    val = reply.Integer();
    return true;
  }

  if (reply.Type() == STATUS && "QUEUED" == reply.Str()) {//transaction support
    return true;
  }

  RecordErrmsg(reply.Str());
  return false;
}

std::string RedisKey::GetRealKey(const std::string& key) {
  if (namespace_.empty())
    return key;

  return namespace_ + key;
}

//todo test
bool RedisKey::Mset(const std::map<std::string,std::string>& mpKV) {
  std::vector<std::string> send;
  send.push_back("MSET");
  
  for (std::map<std::string, std::string>::const_iterator it = mpKV.begin(); 
       it != mpKV.end(); ++it) {
    if (it->first.empty() || it->second.empty()) {
      return false;
    }
    send.push_back(it->first);
    send.push_back(it->second);
  }

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  return true;
}

///////////
bool RedisWatch::Watch(const std::string& key) {
  std::vector<std::string> key_vec;
  key_vec.push_back(key);
  return Watch(key_vec);
}
bool RedisWatch::Watch(const std::vector<std::string>& key_vec) {
  std::vector<std::string> send(1,"WATCH");

  for (size_t i = 0; i != key_vec.size(); ++i) {
    send.push_back(key_vec[i]);
  }

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}
bool RedisWatch::UnWatch() {
  std::vector<std::string> send;
  send.push_back("UNWATCH");

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}

//////////////  
bool RedisTransaction::Start() {
  if (in_trans_)
    return false;
  Reply reply;
  return (in_trans_ = ExecCmd("MULTI", reply));
}

bool RedisTransaction::Commit() {
  if (!in_trans_)
    return false;

  in_trans_ = false;

  Reply reply;
  return ExecCmd("EXEC", reply) && NIL != reply.Type();
}

bool RedisTransaction::Rollback() {
  if (!in_trans_)
    return false;

  in_trans_ = false;

  Reply reply;
  return ExecCmd("DISCARD", reply);
}

bool RedisTransaction::ExecCmd(const std::string& cmd,Reply& reply) {
  std::vector<std::string> send;
  send.push_back(cmd);

  if (REDIS_OK != client_.Command(send, reply)) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }
  return true;
}

//////////////////
int32_t RedisPublisher::Publish(const std::string& channel, const std::string& msg) {//return the num of the subscriber that receives the message
  std::vector<std::string> cmd_vec;
  cmd_vec.push_back("PUBLISH");
  cmd_vec.push_back(channel);
  cmd_vec.push_back(msg);

  Reply reply;
  int32_t ret = client_.Command(cmd_vec, reply);
  if (REDIS_OK != ret) {
    return -1;
  }
  if (INTEGER != reply.Type()) {
    return -1;
  }
  return reply.Integer();
}

////////
bool RedisLock::TryLock(const std::string& lock_info)
{
  if (have_locked_) {
    return true;
  }
  have_locked_ = client_.Setnx(lock_name_, lock_info);
  return have_locked_;
}
bool RedisLock::Unlock()
{
  if (!have_locked_) {
    return false;
  }
  have_locked_ = !client_.Del(lock_name_);
  return !have_locked_;
}
bool RedisLock::ForceUnlock()
{
  if (have_locked_) {
    return Unlock();
  }
  return client_.Del(lock_name_);
}
bool RedisLock::GetLockInfo(std::string& lock_info)
{
  return client_.Get(lock_name_, lock_info);
}

//////////////////
bool RedisScopedLock::TryLock(const std::string& lock_info)
{
  return lock_.TryLock(lock_info);
}
bool RedisScopedLock::Unlock()
{
  return lock_.Unlock();
}
bool RedisScopedLock::GetLockInfo(std::string& lock_info)
{
  return lock_.GetLockInfo(lock_info);
}

/////////////////////
bool RedisZset::Zadd(const std::string& key, const std::string& member, const int64_t& score)
{
  std::vector<std::string> send;
  send.push_back("ZADD");
  send.push_back(key);
  send.push_back(LibString::type2str(score) );
  send.push_back(member);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}

bool RedisZset::Zrem(const std::string& key, const std::string& member)
{
  std::vector<std::string> send;
  send.push_back("ZREM");
  send.push_back(key);
  send.push_back(member);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if ( reply.Integer() == 1 ) {
    return true;
  }
  RecordErrmsg("member not exist");
  return false;
}

bool RedisZset::Zincrby(const std::string& key, const std::string& member, const int64_t& score)
{
  std::vector<std::string> send;
  send.push_back("ZINCRBY");
  send.push_back(key);
  send.push_back(LibString::type2str(score));
  send.push_back(member);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  return true;
}

bool RedisZset::ZincrbyV1(const std::string& key, const std::string& member, const int64_t& score, int64_t& new_score )
{
  std::vector<std::string> send;
  send.push_back("ZINCRBY");
  send.push_back(key);
  send.push_back(LibString::type2str(score));
  send.push_back(member);

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  new_score = strtoll(reply.Str().c_str(), NULL, 10);

  return true;
}

bool RedisZset::Zrange(const std::string& key, const int32_t start, const int32_t end, 
                       std::vector<std::string>& members, std::vector<std::string>& scores)
{
  std::vector<std::string> send;
  send.push_back("ZRANGE");
  send.push_back(key);
  send.push_back(LibString::type2str(start));
  send.push_back(LibString::type2str(end));
  send.push_back("WITHSCORES");

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  members.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i < element_vec.size(); i += 2) {
    members.push_back(element_vec[i].Str());
    scores.push_back(element_vec[i + 1].Str());
  }

  return true;
}

bool RedisZset::ZrangeByScore(const std::string& key, const int64_t& start, const int64_t& end, std::vector<std::string>& members, uint32_t offset, uint32_t count)
{
  std::vector<std::string> send;
  send.push_back("ZRANGEBYSCORE");
  send.push_back(key);
  send.push_back(LibString::type2str(start));
  send.push_back(LibString::type2str(end));

  if ( count > 0 )
  {
    send.push_back("LIMIT");
    send.push_back(LibString::type2str(offset));
    send.push_back(LibString::type2str(count));
  }

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  members.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i != element_vec.size(); ++i) {
    members.push_back(element_vec[i].Str());
  }

  return true;
}

bool RedisZset::ZrangeByScoreV1(const std::string& key, const int64_t& start, const int64_t& end, 
                                std::vector<std::string>& members, std::vector<std::string>& scores, 
                                uint32_t offset , uint32_t count )
{
  std::vector<std::string> send;
  send.push_back("ZRANGEBYSCORE");
  send.push_back(key);
  send.push_back(LibString::type2str(start));
  send.push_back(LibString::type2str(end));

  if ( count > 0 )
  {
    send.push_back("LIMIT");
    send.push_back(LibString::type2str(offset));
    send.push_back(LibString::type2str(count));
  }

  send.push_back("WITHSCORES");

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }

  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  members.clear();
  scores.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i < element_vec.size(); i += 2) {
    members.push_back(element_vec[i].Str());
    scores.push_back(element_vec[i + 1].Str());
  }

  return true;
}

bool RedisZset::ZrevrangeByScore(const std::string& key, const int64_t& start, const int64_t& end, std::vector<std::string>& members)
{
  std::vector<std::string> send;
  send.push_back("ZREVRANGEBYSCORE");
  send.push_back(key);
  send.push_back(LibString::type2str(start));
  send.push_back(LibString::type2str(end));

  Reply reply;
  int32_t ret = client_.Command(send, reply);
  if (ret != REDIS_OK) {
    RecordErrmsg(client_.ErrMsg());
    return false;
  }

  if (reply.Type() == ERROR) {
    RecordErrmsg(reply.Str());
    return false;
  }


  if (reply.Type() != ARRAY) {
    RecordErrmsg("array result expected");
    return false;
  }

  members.clear();
  const std::vector<Reply>& element_vec = reply.Elements();
  for (size_t i = 0; i != element_vec.size(); ++i) {
    members.push_back(element_vec[i].Str());
  }

  return true;
}

/////////////
//using libevent as net event proc
void AsyncClient::InitEvent() {
  if (NULL == event_) {
    event_ = event_base_new();
  }
}
void AsyncClient::FreeEvent() {
  if (NULL == event_)
    return;

  DetachEvent();

  if (m_InitEvent == false) {
    event_base_free((struct event_base *)event_);
  }
  event_ = NULL;
}
bool AsyncClient::AttachEvent() {
  if (NULL == event_)
    return false;
  std::cout << "attch new event " << std::endl;
  return REDIS_OK == redisLibeventAttach(rc_, (struct event_base *)event_);
}
void AsyncClient::DetachEvent() {
  redisLibeventDetach(rc_);
}
void AsyncClient::LoopEvent() {
  event_base_dispatch((struct event_base *)event_);
}

int32_t AsyncClient::Init(const std::string& host, int32_t port,
                          const std::string& pwd,  int32_t db)
{
  host_   = host;
  port_   = port;
  pwd_    = pwd;
  db_     = db;

  rc_     = NULL;
  m_IsConn = 0;
  return 0;
}

int32_t AsyncClient::Init(const std::string& host, int32_t port,
                          const std::string& pwd,  int32_t db, void *even_base)
{
  host_   = host;
  port_   = port;
  pwd_    = pwd;
  db_     = db;

  rc_     = NULL;
  event_ = even_base;
  m_IsConn = 0;
  m_InitEvent = true;
  return 0;
}

bool AsyncClient::AsyncCommand(const std::vector<std::string>& cmd_vec, AsyncHandler async_handler) {//async_handler should have a long life period
#ifdef REDIS_DEBUG
  printf("AsyncCommand\n");
#endif
  InitEvent();
  if (NULL == rc_) {
    std::cout << "is begin, first to connnect" << std::endl;
    if (AsyncConnect() < 0) {
      std::cout << "connect not succ " << std::endl;
      return false;
    }
  }
  return AsyncCommandImpl(cmd_vec, async_handler);
}

const char* AsyncClient::Errmsg() {
  return errmsg_.c_str();
}
void AsyncClient::SetErrmsg(const std::string& msg) {
  if (!msg.empty()) {
    errmsg_.append(msg);
  }
}
void AsyncClient::ClearErrMsg() {
  errmsg_.clear();
}

bool AsyncClient::AsyncCommandImpl(const std::vector<std::string>& cmd_vec, AsyncHandler async_handler) {
#ifdef REDIS_DEBUG
  printf("AsyncCommandImpl\t");
  for (size_t i = 0; i != cmd_vec.size(); ++i) {
    printf("%s ", cmd_vec[i].c_str());
  }
  printf("\n");
#endif
  std::vector<const char*> argv;
  std::vector<size_t> argvlen;
  argv.reserve(cmd_vec.size());
  argvlen.reserve(cmd_vec.size());

  for(size_t i = 0; i != cmd_vec.size(); ++i) {
    argv.push_back(cmd_vec[i].c_str());
    argvlen.push_back(cmd_vec[i].size());
  }

  AttachEvent();

  int32_t result = redisAsyncCommandArgv(rc_, &AsyncClient::OnMessage,
                                         &async_handler, static_cast<int>(cmd_vec.size()),
                                         argv.data(), argvlen.data());
  if (REDIS_ERR == result) {
    errmsg_.assign(rc_->errstr);
    return false;
  }
  LoopEvent();

  return true;
}

void AsyncClient::OnMessage(struct redisAsyncContext *ac, void *reply_data, void *privdata) {
#ifdef REDIS_DEBUG
  printf("OnMessage\n");
#endif
  if (NULL == reply_data) {
#ifdef REDIS_DEBUG
    printf("NULL Reply\n");
#endif
    return;
  }

  if (NULL == privdata)
    return;

  Reply reply((redisReply *)reply_data);
  AsyncHandler& async_handler = *(AsyncHandler *)privdata;
  async_handler(reply);
}

int32_t AsyncClient::OnAuth(Reply& reply) {
#ifdef REDIS_DEBUG
  printf("OnAuth\n");
  reply.Print();
#endif
  DetachEvent();
  event_base_loopbreak((struct event_base *)event_);
  if (reply.Type() == ERROR && !pwd_.empty()) {
    errmsg_.assign("Auth failed").append(rc_->errstr);
    return -1;
  }
  return REDIS_OK;
}

int32_t AsyncClient::OnSelect(Reply& reply) {
#ifdef REDIS_DEBUG
  printf("OnSelect\n");
  reply.Print();
#endif
  DetachEvent();
  event_base_loopbreak((struct event_base *)event_);
  if (reply.Type() == ERROR) {
    errmsg_.assign("Select failed").append(rc_->errstr);
    return -1;
  }
  return REDIS_OK;
}

int32_t AsyncClient::OnPing(Reply& reply) {
#ifdef REDIS_DEBUG
  printf("OnPing\n");
  reply.Print();
#endif
  DetachEvent();
  event_base_loopbreak((struct event_base *)event_);
  if (reply.Type() == ERROR)
    return -1;

  return REDIS_OK;
}

void AsyncClient::InitConnect() {
  //Auth();
  Ping();
  Select();
}

int32_t AsyncClient::Auth() {
  using namespace std::placeholders;

  if (pwd_.empty())
    return REDIS_OK;

  std::vector<std::string> cmd_vec;
  cmd_vec.push_back("AUTH");
  cmd_vec.push_back(pwd_);
  return AsyncCommand(cmd_vec, std::bind(&AsyncClient::OnAuth, this, _1));
}

int32_t AsyncClient::Ping() {
  using namespace std::placeholders;

  std::vector<std::string> cmd_vec;
  cmd_vec.push_back("PING");
  return AsyncCommand(cmd_vec, std::bind(&AsyncClient::OnPing, this, _1));
}

int32_t AsyncClient::Select() {
  using namespace std::placeholders;

  std::vector<std::string> cmd_vec;
  cmd_vec.push_back("SELECT");
  cmd_vec.push_back(std::to_string(static_cast<long long>(db_)));
  return AsyncCommand(cmd_vec, std::bind(&AsyncClient::OnSelect, this, _1));
}

int32_t AsyncClient::AsyncConnect()
{
  Close();     //cleanup before reconnect
  rc_ = redisAsyncConnect(host_.c_str(), port_);
  if (REDIS_OK != rc_->err) {
    errmsg_.assign("AsyncConnect failed:").append(rc_->errstr);
    Close();
    return -1;
  }

  rc_->data = this;//pass object to disconnect handler
  m_IsConn = 1;
  if (!AttachEvent()) {
    Close();
    return -1;
  }

  std::cout << "register  to connnect " << std::endl; 
  if (REDIS_OK != redisAsyncSetConnectCallback(rc_, &AsyncClient::OnAsyncConnect)) {
    std::cout << "set async connnect callback failed" << std::endl;
  }

  LoopEvent();
  std::cout << m_IsConn << std::endl; 
  if (m_IsConn != 2) {
    Close();
    m_IsConn = 0;
    std::cout << "conn failed" << std::endl;
    return -1;
  } else {
    DetachEvent();
  }
  InitConnect();

  redisEnableKeepAlive(&(rc_->c));//ignore fail
  rc_->data = this;
  redisAsyncSetDisconnectCallback(rc_, &AsyncClient::OnAsyncDisconnect);//reconect when async disconnect happen
  return 0;
}

void  AsyncClient::OnAsyncConnect(const struct redisAsyncContext *ac, int status) {
  AsyncClient * async_client = (AsyncClient*)ac->data;
  std::cout << "onSyncConn call,status: " << status 
      << ", errmsg: " << ac->errstr << std::endl;
  if (REDIS_OK == status)
  {
    std::cout << "connected ..." << std::endl;
    async_client->SetConnected();
  } else  {
    std::cout << "connect failed XXX" << std::endl;
    async_client->Clear(); //connect <= 0;
  }

  event_base_loopbreak((struct event_base *)async_client->Event());
}

void AsyncClient::OnAsyncDisconnect(const struct redisAsyncContext *ac, int status) {//err message lost
  if (REDIS_OK == status) return;//disconnect by user
#ifdef REDIS_DEBUG
  //disconnect because of error occured, reconnect then
  fprintf(stderr, "dis connenct [%d]:[%s]\n", ac->err, ac->errstr);
#endif
  AsyncClient* async_client = (AsyncClient*)ac->data;
  async_client->Close();
  event_base_loopbreak((struct event_base *)async_client->Event());
  std::cout << "disconn, errno: " << ac->err << ",errmsg: " << ac->errstr << std::endl;
}

void* AsyncClient::Event() {
  return event_;
}

void AsyncClient::Clear() {
  m_IsConn = 0;
}

void AsyncClient::SetConnected() {
  m_IsConn = 2;
}
void AsyncClient::Close()
{
  if (rc_ != NULL) {
    redisAsyncFree(rc_);
    rc_ = NULL;
  }
  m_IsConn = 0;
}

bool AsyncClient::ConnectedOk() { return m_IsConn == 2; }
int  AsyncClient::ConnectStatus() { return m_IsConn ; }
///////
int32_t RedisSubscriber::AsyncHandlerWrapper(Reply& reply) {
  if (ARRAY == reply.Type() && 3 == reply.Elements().size()) {
    const std::vector<Reply>& elements = reply.Elements();
    if (elements[0].Type() == STRING &&
        elements[1].Type() == STRING &&
        elements[2].Type() == STRING &&
        (elements[0].Str() == "message" || elements[0].Str() == "MESSAGE")) {//process message with the handler_
      return handler_(elements[1].Str(), elements[2].Str()); //elements[1] is channel name, elements[2] is publish data.
    }
  }

  DiscardMessage(reply);
  return REDIS_OK;
}

int32_t RedisSubscriber::AsyncHandlerWrapperMulti(Reply& reply) {
  if (ARRAY == reply.Type() && 3 == reply.Elements().size()) {
    const std::vector<Reply>& elements = reply.Elements();
    if (elements[0].Type() == STRING &&
        elements[1].Type() == STRING &&
        elements[2].Type() == STRING &&
        (elements[0].Str() == "message" || elements[0].Str() == "MESSAGE")) {//process message with the handler_
      if (mp_sub_ret_proc.find(elements[1].Str()) !=  mp_sub_ret_proc.end()) {
        return (*mp_sub_ret_proc[elements[1].Str()])(elements[1].Str(), elements[2].Str());
      }
      //elements[1] is channel name, elements[2] is publish data.
    }
  }

  DiscardMessage(reply);
  return REDIS_OK;
}

void RedisSubscriber::DiscardMessage(Reply& reply) {
#ifdef REDIS_DEBUG
  reply.Print();
#endif
}

//订阅就是调用redis异步client 发送订阅命令
//访问方法:订阅一个频道
bool RedisSubscriber::Subscribe(const std::string& channel, SubscribeHandler handler) {
  if (channel.empty()) return false;

  cmd_vec_.clear();
  cmd_vec_.push_back("SUBSCRIBE");
  cmd_vec_.push_back(channel);

  handler_ = handler;

  return SubscribeImpl();
}

//订阅多个频道
bool RedisSubscriber::Subscribe(const std::vector<std::string>& channel_vec, SubscribeHandler handler) {
  cmd_vec_.clear();
  cmd_vec_.push_back("SUBSCRIBE");
  for (size_t i = 0; i != channel_vec.size(); ++i) {
    if (!channel_vec[i].empty()) {
      cmd_vec_.push_back(channel_vec[i]);
    }
  }

  if (1 == cmd_vec_.size()) return false;

  handler_ = handler;

  return SubscribeImpl();
}

//订阅多个，并且每个的订阅结果处理接口都可以自定义
bool RedisSubscriber::Subscribe(const std::map<std::string, SubRetProcBase*>& mpCmdHandler) {
  client_.ClearErrMsg();
  mp_sub_ret_proc.clear();
  mp_sub_ret_proc = mpCmdHandler;

  cmd_vec_.clear();
  cmd_vec_.push_back("SUBSCRIBE");
  std::map<std::string, SubRetProcBase*>::const_iterator  it ;
  for (it = mpCmdHandler.begin(); it != mpCmdHandler.end(); ++it) {
    if (!it->first.empty()) {
      cmd_vec_.push_back(it->first);
    }
  }

  if (1 == cmd_vec_.size()) { 
    client_.SetErrmsg("subscribe channel is empty");
    return false;
  }

  return client_.AsyncCommand(cmd_vec_, std::bind(&RedisSubscriber::AsyncHandlerWrapperMulti, this, std::placeholders::_1));

}

const char* RedisSubscriber::Errmsg() {
  return client_.Errmsg();
}

bool RedisSubscriber::SubscribeImpl() {
  return client_.AsyncCommand(cmd_vec_, std::bind(&RedisSubscriber::AsyncHandlerWrapper, this, std::placeholders::_1));
}

////
}

