/*******************************************************************************
* * Project:  Starship
 * @file     CmdBeat.hpp
 * @brief    心跳包响应
 * @author   
 * @date:    2015年11月5日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMD_SYS_CMD_CMDBEAT_HPP_
#define SRC_CMD_SYS_CMD_CMDBEAT_HPP_

#include "cmd/Cmd.hpp"

namespace oss
{

class CmdBeat : public Cmd
{
public:
    CmdBeat();
    virtual ~CmdBeat();
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace oss */

#endif /* SRC_CMD_SYS_CMD_CMDBEAT_HPP_ */
