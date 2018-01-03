#include "StepFrontToBackend.h"

namespace im {
  StepFrontToBackend::StepFrontToBackend(const oss::tagMsgShell& stMsgShell,
                                         const MsgHead& oInMsgHead,
                                         const MsgBody& oInMsgBody) 
      :oss::Step(stMsgShell, oInMsgHead, oInMsgBody) {
  }

  StepFrontToBackend::~StepFrontToBackend() {

  }

  oss::E_CMD_STATUS StepFrontToBackend::Emit(int iErrno,
                                             const std::string& strErrMsg,
                                             const std::string& strErrShow) {
    MsgHead sendHead = m_oReqMsgHead;
    sendHead.set_seq(GetSequence());
    
    std::string sserType;
    if (false  == QueryNodeTypeByCmd(sserType, m_oReqMsgHead.cmd())) {
      LOG4_ERROR("Get node type fail by cmd: %u", m_oReqMsgHead.cmd());
      return oss::STATUS_CMD_FAULT;
    }
    LOG4_TRACE("send msg head: %s\n send msg body: %s", 
               sendHead.DebugString().c_str(),
               m_oReqMsgBody.DebugString().c_str());

    if (false == SendToNext(sserType, sendHead, m_oReqMsgBody, this)) {
      LOG4_ERROR("send req service: %s failed", sserType.c_str());
      return oss::STATUS_CMD_FAULT;
    }

    return oss::STATUS_CMD_RUNNING;
  }

  oss::E_CMD_STATUS StepFrontToBackend::Callback(
      const oss::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,
      const MsgBody& oInMsgBody, void* data) {
    
    m_oReqMsgHead.set_cmd(oInMsgHead.cmd());
    m_oReqMsgHead.set_msgbody_len(oInMsgBody.ByteSize());
    Step::SendTo(m_stReqMsgShell, m_oReqMsgHead, oInMsgBody);
    LOG4_TRACE("recv msg head: %s, recv msg body: %s",
               m_oReqMsgHead.DebugString().c_str(),
               oInMsgBody.DebugString().c_str());
    return oss::STATUS_CMD_COMPLETED;
  }

  oss::E_CMD_STATUS StepFrontToBackend::Timeout() {
    LOG4_ERROR("cmd %u, seq %lu, logic timeout!", m_oReqMsgHead.cmd(), m_oReqMsgHead.seq());
    return oss::STATUS_CMD_FAULT;
  }

  bool StepFrontToBackend::QueryNodeTypeByCmd(std::string& sNodeType, const int iCmd) {
    return GetLabor()->QueryNodeTypeByCmd(sNodeType, iCmd);
  }
  /////
}
