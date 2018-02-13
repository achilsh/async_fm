/*******************************************************************************
 * Project:  AsyncServer
 * @file     RedisStep.hpp
 * @brief    带Redis的异步步骤基类
 * @author   
 * @date:    2015年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_REDISSTEP_HPP_
#define SRC_STEP_REDISSTEP_HPP_
#include <set>
#include <list>
#include <vector>
#include "dbi/redis/RedisCmd.hpp"
#include "Step.hpp"


namespace oss
{

enum ReplyType
{
  STRING    = 1,
  ARRAY     = 2,
  INTEGER   = 3,
  NIL       = 4,
  STATUS    = 5,
  ERROR     = 6
};

class OssReply
{
 public:
  ReplyType Type() const {return type_;}
  long long Integer() const {return integer_;}
  const std::string& Str() const {return str_;}
  const std::vector<OssReply>& Elements() const {return elements_;}

  void SwapFunc(redisReply* other)
  {
  }

  OssReply(redisReply *reply = NULL):type_(ERROR), integer_(0)
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
        for (size_t i = 0; i < reply->elements; ++i)
        {
          elements_.push_back(OssReply(reply->element[i]));
        }
        break;
      default:
        break;
    }
  }

  ~OssReply(){}

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
      const std::vector<OssReply>& elements = Elements();

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
  std::vector<OssReply>  elements_;
};

/**
 * @brief StepRedis在回调后一定会被删除
 */
class RedisStep: virtual public Step
{
public:
    RedisStep(const std::string& sCoName);

    RedisStep(const tagMsgShell& stReqMsgShell, 
              const MsgHead& oReqMsgHead, 
              const MsgBody& oReqMsgBody, 
              const std::string& sCoName ="");
    virtual ~RedisStep();

    /**
     * @brief: CorFunc
     *  由业务的子类来实现，
     *  该接口已经被协程调用
     *
     *  协程只需在该接口内部写同步逻辑即可
     */ 
    virtual void CorFunc() = 0;

public:

    /**
     * @brief: ExecuteRedisCmd 
     * 逻辑上同步执行redis cmd, 框架底层异步收发命令和结果
     *
     * @param reply: redis 是命令执行的结果
     * , 使用时只要传入一个 OssReply(NULL),
     * 接口会把获取的结果地址复制给入参
     *
     * @return: false, 失败; true, 成功
     *  具体的错误码和错误信息存放在成员:
     * m_iRedisErrNo 和m_sRedisErrMsg 中。
     */
    bool ExecuteRedisCmd(OssReply*& reply);

    /**
     * @brief: SetRedisRetBody 
     * 框架在收到redis回报后，调用该接口完成回报的保存 
     *
     * @param replyRedis
     */
    void SetRedisRetBody(redisReply* replyRedis);

    loss::RedisCmd* RedisCmd()
    {
        return(m_pRedisCmd);
    }
    
    void SetRedisCmdErrMsg(const std::string& sMsg)
    {
       m_sRedisErrMsg  =  sMsg;
    }

    void SetRedisCmdErrNo(int32_t iENo)
    {
        m_iRedisErrNo =  iENo;
    }
    //--------------------------------//
    const std::string& GetRedisCmdErrMsg() const
    {
        return  m_sRedisErrMsg;
    }
    int32_t GetRedisErrNo() 
    {
        return m_iRedisErrNo;
    }

    OssReply& GetRedisCmdResult() 
    {
       return m_redisRetBody;
    }
public:
    const loss::RedisCmd* GetRedisCmd()
    {
        return(m_pRedisCmd);
    }

protected:
    int32_t     m_iRedisErrNo;
    std::string m_sRedisErrMsg;
    OssReply    m_redisRetBody;

private:
    loss::RedisCmd* m_pRedisCmd;

};


/**
 * @brief Redis连接属性
 * @note  Redis连接属性，因内部带有许多指针，并且没有必要提供深拷贝构造，所以不可以拷贝，也无需拷贝
 */
struct tagRedisAttr
{
    uint32 ulSeq;                           ///< redis连接序列号
    redisReply* pReply;                     ///< redis命令执行结果
    bool bIsReady;                          ///< redis连接是否准备就绪
    std::list<RedisStep*> listData;         ///< redis连接回调数据
    std::list<RedisStep*> listWaitData;     ///< redis等待连接成功需执行命令的数据

    tagRedisAttr() : ulSeq(0), pReply(NULL), bIsReady(false)
    {
    }

    ~tagRedisAttr()
    {
        //freeReplyObject(pReply);  redisProcessCallbacks()函数中有自动回收
        for (std::list<RedisStep*>::iterator step_iter = listData.begin();
                        step_iter != listData.end(); ++step_iter)
        {
            if (*step_iter != NULL)
            {
                //delete (*step_iter); 由协程管理器实例来释放
                *step_iter = NULL;
            }
        }
        listData.clear();

        for (std::list<RedisStep*>::iterator step_iter = listWaitData.begin();
                        step_iter != listWaitData.end(); ++step_iter)
        {
            if (*step_iter != NULL)
            {
                delete (*step_iter);
                //*step_iter = NULL;
            }
        }
        listWaitData.clear();
    }
};

} /* namespace oss */

#endif /* SRC_STEP_REDISSTEP_HPP_ */
