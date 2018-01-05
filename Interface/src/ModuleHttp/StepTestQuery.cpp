#include "StepTestQuery.h"

namespace im {

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
  SendAck("step Test query time out");
  return  oss::STATUS_CMD_FAULT;
}
//
oss::E_CMD_STATUS StepTestQuery::Emit(int err, 
                       const std::string& strErrMsg ,
                       const std::string& strErrShow ) {
  MsgBody oOutMsgBody;
  MsgHead oOutMsgHead;
#if 1
  loss::CJsonObject tData; 
  tData.Add("key",m_sKey);
  oOutMsgBody.set_body(tData.ToString());
  LOG4_TRACE("add test 1 ");  
  oOutMsgHead.set_cmd(101); //this is command no.
  oOutMsgHead.set_seq(GetSequence());
  oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
  
  LOG4_TRACE("send req to TestLogic, msg body serailze len: %d, in head body len: %d", 
             oOutMsgBody.ByteSize(), oOutMsgHead.msgbody_len());
  std::string strDstNodeType;
  if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) {
    LOG4_ERROR("get node type fail by cmd: %u", 101);
    return oss::STATUS_CMD_FAULT;
  }

  if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody, this)) {
    LOG4_ERROR("send data to TestLogic failed");
    return oss::STATUS_CMD_FAULT;
  }
#endif
  return oss::STATUS_CMD_RUNNING;
}

oss::E_CMD_STATUS StepTestQuery::Callback(
         const oss::tagMsgShell& stMsgShell,
         const MsgHead& oInMsgHead,
         const MsgBody& oInMsgBody,
         void* data) {
#if 1 
  loss::CJsonObject jsData;
  std::string sData = oInMsgBody.body();
  if (false == jsData.Parse(sData)) {
    sData = "http parse ret body fail";
    SendAck(sData);
  } else {
    sData= jsData("key");
    SendAck("", sData);
  }
#endif
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
