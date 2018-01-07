#include "CmdFrontToBackend.h"
#include "StepFrontToBackend.h"
#include "OssError.hpp"
#include "OssDefine.hpp"

#ifdef __cplusplus
extern "C" {
#endif
  oss::Cmd* create()
  {
    oss::Cmd* pCmd = new im::CmdFrontToBackend();
    return(pCmd);
  }
#ifdef __cplusplus
}
#endif

namespace im {
  CmdFrontToBackend::CmdFrontToBackend(): pStepFront2Backend(NULL) {

  }
  CmdFrontToBackend::~CmdFrontToBackend() {

  }
  bool CmdFrontToBackend::AnyMessage(
      const oss::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,
      const MsgBody& oInMsgBody) {
    LOG4_TRACE("call: %s", __FUNCTION__);
    
    pStepFront2Backend = new StepFrontToBackend(stMsgShell,oInMsgHead,
                                                oInMsgBody);
    if (pStepFront2Backend == NULL) {
      return false;
    }
    
    if (RegisterCallback(pStepFront2Backend) == false) {
      delete pStepFront2Backend; pStepFront2Backend = NULL;
      LOG4_ERROR("register StepFrontToBackend: %p failed", pStepFront2Backend);
      return false;
    }
    
    if (pStepFront2Backend->Emit(oss::ERR_OK) != oss::STATUS_CMD_RUNNING) {
      LOG4_ERROR("emit StepFrontToBackend: %p failed", pStepFront2Backend);
      DeleteCallback(pStepFront2Backend);
      return false;
    }
    return true;
  }
  //
}
