/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepConnectWorker.hpp
 * @brief 
 * @author   
 * @date:    2015年8月14日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_
#define SRC_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_

#include "step/Step.hpp"
#include "StepTellWorker.hpp"

namespace oss
{

class Step;

class StepConnectWorker : public Step
{
public:
    StepConnectWorker(const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
    virtual ~StepConnectWorker();

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
    int m_iTimeoutNum;          ///< 超时次数
    tagMsgShell m_stMsgShell;
    MsgHead m_oConnectMsgHead;
    MsgBody m_oConnectMsgBody;

public:
    StepTellWorker* pStepTellWorker;        ///< 仅为了生成可读性高的类图，构造函数中不分配空间，析构函数中也不回收空间
};

} /* namespace oss */

#endif /* SRC_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_ */
