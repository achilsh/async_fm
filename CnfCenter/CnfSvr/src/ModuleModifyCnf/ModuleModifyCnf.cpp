#include "ModuleModifyCnf.h"

#ifdef __cplusplus
extern "C" {
#endif
  oss::Cmd* create()
  {
        oss::Cmd* pCmd = new CNF_SRV::ModuleModifyCnf();
            return(pCmd);
  }
#ifdef __cplusplus
}
#endif

using namespace oss;
namespace CNF_SRV {
ModuleModifyCnf::ModuleModifyCnf(){
}

ModuleModifyCnf::~ModuleModifyCnf() {
  std::map<std::string, CnfSrvOp*>::iterator it;
  for (it= m_CnfSrvOp.begin(); it != m_CnfSrvOp.end(); ++it) {
    if (it->second) { delete it->second; }
  }
  m_CnfSrvOp.clear();
}

bool ModuleModifyCnf::Init() {
  m_CnfSrvOp = CnfSrvOpRegister::RegisterOp(GetLabor()->GetCustomConf(),GetLabor()->GetLogger()); 
  return true;
}

//msg body content format: {"conf_type": "host_conf",
//                          "host_conf": {"ip":"","port":0,
//                                        "srvname": ""
//                                        },
//                          "cnf_path": "",
//                          "cnf_content": {}
//                        }
//                        or 
//                        {"conf_type":"srv_name",
//                          "srv_name": [{"ip":"","port": 0},
//                                       {},
//                                       {}
//                                      ]
//                        }
//
bool ModuleModifyCnf::AnyMessage(
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
    LOG4_ERROR("has not carry out cnf modify interface, type: %s",
               cnfTypeVal.c_str());
    SendAck(100,"server has not carry out function");
    return false;
  }

  CnfSrvOp* cnfModify = it->second;
  if (false == cnfModify->OpCnfSrvCreate(jsParam)) { //include: create + pub op
     LOG4_ERROR("modify cnf failed, type: %s",cnfTypeVal.c_str());
     SendAck(100,"modify cnf failed");
     return false;
  }
  SendAck(0,"succ");
  return true;
}

void ModuleModifyCnf::SendAck(int ino, const std::string serrmsg) {
  HttpMsg outHttpMsg;
  outHttpMsg.set_type(HTTP_RESPONSE);
  outHttpMsg.set_status_code(200);
  outHttpMsg.set_http_major(m_oInHttpMsg.http_major());
  outHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());

  loss::CJsonObject retJson;
  retJson.Add("code", ino);
  retJson.Add("msg", serrmsg);

  outHttpMsg.set_body(retJson.ToString());
  GetLabor()->SendTo(m_tagMsgShell,outHttpMsg);
}

}
