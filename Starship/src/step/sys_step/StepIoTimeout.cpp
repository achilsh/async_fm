/*******************************************************************************
 * Project:  Starship
 * @file     StepIoTimeout.cpp
 * @brief 
 * @author   
 * @date:    2015年10月31日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepIoTimeout.hpp"

namespace oss
{

StepIoTimeout::StepIoTimeout(const tagMsgShell& stMsgShell, struct ev_timer* pWatcher, const std::string& sCoName)
    :Step(sCoName), m_stMsgShell(stMsgShell), watcher(pWatcher)
{
}

StepIoTimeout::~StepIoTimeout()
{
}

#if 0
E_CMD_STATUS StepIoTimeout::Emit(
                int iErrno,
                const std::string& strErrMsg,
                const std::string& strErrShow)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    oOutMsgHead.set_cmd(CMD_REQ_BEAT);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(0);
    if (SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody))
    {
        return(STATUS_CMD_RUNNING);
    }
    else        // SendTo错误会触发断开连接和回收资源
    {
        return(STATUS_CMD_FAULT);
    }
}

E_CMD_STATUS StepIoTimeout::Callback(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody,
                void* data)
{
    GetLabor()->IoTimeout(watcher, true);
    return(STATUS_CMD_COMPLETED);
}

E_CMD_STATUS StepIoTimeout::Timeout()
{
    GetLabor()->IoTimeout(watcher, false);
    return(STATUS_CMD_FAULT);
}

#endif 

//协程专用函数数 
void StepIoTimeout::CorFunc()
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;

    oOutMsgHead.set_cmd(CMD_REQ_BEAT);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(0);

    if (SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody,this) == false)
    {
        LOG4_ERROR("send io time out ping err");
        return ;
    }

    LOG4_TRACE("check step io timeout running alway");
    GetLabor()->IoTimeout(watcher, true);
}

} /* namespace oss */
