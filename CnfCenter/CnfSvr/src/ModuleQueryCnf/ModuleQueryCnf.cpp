#include "ModuleQueryCnf.h"


#ifdef __cplusplus
extern "C" {
#endif
  oss::Cmd* create()
  {
        oss::Cmd* pCmd = new CNF_SRV::ModuleQueryCnf();
            return(pCmd);
  }
#ifdef __cplusplus
}
#endif

using namespace oss;
namespace CNF_SRV {

ModuleQueryCnf::ModuleQueryCnf() {
  //
}

ModuleQueryCnf::~ModuleQueryCnf() {

  std::map<std::string, CnfSrvOp*>::iterator it;
  for (it= m_CnfSrvOp.begin(); it != m_CnfSrvOp.end(); ++it) {
    if (it->second) { delete it->second; }
  }
  m_CnfSrvOp.clear();
}
//从 custom 配置项 获取业务配置信息
bool ModuleQueryCnf::Init() {
  m_CnfSrvOp = CnfSrvOpRegister::RegisterOp(GetLabor()->GetCustomConf(),GetLabor()->GetLogger()); 
  return true;
}


//msg body content format: {"conf_type": "host_conf",
//                          "host_conf": {"ip":"","port":0,
//                                        "srvname": ""
//                                        } 
//                        }
//                        or 
//                        {"conf_type":"srv_name",
//                          "srv_name":"achilsh"
//                        }
bool ModuleQueryCnf::AnyMessage(
    const oss::tagMsgShell& stMsgShell,
    const HttpMsg& oInHttpMsg) {
  m_tagMsgShell = stMsgShell;
  m_oInHttpMsg = oInHttpMsg;
  
  if (HTTP_POST != oInHttpMsg.method()) {
    LOG4_ERROR("req http not post method");
    SendAck(100, "http req method not post");
    return false;
  }

  if (HTTP_REQUEST != oInHttpMsg.type()) {
    LOG4_ERROR("req http is request");
    SendAck(100, "http no request");
    return false;
  }

  loss::CJsonObject jsParam;
  if (jsParam.Parse(oInHttpMsg.body()) == false) {
    LOG4_ERROR("parse http body failed");
    SendAck(100, "parse http body json err");
    return false;
  }

  std::string cnfTypeVal;
  if (false == jsParam.Get(CNF_TYPE_INDEX, cnfTypeVal)) {
    LOG4_ERROR("get cnf item: %s value failed", CNF_TYPE_INDEX);
    SendAck(100, "get cnf item value failed");
    return false;
  }

  std::map<std::string, CnfSrvOp*>::iterator it  ;
  it =  m_CnfSrvOp.find(cnfTypeVal);
  if (it == m_CnfSrvOp.end() || it->second == NULL) {
    LOG4_ERROR("has not carry out cnf query interface, type: %s",
               cnfTypeVal.c_str());
    SendAck(100,"server has not carry out function");
    return false;
  }

  LOG4_TRACE("query type: %s",  cnfTypeVal.c_str());
  loss::CJsonObject queryRet;
  CnfSrvOp* cnfQuery = it->second;
  if (false == cnfQuery->OpCnfSrvQuery(jsParam, queryRet)) {
    LOG4_ERROR("query cnf failed, type: %s", cnfTypeVal.c_str());
    SendAck(100, "query cnf fail");
    return false;
  }
  SendAck(0, queryRet.ToString());

  return true;
}

void ModuleQueryCnf::SendAck(int ino, const std::string serrmsg) {
  HttpMsg outHttpMsg;
  outHttpMsg.set_type(HTTP_RESPONSE);
  outHttpMsg.set_status_code(200);
  outHttpMsg.set_http_major(m_oInHttpMsg.http_major());
  outHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());
  
  loss::CJsonObject retJson;
  if (ino == 0) {
    retJson.Add("code", ino);
    retJson.Add("data", loss::CJsonObject(serrmsg));
  } else {
    retJson.Add("code", ino);
    retJson.Add("err", serrmsg);
  }

  outHttpMsg.set_body(retJson.ToString());
  GetLabor()->SendTo(m_tagMsgShell,outHttpMsg);
}

//////////
}
