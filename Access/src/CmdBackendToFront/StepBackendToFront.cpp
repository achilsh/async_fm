#include "StepBackendToFront.h"

namespace im {

  StepBackendToFront::StepBackendToFront(const oss::tagMsgShell& stMsgShell,
                                         const MsgHead& oInMsgHead,
                                         const MsgBody& oInMsgBody, const std::string& sCoName) 
  : oss::Step(stMsgShell, oInMsgHead, oInMsgBody, sCoName) {

  }
  StepBackendToFront::~StepBackendToFront() {
  }

  //
  void StepBackendToFront::CorFunc()
  {

  }
}
