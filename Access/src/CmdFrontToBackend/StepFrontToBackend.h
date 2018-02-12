/**
 * @file: StepFrontToBackend.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x01
 * @date: 2017-12-28
 */

#ifndef __STEPFRONTTOBACKEND_H__
#define __STEPFRONTTOBACKEND_H__

#include "step/Step.hpp"
namespace im {
  
  class StepFrontToBackend: public oss::Step {
    public:
      StepFrontToBackend(const oss::tagMsgShell& stMsgShell,
                         const MsgHead& oInMsgHead,
                         const MsgBody& oInMsgBody,
                         const std::string& sCoName);
      //
      virtual ~StepFrontToBackend();
#if 0
      //
      virtual oss::E_CMD_STATUS Emit(int iErrno,
                                     const std::string& strErrMsg = "", 
                                     const std::string& strErrShow = "");
      virtual oss::E_CMD_STATUS Callback(
          const oss::tagMsgShell& stMsgShell,
          const MsgHead& oInMsgHead,
          const MsgBody& oInMsgBody,
          void* data = NULL);

      virtual oss::E_CMD_STATUS Timeout();
#endif 

      virtual void CorFunc();     //协程专用函数数
    private:
      bool QueryNodeTypeByCmd(std::string& sNodeType,const int iCmd);
  };
  ///
}
///
#endif

