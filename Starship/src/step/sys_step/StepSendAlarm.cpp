#include "StepSendAlarm.hpp"

namespace oss {
////
StepSendAlarm::StepSendAlarm(const loss::CJsonObject& jsData,const int iCmd)
    :m_SendData(jsData), m_iCmd(iCmd) {

}

StepSendAlarm::~StepSendAlarm() {
  //
}


E_CMD_STATUS StepSendAlarm::Emit(int iErrno,
                                 const std::string& strErrMsg,
                                 const std::string& strErrShow) {
  if (m_SendData.IsEmpty()) {
    return STATUS_CMD_COMPLETED;
  }

  std::string sNodeType;
  if (false == GetLabor()->QueryNodeTypeByCmd(sNodeType, m_iCmd)) {
    LOG4_ERROR("get node type failed by cmd: %u", m_iCmd);
    return STATUS_CMD_FAULT;
  } 

  //------//
  MsgBody oMsgBody;
  oMsgBody.set_body(m_SendData.ToString());
  //------//
  
  MsgHead oMsgHead;
  oMsgHead.set_cmd(m_iCmd);
  oMsgHead.set_seq(GetSequence());
  oMsgHead.set_msgbody_len(oMsgBody.ByteSize());

  if (false == SendToNext(sNodeType, oMsgHead, oMsgBody, this)) {
    LOG4_ERROR("fail send alarm msg to node: %s", sNodeType.c_str());
    return STATUS_CMD_FAULT;
  }
  return STATUS_CMD_RUNNING;
}

E_CMD_STATUS StepSendAlarm::Callback(const tagMsgShell& stMsgShell,
                                     const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,
                                     void* data) {
  return STATUS_CMD_COMPLETED;
}

E_CMD_STATUS StepSendAlarm::Timeout() {
  return(STATUS_CMD_FAULT);
}
////////////

}
