#include "CmdBackendToFront.h"
#include "StepBackendToFront.h"

#include "OssError.hpp"
#include "OssDefine.hpp"


#ifdef __cplusplus
extern "C" {
#endif
  oss::Cmd* create()
  {
    oss::Cmd* pCmd = new im::CmdBackendToFront();
    return(pCmd);
  }
#ifdef __cplusplus
}
#endif

namespace im {
  CmdBackendToFront::CmdBackendToFront():pStepB2F(NULL) {
  //
  }
  CmdBackendToFront::~CmdBackendToFront() {
  }
  bool CmdBackendToFront::AnyMessage(
      const oss::tagMsgShell& stMsgShell,
      const MsgHead& oInMsgHead,
      const MsgBody& oInMsgBody) {
      
    LOG4_TRACE("call: %s", __FUNCTION__);
    pStepB2F = new StepBackendToFront(stMsgShell,
                                      oInMsgHead,
                                      oInMsgBody); 
    if (pStepB2F == NULL) {
      return false;
    }

    if (RegisterCallback(pStepB2F) == false) {
      delete pStepB2F; pStepB2F = NULL;
      LOG4_ERROR("register StepBackendToFront: %p fail", pStepB2F);
      return false;
    }
    
    if (pStepB2F->Emit(oss::ERR_OK) != oss::STATUS_CMD_RUNNING) {
      LOG4_ERROR("emit StepBackendToFront: %p failed", pStepB2F);
      DeleteCallback(pStepB2F);
      return false;
    }
    return true;
  }
//
}
