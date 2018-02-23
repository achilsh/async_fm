/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdNodeNotice.hpp
 * @brief    注册通知
 * @author   HSC
 * @date:    2015年8月9日
 * @note     节点注册通知
 * Modify history:
 ******************************************************************************/
#ifndef CMD_NODE_NOTICE_HPP_
#define CMD_NODE_NOTICE_HPP_

#include "protocol/oss_sys.pb.h"
#include "cmd/Cmd.hpp"

namespace oss
{
/**
 * @brief   节点注册
 * @author  hsc
 * @date    2015年8月9日
 * @note    各个模块启动时需要向CENTER进行注册
 */
class CmdNodeNotice : public Cmd
{
public:
    CmdNodeNotice();
    virtual ~CmdNodeNotice();
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace oss */

#endif /* CMD_NODE_NOTICE_HPP_ */
