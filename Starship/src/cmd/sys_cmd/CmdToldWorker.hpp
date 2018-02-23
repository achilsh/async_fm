/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdToldWorker.hpp
 * @brief    被告知Worker信息
 * @author   
 * @date:    2015年8月9日
 * @note     A连接B成功后，A将己方的Worker信息A_attr（节点类型和Worker唯一标记）告知B，
 * B收到A后将A_attr登记起来，并将己方的Worker信息B_attr回复A
 * Modify history:
 ******************************************************************************/
#ifndef CMDTOLDWORKER_HPP_
#define CMDTOLDWORKER_HPP_

#include "protocol/oss_sys.pb.h"
#include "cmd/Cmd.hpp"

namespace oss
{

/**
 * @brief   被告知Worker信息
 * @author  
 * @date    2015年8月9日
 * @note    A连接B成功后，A将己方的Worker信息A_attr（节点类型和Worker唯一标记）告知B，
 * B收到A后将A_attr登记起来，并将己方的Worker信息B_attr回复A。  因为这是B对A的响应，
 * 所以无需开启Step等待A的接收结果，若A为收到B的响应，则应再告知一遍。
 */
class CmdToldWorker : public Cmd
{
public:
    CmdToldWorker();
    virtual ~CmdToldWorker();
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
    bool UpDateNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo);
};

} /* namespace oss */

#endif /* CMDTOLDWORKER_HPP_ */
