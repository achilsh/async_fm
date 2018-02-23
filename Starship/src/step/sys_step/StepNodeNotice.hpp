/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepNodeNotice.hpp
 * @brief    处理节点注册通知
 * @author   hsc
 * @date:    2015年9月16日
 * @note      
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_SYS_STEP_NODENOTICE_HPP_
#define SRC_STEP_SYS_STEP_NODENOTICE_HPP_

#include "protocol/oss_sys.pb.h"
#include "step/Step.hpp"

namespace oss
{
class StepNodeNotice : public Step
{
public:
    StepNodeNotice(const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    const std::string& sCoName);
    virtual ~StepNodeNotice();
   
    //协程专用函数数 
    virtual void CorFunc(); 
public:
    //json数据信息
    loss::CJsonObject m_jsonData;
private:
    int m_iTimeoutNum;          ///< 超时次数
    tagMsgShell m_stMsgShell;
};

} /* namespace oss */

#endif /* SRC_STEP_SYS_STEP_STEPTELLWORKER_HPP_ */
