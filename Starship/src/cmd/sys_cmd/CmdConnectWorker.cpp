/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdConnectWorker.cpp
 * @brief 
 * @author   
 * @date:    2015年8月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdConnectWorker.hpp"

namespace oss
{

CmdConnectWorker::CmdConnectWorker()
    : pStepConnectWorker(NULL)
{
}

CmdConnectWorker::~CmdConnectWorker()
{
}

bool CmdConnectWorker::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    return(true);
}

bool CmdConnectWorker::Start(const tagMsgShell& stMsgShell, int iWorkerIndex)
{
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    ConnectWorker oConnWorker;
    oConnWorker.set_worker_index(iWorkerIndex);
    oMsgBody.set_body(oConnWorker.SerializeAsString());
    oMsgHead.set_cmd(CMD_REQ_CONNECT_TO_WORKER);
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    
    LOG4_DEBUG("send cmd %d.", oMsgHead.cmd());
    for (int i = 0; i < 3; ++i)
    {
        pStepConnectWorker = new StepConnRemoteNode(stMsgShell, oMsgHead, oMsgBody, "connect_step");
        if (pStepConnectWorker == NULL)
        {
            LOG4_ERROR("error %d: new StepConnectWorker() error!", ERR_NEW);
            continue;
        }
        
        if (!RegisterCoroutine(pStepConnectWorker))
        {
            DeleteCoroutine(pStepConnectWorker);
            continue;
        }
        return(true);
    }
    return(false);
}

} /* namespace oss */
