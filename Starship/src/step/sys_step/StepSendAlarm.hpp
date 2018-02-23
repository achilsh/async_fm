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

class StepSendAlarm: public Step 
{
 public:
  StepSendAlarm(const loss::CJsonObject& jsData,const int iCmd);
  virtual ~StepSendAlarm();
  
  //协程专用函数数 
  virtual void CorFunc(); 
 private:
  loss::CJsonObject m_SendData;
  int m_iCmd;
};

}
///////
#endif

