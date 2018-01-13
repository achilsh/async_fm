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
                    const MsgBody& oInMsgBody);
    virtual ~StepNodeNotice();

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

public:
    //json数据信息
    loss::CJsonObject m_jsonData;
private:
    int m_iTimeoutNum;          ///< 超时次数
    tagMsgShell m_stMsgShell;
};

} /* namespace oss */

#endif /* SRC_STEP_SYS_STEP_STEPTELLWORKER_HPP_ */
