/**
 * @file: CmdBackendToFront.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x01
 * @date: 2017-12-28
 */

#ifndef __BACKENDTOFRONT_H__
#define __BACKENDTOFRONT_H__ 

#include "cmd/Cmd.hpp"

#ifdef __cplusplus
extern "C" {
#endif
  oss::Cmd* create();
#ifdef __cplusplus
}
#endif


namespace im {
  class StepBackendToFront;

  class CmdBackendToFront: public oss::Cmd {
    public:
      CmdBackendToFront();
      virtual ~CmdBackendToFront();
      //
      virtual bool AnyMessage(
          const oss::tagMsgShell& stMsgShell,
          const MsgHead& oInMsgHead,
          const MsgBody& oInMsgBody);

      StepBackendToFront* pStepB2F;
  };
  //
  //
}

#endif

