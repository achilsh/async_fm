/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdToldWorker.cpp
 * @brief 
 * @author   
 * @date:    2015年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <cmd/sys_cmd/CmdToldWorker.hpp>

namespace oss
{

CmdToldWorker::CmdToldWorker()
{
}

CmdToldWorker::~CmdToldWorker()
{
}

bool CmdToldWorker::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    bool bResult = false;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    TargetWorker oInTargetWorker;
    TargetWorker oOutTargetWorker;
    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
    oOutMsgHead.set_seq(oInMsgHead.seq());
    if (GetCmd() == (int)oInMsgHead.cmd())
    {
        if (oInTargetWorker.ParseFromString(oInMsgBody.body()))
        {
            bResult = true;
            LOG4_DEBUG("AddMsgShell(%s, fd %d, seq %llu)!",
                            oInTargetWorker.worker_identify().c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
            GetLabor()->AddMsgShell(oInTargetWorker.worker_identify(), stMsgShell);
            //GetLabor()->AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
            UpDateNodeInfo(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
            GetLabor()->AddInnerFd(stMsgShell);
            snprintf(m_pErrBuff, gc_iMaxBuffLen, "%s:%d:%d", GetLabor()->GetHostForServer().c_str(),
                            GetLabor()->GetPortForServer(), GetLabor()->GetWorkerIndex());
            oOutTargetWorker.set_err_no(0);
            oOutTargetWorker.set_worker_identify(m_pErrBuff);
            oOutTargetWorker.set_node_type(GetLabor()->GetNodeType());
            oOutTargetWorker.set_err_msg("OK");
        }
        else
        {
            bResult = false;
            oOutTargetWorker.set_err_no(ERR_PARASE_PROTOBUF);
            oOutTargetWorker.set_worker_identify("unknow");
            oOutTargetWorker.set_node_type(GetLabor()->GetNodeType());
            oOutTargetWorker.set_err_msg("WorkerLoad ParseFromString error!");
            LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
        }
    }
    else
    {
        bResult = false;
        snprintf(m_pErrBuff, gc_iMaxBuffLen, "invalid cmd %d for CmdUpdateWorkerLoad", oInMsgHead.cmd());
        LOG4_ERROR("error %d: %s!", ERR_UNKNOWN_CMD, m_pErrBuff);
        oOutTargetWorker.set_err_no(ERR_UNKNOWN_CMD);
        oOutTargetWorker.set_worker_identify("unknow");
        oOutTargetWorker.set_node_type(GetLabor()->GetNodeType());
        oOutTargetWorker.set_err_msg(m_pErrBuff);
    }
    oOutMsgBody.set_body(oOutTargetWorker.SerializeAsString());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
    return(bResult);
}

bool CmdToldWorker::UpDateNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo)
{
    return GetLabor()->UpDateNodeInfo(strNodeType,strNodeInfo);
}


} /* namespace oss */
