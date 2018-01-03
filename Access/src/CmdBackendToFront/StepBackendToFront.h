/**
 * @file: StepBackendToFront.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x01
 * @date: 2017-12-28
 */
#ifndef __STEPBACKENDTOFRONT_H__
#define __STEPBACKENDTOFRONT_H__

#include "step/Step.hpp"
namespace im {
  class StepBackendToFront: public oss::Step {
    public:
     StepBackendToFront(const oss::tagMsgShell& stMsgShell,
                        const MsgHead& oInMsgHead,
                        const MsgBody& oInMsgBody);
     virtual ~StepBackendToFront();

     virtual oss::E_CMD_STATUS Emit(int iErrno,
                                    const std::string& strErrMsg = "", 
                                    const std::string& strErrShow = "");
     virtual oss::E_CMD_STATUS Callback(
         const oss::tagMsgShell& stMsgShell,
         const MsgHead& oInMsgHead,
         const MsgBody& oInMsgBody,
         void* data = NULL);

     virtual oss::E_CMD_STATUS Timeout();
  };
  //
}
#endif

