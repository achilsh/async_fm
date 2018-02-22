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
                        const MsgBody& oInMsgBody,
                        const std::string& sCoName);
     virtual ~StepBackendToFront();
     virtual void CorFunc();     //协程专用函数数
  };
  //
}
#endif

