/**
 * @file: CmdFrontToBackend.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x01
 * @date: 2017-12-28
 */

#ifndef __CMDFRONTTOBACKEND_H__
#define __cmdfronttobackend_h__ 

#include "cmd/Cmd.hpp"

namespace im {
  
  class StepFrontToBackend;
  class CmdFrontToBackend : public oss::Cmd {
    public:
      CmdFrontToBackend();
      virtual ~CmdFrontToBackend();

      virtual bool AnyMessage(
          const oss::tagMsgShell& stMsgShell,
          const MsgHead& oInMsgHead,
          const MsgBody& oInMsgBody);
      //
      StepFrontToBackend* pStepFront2Backend;
  };
  ///
}

OSS_EXPORT(im::CmdFrontToBackend);
#endif

