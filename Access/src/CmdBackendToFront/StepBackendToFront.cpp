#include "StepBackendToFront.h"

namespace im {

  StepBackendToFront::StepBackendToFront(const oss::tagMsgShell& stMsgShell,
                                         const MsgHead& oInMsgHead,
                                         const MsgBody& oInMsgBody) 
  : oss::Step(stMsgShell, oInMsgHead, oInMsgBody) {

  }
  StepBackendToFront::~StepBackendToFront() {
  }

  oss::E_CMD_STATUS StepBackendToFront::Emit(int iErrno,
                                             const std::string& strErrMsg, 
                                             const std::string& strErrShow ) {
    return oss::STATUS_CMD_RUNNING; 
  }

  oss::E_CMD_STATUS StepBackendToFront::Callback(
      const oss::tagMsgShell& stMsgShell,
      const MsgHead& oInMsgHead,
      const MsgBody& oInMsgBody,
      void* data) {

    return oss::STATUS_CMD_COMPLETED;
  }

  oss::E_CMD_STATUS StepBackendToFront::Timeout() {
    return oss::STATUS_CMD_FAULT;
  }
  //
}
