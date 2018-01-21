#include "StepTestQuery.h"
#include "thrift_serialize.h"
//
#include "hello_test_types.h"
using namespace Test; 


namespace im {

int StepTestQuery::m_Test = 0;

StepTestQuery::StepTestQuery(const oss::tagMsgShell& stMsgShell,
                            const HttpMsg& oHttpMsg,
                            const std::string& sVal) {
  m_stMsgShell = stMsgShell; m_oHttpMsg = oHttpMsg; m_sKey = sVal;
}
//
StepTestQuery::~StepTestQuery() {
}

oss::E_CMD_STATUS StepTestQuery::Timeout() {
  LOG4_ERROR("StepTestQuery tm out ");
  LOG4_ALARM_REPORT("resp tm out, key: %s", m_sKey.c_str());
  SendAck("step Test query time out");
  return  oss::STATUS_CMD_FAULT;
}
//
oss::E_CMD_STATUS StepTestQuery::Emit(int err, 
                       const std::string& strErrMsg ,
                       const std::string& strErrShow ) {
  MsgBody oOutMsgBody;
  MsgHead oOutMsgHead;
  LOG4_TRACE("===== 54 again =====");
  /***
  loss::CJsonObject tData; 
  tData.Add("key",m_sKey);
  oOutMsgBody.set_body(tData.ToString());
  ***/
  OneTest t;
  t.__set_fOne(m_sKey); 
  std::string sData;
  ThrifSerialize<OneTest>::ToString(t,sData);
  
  oOutMsgBody.set_body(sData);

  oOutMsgHead.set_cmd(101); //this is command no.
  oOutMsgHead.set_seq(GetSequence());
  oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
  
  LOG4_TRACE("send req to TestLogic, msg body serailze len: %d, in head body len: %d", 
             oOutMsgBody.ByteSize(), oOutMsgHead.msgbody_len());
  std::string strDstNodeType;
  if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) {
    LOG4_ERROR("get node type fail by cmd: %u", 101);
    LOG4_ALARM_REPORT("not get node type for cmd: %u", 101);
    return oss::STATUS_CMD_FAULT;
  }

  //test session id;
  SetId(m_sKey);

  if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody, this)) {
    LOG4_ERROR("send data to TestLogic failed");
    LOG4_ALARM_REPORT("send to next fail");
    
    return oss::STATUS_CMD_FAULT;
  }

  return oss::STATUS_CMD_RUNNING;
}

oss::E_CMD_STATUS StepTestQuery::Callback(
         const oss::tagMsgShell& stMsgShell,
         const MsgHead& oInMsgHead,
         const MsgBody& oInMsgBody,
         void* data) {
  /*****
  loss::CJsonObject jsData;
  std::string sData = oInMsgBody.body();
  if (false == jsData.Parse(sData)) {
    sData = "http parse ret body fail";
    SendAck(sData);
  } else {
    sData= jsData("key");
    SendAck("", sData);
  }
  ****/
  OneTest t;
  std::string sData = oInMsgBody.body();
  if (0 != ThrifSerialize<OneTest>::FromString(sData,t)) {
    sData = "http parse ret body fail";
    LOG4_ALARM_REPORT("serialize fail");
    SendAck(sData);
  } else  {
    sData = t.fOne;
    SendAck("", sData);
  }
  return oss::STATUS_CMD_COMPLETED;
}

void StepTestQuery::SendAck(const std::string& sErr, const std::string &sEData) {
  std::string sData;
  if (sErr.empty() == false) {
    sData = sErr;
  } else {
    sData = sEData;
  }

  HttpMsg oOutHttpMsg;
  oOutHttpMsg.set_type(HTTP_RESPONSE);
  oOutHttpMsg.set_status_code(200);
  oOutHttpMsg.set_http_major(m_oHttpMsg.http_major());
  oOutHttpMsg.set_http_minor(m_oHttpMsg.http_minor());

  loss::CJsonObject retJson;
  retJson.Add("ret", sData);
  oOutHttpMsg.set_body(retJson.ToString());
  SendTo(m_stMsgShell,oOutHttpMsg);
}
//
}
