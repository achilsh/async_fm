#include <sstream>
#include "CnfSrv_com.h"

namespace CNF_SRV {

//这是从配置文件中读取配置的总类，对各个配置种类创建{查询，创建，修改} 类
std::map<std::string, CnfSrvOp*>&
    CnfSrvOpRegister::RegisterOp(const loss::CJsonObject& jsSrvCnf, const log4cplus::Logger& logger) {
      loss::CJsonObject chListJson;
      static std::map<std::string, CnfSrvOp*> mpCnfSrv;
      std::map<std::string, CnfSrvOp*>::iterator it;
      mpCnfSrv.clear();
      //foramte: {"conf_type": ["host_conf", "srv_name"] }
      //也是信息变更时，消息发布频道，cnf agent 订阅频道。
      bool bRet = jsSrvCnf.Get(CNF_TYPE_INDEX, chListJson);
      if (bRet == false) {
        return mpCnfSrv;
      }

      for (int iArr = 0 ; iArr < chListJson.GetArraySize(); ++iArr) {
        std::string sCnfType  = chListJson(iArr);
        CnfSrvOp* cnfInstance = NULL;

        if (sCnfType == CNF_HOST_CONF) {
          cnfInstance  = new CnfSrvHostCnf(jsSrvCnf);
          LOG4CPLUS_INFO_FMT(logger, "register host cnf op");
        } else if (sCnfType  == CNF_SRV_NAME) {
          LOG4CPLUS_INFO_FMT(logger, "register srv name op");
          cnfInstance  = new CnfSrvNameCnf(jsSrvCnf);
        } else {

        }

        if (cnfInstance == NULL) {
          continue;
        }

        cnfInstance->SetPubChannel(sCnfType);
        cnfInstance->SetLoger(logger);
        cnfInstance->InitRedis();

        it = mpCnfSrv.find(sCnfType);
        if (it == mpCnfSrv.end()) {
          mpCnfSrv[sCnfType] = cnfInstance;  
        } else {
          delete it->second;
          it->second = cnfInstance;
        }
      }
      return mpCnfSrv;
    }

CnfSrvOp::CnfSrvOp(const loss::CJsonObject& jsSrvCnf):m_jsSrvCnf(jsSrvCnf), m_Init(false){
}

CnfSrvOp::~CnfSrvOp () {
  m_Init  = false;
}


// redis conf format:
//{"redis": {"host":"", "port": 0, "pwd": "", "db": 0} }
bool CnfSrvOp::InitRedis() {
  if (m_Init == true) {
    return true;
  }

  loss::CJsonObject cJsonRedis;
  bool bRet = m_jsSrvCnf.Get("redis", cJsonRedis);
  if (bRet == false) {
    return false;
  }
  std::string sRedisHost = "";
  int32_t iRedisPort = 6379;
  std::string sRedisPWD = "";
  int32_t sRedisDbId = 0;

  cJsonRedis.Get("ip",sRedisHost);
  cJsonRedis.Get("port",iRedisPort);
  cJsonRedis.Get("pwd",sRedisPWD);
  cJsonRedis.Get("db",sRedisDbId);
  LOG4_TRACE("redis conf, ip: %s, port: %d, pwd: %s, db: %d",
             sRedisHost.c_str(), iRedisPort, sRedisPWD.c_str(), sRedisDbId);
  
  if (0 != m_syncRedisCli.Init(sRedisHost,
                               iRedisPort,
                               sRedisPWD,
                               sRedisDbId)) {
    LOG4_ERROR("init redis failed,reid ip: %s, port: %u,"
               "pwd: %s, db: %d", sRedisHost.c_str(),
               iRedisPort,sRedisPWD.c_str(), sRedisDbId);

    return false;
  }

  m_Init = true;
  return true;
}

bool CnfSrvOp::PushWriteOp() {
  if (m_PushData.empty() || m_PushChannel.empty()) {
    return false;
  }

  RedisPublisher rPub(m_syncRedisCli); 
  if (false == rPub.Publish(m_PushChannel,m_PushData)) {
    return false;
  }
  return true;
}

CnfSrvHostCnf::CnfSrvHostCnf(const loss::CJsonObject& jsSrvCnf) 
  :CnfSrvOp(jsSrvCnf) {
}

CnfSrvHostCnf::~CnfSrvHostCnf() {
}

bool CnfSrvHostCnf::ParseCnfHostConf(const loss::CJsonObject& jsCondition,
                                     std::string& sIp,uint32_t& uiPort,
                                     std::string& sSrvName) {
  loss::CJsonObject hostConfQuery;
  //foramt: {"host_conf": {"ip": "", "port": 0, "srvname": ""} }
  bool bRet = jsCondition.Get(CNF_HOST_CONF, hostConfQuery);
  if (bRet == false) {
    return false;
  }

  if (!(hostConfQuery.Get(CNF_HOST_CNF_IP, sIp) && 
        hostConfQuery.Get(CNF_HOST_CNF_PORT,uiPort) && 
        hostConfQuery.Get(CNF_HOST_CNF_SRV_NAME, sSrvName))) {
    LOG4_ERROR("get ip, port, srvname from msg body failed");
    return false;
  }
  return true;
}

// query host conf, redis key is=>   prefix:ip:port:srvname
// so, json is: {"host_conf_query": {"ip": "127.0.1","port":6565, "srvname":"test_a" }}
bool CnfSrvHostCnf::CnfSrvQuery(const loss::CJsonObject& jsCondition, 
                                loss::CJsonObject& msgQuery) {
  std::string sCnfHost("");
  uint32_t  uiCnfPort = 0;
  std::string sCnfSrvName("");
  LOG4_TRACE("req msg: %s", jsCondition.ToString().c_str());
  //目前现支持通过IP，port， srvname 三个参数来查询服务配置的详情。
  //
  if (false == ParseCnfHostConf(jsCondition, sCnfHost,
                                uiCnfPort, sCnfSrvName)) {
    LOG4_ERROR("parse host, port, srv name from cnf failed");
    return false;
  }

  std::stringstream ios; //todo
  ios << PREFIXHOSTKEY << ":" << sCnfHost << ":" << uiCnfPort
      << ":" << sCnfSrvName;
  std::string sKey(ios.str());
  LOG4_TRACE("query redis key: %s", sKey.c_str());

  std::vector<std::string> vField;
  vField.push_back(FieldNameFilePath);
  vField.push_back(FieldNameCnfContent);
  
  RedisHash rHashOp(m_syncRedisCli);
  std::map<std::string, std::string> mHVals;

  if (false == rHashOp.hMGet(sKey, vField, mHVals)) {
    LOG4_ERROR("redis hmget failed, key: %s, errmsg: %s",
               sKey.c_str(), rHashOp.Errmsg().c_str());
    return false;
  }

  const std::string& sFilePathName = mHVals[FieldNameFilePath];
  const std::string& sCnfContent  = mHVals[FieldNameCnfContent];
  
  //return data format like this: {"cnf_path: "/etc/xxx.json" ,"cnf_content":  {} }
  msgQuery.Clear();
  msgQuery.Add(FilePathNameJSIndex, sFilePathName);
  if (sCnfContent.empty()) {
    msgQuery.AddEmptySubObject(CnfContentJsIndex);
  } else {
    msgQuery.Add(CnfContentJsIndex, loss::CJsonObject(sCnfContent));
  }
  return true;
}

//ip, port , srvname,   conf file path, conf content, 
//{"host_conf": {"ip":"","port": , "srvname": ""},
//"cnf_path": "",
//"cnf_content": {} 
//}
bool CnfSrvHostCnf::CnfSrvCreate(const loss::CJsonObject& cnfData) {
  std::string sCnfHost("");
  uint32_t  uiCnfPort = 0;
  std::string sCnfSrvName("");
  LOG4_TRACE("create host cnf"); 
  if (false == ParseCnfHostConf(cnfData, sCnfHost,
                                uiCnfPort,sCnfSrvName)) {
    LOG4_ERROR("parse cnf host port srvname failed from http body");
    return false;
  }

  std::string sCnfPath;
  loss::CJsonObject jsCnfContent;
  
  if (cnfData.Get(FilePathNameJSIndex,sCnfPath) == false ||
      cnfData.Get(CnfContentJsIndex, jsCnfContent) == false) {
    return false;
  }

  std::stringstream ios; //todo
  ios << PREFIXHOSTKEY << ":" << sCnfHost << ":" << uiCnfPort
      << ":" << sCnfSrvName;
  std::string sKey(ios.str());
  
  /*** 
  RedisKey keyOp(m_syncRedisCli);
  if (true == keyOp.Exists(sKey)) {
    //is exist, so not can been create new host conf.
    return false;
  }
  ***/

  RedisHash hashOp(m_syncRedisCli);
  bool bSucc = true;
  if (false == hashOp.hSet(sKey, FieldNameFilePath, sCnfPath)) {
    LOG4_ERROR("hset cmd: %s fail, errmsg: %s",
               sKey.c_str(),hashOp.Errmsg().c_str());
    LOG4_ERROR("hset failed, key: %s, field: %s", sKey.c_str(),
               FieldNameFilePath);
    bSucc = false;
  }
  if (false ==  hashOp.hSet(sKey, FieldNameCnfContent,
                           jsCnfContent.ToString())) {
    LOG4_ERROR("hset cmd: %s fail, errmsg: %s",
               sKey.c_str(),hashOp.Errmsg().c_str());
    bSucc = false;
  }

  m_PushData.clear();
  loss::CJsonObject jsPushData;
  //format: {"ip":"", "cnf_path": "", "cnf_dat": {} }
  jsPushData.Add("ip", sCnfHost);
  jsPushData.Add("port", uiCnfPort);
  jsPushData.Add("cnf_path", sCnfPath);
  jsPushData.Add("cnf_dat", jsCnfContent);
  m_PushData.append(jsPushData.ToString());
  
  return true;
}

//ip, port , srvname,   conf file path, conf content, 
//{"host_conf": {"ip":"","port": , "srvname": ""},
//"cnf_path": "",
//"cnf_content": {} 
//}
bool CnfSrvHostCnf::CnfSrvModify(const loss::CJsonObject& cnfData) {
  if (m_Init == false) {
    return false;
  }

  std::string sCnfHost("");
  uint32_t  uiCnfPort = 0;
  std::string sCnfSrvName("");
  
  if (false == ParseCnfHostConf(cnfData, sCnfHost,
                                uiCnfPort,sCnfSrvName)) {
    return false;
  }

  std::string sCnfPath;
  loss::CJsonObject jsCnfContent;
  
  if (cnfData.Get(FilePathNameJSIndex, sCnfPath) == false ||
      cnfData.Get(CnfContentJsIndex, jsCnfContent) == false) {
    return false;
  }

  std::stringstream ios; //todo
  ios << PREFIXHOSTKEY << ":" << sCnfHost << ":" << uiCnfPort
      << ":" << sCnfSrvName;
  std::string sKey(ios.str());

  RedisHash hashOp(m_syncRedisCli);
  bool bSucc = true;
  if (false == hashOp.hSet(sKey, FieldNameFilePath, sCnfPath)) {
    bSucc = false;
  }
  if (false ==  hashOp.hSet(sKey, FieldNameCnfContent,
                           jsCnfContent.ToString())) {
    bSucc = false;
  }
  if (bSucc == false) {
    return false;
  }
  m_PushData.clear();
  loss::CJsonObject jsPushData;
  //format: {"ip":"", "cnf_path": "","port":0, "cnf_dat": {} }
  jsPushData.Add("ip", sCnfHost);
  jsPushData.Add("port", uiCnfPort);
  jsPushData.Add("cnf_path", sCnfPath);
  jsPushData.Add("cnf_dat", jsCnfContent);
  m_PushData.append(jsPushData.ToString());
  return true;
}


CnfSrvNameCnf::CnfSrvNameCnf(const loss::CJsonObject& jsSrvCnf)
  :CnfSrvOp(jsSrvCnf){
}
CnfSrvNameCnf::~CnfSrvNameCnf() {
}

/*****************************************/
//服务名配置存储格式为 ： string
//key: 服务名
//value: 服务配置{ip, port}
/**********************************/
// each one string format: {"ip":"","port":0}
bool CnfSrvNameCnf::CnfSrvQuery(const loss::CJsonObject& jsCondition, 
                                loss::CJsonObject& msgQuery) {
  //input param format: {"srv_name":"achilsh"}  
  //output format: {"achilsh": [{"ip":"","port":0},{},{} ]}
  std::string sSrvName;
  if (false == jsCondition.Get(CNF_SRV_NAME, sSrvName)) {
    return false;
  }
  std::string sKey;
  std::stringstream ios;
  ios << PREFIXSVRNMKEY << ":" << sSrvName;
  sKey.append(ios.str());
  
  LOG4_TRACE("query srv name redis key: %s", sKey.c_str());
  std::string sHostList;  //format: [{"ip":"","port":0}, {"ip":"","port":0}]
  RedisKey keyOp(m_syncRedisCli);
  if (false == keyOp.Get(sKey, sHostList)) {
    LOG4_ERROR("get redis key value failed, key: %s",sKey.c_str());
    return false;
  }

  if (sHostList.empty()) {
    msgQuery.AddEmptySubArray(sSrvName);
    return true;
  }

  loss::CJsonObject jsHostList;
  if (false == jsHostList.Parse(sHostList)) {
    LOG4_ERROR("parse ret json ret failed, redis key: %s", sKey.c_str());
    return false;
  }

  if (jsHostList.IsArray() == false) {
    LOG4_ERROR("parse ret json not json list format");  
    return false;
  }
  msgQuery.Add(sSrvName, jsHostList);
  return true;
  //[{"ip":"","port": 0}, {"ip":"","port": 0}]

}

bool CnfSrvNameCnf::CnfSrvCreate(const loss::CJsonObject& jsCondition) {
  //input param format: {"srv_name":"achilsh", 
  //                      "achilsh":[{"ip":"","port": 0}, 
  //                                 {},
  //                                 {}
  //                                 ]
  //                    } 
  std::string sSrvName;
  if (false == jsCondition.Get(CNF_SRV_NAME, sSrvName)) {
    return false;
  }

  loss::CJsonObject cjHostList;
  if (false == jsCondition.Get(sSrvName, cjHostList)) {
    LOG4_ERROR("parse host list from string failed");
    return false;
  }

  std::string sKey;
  std::stringstream ios;
  ios << PREFIXSVRNMKEY << ":" << sSrvName;
  sKey.append(ios.str());

  //存进redis里面的是 [] json 列表,
  RedisKey keyOp(m_syncRedisCli);
  std::string sHostList(cjHostList.ToString());
  if (false == keyOp.Set(sKey, sHostList)) {
    LOG4_ERROR("set redis string failed, key: %s", sKey.c_str());
    return false;
  }

  LOG4_TRACE("set string to redis, key: %s", sKey.c_str());
  
  m_PushData.clear();
  loss::CJsonObject jsPushData;
  //format: {"achilsh": [{},{},{}]}
  jsPushData.Add(sSrvName,cjHostList);
  m_PushData.append(jsPushData.ToString());
  return true;
}

bool CnfSrvNameCnf::CnfSrvModify(const loss::CJsonObject& cnfData) {
  return CnfSrvCreate(cnfData);
}
/////
}
