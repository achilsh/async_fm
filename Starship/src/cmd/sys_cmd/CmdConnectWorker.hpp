/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdConnectWorker.hpp
 * @brief    连接Worker进程
 * @author   
 * @date:    2015年8月6日
 * @note     CmdConnectWorker有别于其他绝大部分Cmd的是，他是命令发起者而并非命令
 * 接收者，所以一开始他的AnyMessage()的后两个参数并无输入，为简化实现，不调用
 * AnyMessage，而调用Start()。
 * Modify history:
 ******************************************************************************/
#ifndef CMDCONNECTWORKER_HPP_
#define CMDCONNECTWORKER_HPP_

#include "protocol/oss_sys.pb.h"
#include "cmd/Cmd.hpp"
#include "step/sys_step/StepConnRemoteNode.hpp"

namespace oss
{

class CmdConnectWorker : public Cmd
{
public:
    CmdConnectWorker();
    virtual ~CmdConnectWorker();
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
    bool Start(const tagMsgShell& stMsgShell, int iWorkerIndex);

public:
    StepConnRemoteNode* pStepConnectWorker;        ///< 仅为了生成可读性高的类图，构造函数中不分配空间，析构函数中也不回收空间
};

} /* namespace oss */

#endif /* CMDCONNECTWORKER_HPP_ */
