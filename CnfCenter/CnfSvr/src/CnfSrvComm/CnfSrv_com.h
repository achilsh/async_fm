#ifndef _CNF_SRV_COMM_H_
#define _CNF_SRV_COMM_H_ 

#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "util/json/CJsonObject.hpp"
#include "lib_redis.h"
#include <map>
#include "OssDefine.hpp"
//中心服务的自身配置里 设定的配置项索引名字名 
#define CNF_TYPE_INDEX  "conf_type"

//消息发布，订阅频道。
#define CNF_HOST_CONF   "host_conf"
#define CNF_SRV_NAME    "srv_name"
//.......

#define CNF_HOST_CNF_IP    "ip"
#define CNF_HOST_CNF_PORT  "port"
#define CNF_HOST_CNF_SRV_NAME  "srvname"

//这是存储主机配置信息的redis hash field name.
#define FieldNameFilePath      "1"
#define FieldNameCnfContent    "2"

// 这是存储的redis key 前缀。
#define PREFIXHOSTKEY          "cnf_H1"
#define PREFIXSVRNMKEY            "cnf_N1"

//这是查询请求的返回结果中json 前缀。
#define FilePathNameJSIndex      "cnf_path"
#define CnfContentJsIndex        "cnf_content"
//
#define NODETYPE_CMD_KEY_PREX     "nodetype_cmd"
#define NODETYPE_CMD_PUB_KEY       NODETYPE_CMD_KEY_PREX
//
#define CMD_NODETYPE_KEY_PRE      "cmd_nodetype"
//
using namespace LIB_COMM;
using namespace LIB_REDIS;
using namespace loss;

namespace CNF_SRV {
  
  class  CnfSrvOp;
  class CnfSrvOpRegister {
    public:
     static std::map<std::string, CnfSrvOp*>& RegisterOp(
         const loss::CJsonObject& jsSrvCnf,
         const log4cplus::Logger& pLogger);
  };

  //
  class CnfSrvOp {
    public:
     CnfSrvOp(const loss::CJsonObject& jsSrvCnf);
     virtual ~CnfSrvOp ();

     bool OpCnfSrvQuery(const loss::CJsonObject& jsCondition, loss::CJsonObject& msgQuery) {
       if (false == m_Init)  {
         return false;
       }
       return CnfSrvQuery(jsCondition, msgQuery);
     }

     bool OpCnfSrvCreate(const loss::CJsonObject& cnfData){
       if (false == m_Init)  {
         LOG4_ERROR("create failed, as not been init");
         return false;
       }
       if (false == CnfSrvCreate(cnfData)) {
         return false;
       }
       PushWriteOp();
       return true;
     }

     //http svr 暂时不会用到该接口
     bool OpCnfSrvModify(const loss::CJsonObject& cnfData) {
       if (false == m_Init)  {
         return false;
       }
       //所以 CnfSrvModify()暂时也不会用
       if (false == CnfSrvModify(cnfData)) {
          return false;
       }
       PushWriteOp();
       return true;
     }
     void SetPubChannel(const std::string& sCh) {
       m_PushChannel = sCh;
     }
     void  SetLoger(const log4cplus::Logger& logger) { m_pLogger = logger; } 
     log4cplus::Logger GetLogger() { return m_pLogger; }
     bool InitRedis(); 

    protected:
     virtual bool CnfSrvQuery(const loss::CJsonObject& jsCondition, loss::CJsonObject& msgQuery) = 0;
     virtual bool CnfSrvCreate(const loss::CJsonObject& cnfData) = 0;
     virtual bool CnfSrvModify(const loss::CJsonObject& cnfData) = 0;

    protected:
     bool PushWriteOp();
    protected:
     loss::CJsonObject  m_jsSrvCnf;
     Client             m_syncRedisCli;     
     bool               m_Init;
     std::string        m_PushChannel; //某种配置服务订阅的频道
     std::string        m_PushData;
     log4cplus::Logger  m_pLogger;
  };

  //
  class CnfSrvHostCnf: public CnfSrvOp {
    public:
     CnfSrvHostCnf(const loss::CJsonObject& jsSrvCnf);
     virtual ~CnfSrvHostCnf();

    protected: 
     virtual bool CnfSrvQuery(const loss::CJsonObject& jsCondition, 
                              loss::CJsonObject& msgQuery);
     virtual bool CnfSrvCreate(const loss::CJsonObject& cnfData);
     virtual bool CnfSrvModify(const loss::CJsonObject& cnfData);
    private:
     bool ParseCnfHostConf(const loss::CJsonObject& cnfData,
                           std::string& sIp,uint32_t& uiPort,
                           std::string& sSrvName);
     bool PubCmdIdNodeType(const loss::CJsonObject& cnfData);
     bool UpdateCmdNodeType(const std::string& sNodeType,
                            const std::vector<std::string>& vNewCmdList,
                            const std::vector<std::string>& vPreCmdList);
  };
  //
  class CnfSrvNameCnf: public CnfSrvOp {
   public:
    CnfSrvNameCnf(const loss::CJsonObject& jsSrvCnf);
    virtual ~CnfSrvNameCnf();
   protected:
    virtual bool CnfSrvQuery(const loss::CJsonObject& jsCondition, 
                             loss::CJsonObject& msgQuery);
    virtual bool CnfSrvCreate(const loss::CJsonObject& cnfData);
    virtual bool CnfSrvModify(const loss::CJsonObject& cnfData);
  };
//////////////
}
#endif 
