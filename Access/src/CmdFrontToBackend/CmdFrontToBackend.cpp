#include "CmdFrontToBackend.h"
#include "StepFrontToBackend.h"
#include "OssError.hpp"
#include "OssDefine.hpp"

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
                                                oInMsgBody, "access_co");
    if (pStepFront2Backend == NULL) {
      return false;
    }
    
    if (RegisterCoroutine(pStepFront2Backend) == false)
    {
        DeleteCoroutine(pStepFront2Backend);
        delete pStepFront2Backend;
        return false;
    }
    return true;
    
#if 0
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
#endif

    return true;
  }
  //
}
