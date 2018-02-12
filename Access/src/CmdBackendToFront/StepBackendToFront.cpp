#include "StepBackendToFront.h"

namespace im {

  StepBackendToFront::StepBackendToFront(const oss::tagMsgShell& stMsgShell,
                                         const MsgHead& oInMsgHead,
                                         const MsgBody& oInMsgBody, const std::string& sCoName) 
  : oss::Step(stMsgShell, oInMsgHead, oInMsgBody, NULL, sCoName) {

  }
  StepBackendToFront::~StepBackendToFront() {
  }

#if 0
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
#endif
  //
  void StepBackendToFront::CorFunc()
  {

  }
}
