/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepTellWorker.hpp
 * @brief    告知对端己方Worker进程信息
 * @author   
 * @date:    2015年8月13日
 * @note     在作为客户端发起对一个服务端的连接，当对端返回连接成功信息时，启动
 * StepTellWorker向对端发送己方Worker信息，并等待对端回复对方Worker信息回调。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_SYS_STEP_STEPTELLWORKER_HPP_
#define SRC_STEP_SYS_STEP_STEPTELLWORKER_HPP_

#include "protocol/oss_sys.pb.h"
#include "step/Step.hpp"

namespace oss
{

class StepTellWorker : public Step
{
public:
    StepTellWorker(const tagMsgShell& stMsgShell);
    virtual ~StepTellWorker();

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
    bool UpDatePeerNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo);

private:
    int m_iTimeoutNum;          ///< 超时次数
    tagMsgShell m_stMsgShell;
};

} /* namespace oss */

#endif /* SRC_STEP_SYS_STEP_STEPTELLWORKER_HPP_ */
