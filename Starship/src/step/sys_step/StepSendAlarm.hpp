/**
 * @file: StepSendAlarm.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-08
 */

#ifndef src_step_sys_step_StepSendAlarm_hpp_
#define src_step_sys_step_StepSendAlarm_hpp_

#include "step/Step.hpp"
namespace oss {

class StepSendAlarm: public Step {
 public:
  StepSendAlarm(const loss::CJsonObject& jsData,
                const int iCmd);
  virtual ~StepSendAlarm();
  //
  virtual E_CMD_STATUS Emit(
      int iErrno = 0,
      const std::string& strErrMsg = "",
      const std::string& strErrShow = "");

  virtual E_CMD_STATUS Callback(
      const tagMsgShell& stMsgShell,
      const MsgHead& oInMsgHead,
      const MsgBody& oInMsgBody,
      void* data = NULL);
  virtual E_CMD_STATUS Timeout();

 private:
  loss::CJsonObject m_SendData;
  int m_iCmd;
};

}
///////
#endif

