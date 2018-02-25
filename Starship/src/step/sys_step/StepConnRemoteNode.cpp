#include "StepConnRemoteNode.hpp"

namespace oss
{
StepConnRemoteNode::StepConnRemoteNode(const tagMsgShell& stMsgShell,
                                       const MsgHead& oInMsgHead,
                                       const MsgBody& oInMsgBody,
                                       const std::string& sCoName)
    :Step(stMsgShell, oInMsgHead, oInMsgBody, sCoName)
{
}

StepConnRemoteNode::~StepConnRemoteNode()
{
}

void StepConnRemoteNode::CorFunc()
{
    //send to req which node to connect.
    m_oReqMsgHead.set_seq(GetSequence());
    if (false == SendTo(m_stReqMsgShell, m_oReqMsgHead, m_oReqMsgBody,
                        this)) //, m_rspMsgHead, m_rspMsgBody))
    {
        LOG4_ERROR("send connect remote peer cmd fail, step: %p", this);
        return ;
    }

    OrdinaryResponse oRes;
    if (false == oRes.ParseFromString(m_rspMsgBody.body()))
    {
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
        return ;
    } 
    if (oRes.err_no() != ERR_OK)
    {
        LOG4_ERROR("error %d: %s!", oRes.err_no(), oRes.err_msg().c_str());
        return ;
    }

    //tell host node info to peer node, and get peer node info
    MsgHead      oOutMsgHead;
    MsgBody      oOutMsgBody;
    TargetWorker oTargetWorker;

    oTargetWorker.set_err_no(0);
    oTargetWorker.set_worker_identify(GetWorkerIdentify());
    oTargetWorker.set_node_type(GetNodeType());
    oTargetWorker.set_err_msg("OK");
    oOutMsgBody.set_body(oTargetWorker.SerializeAsString());
    oOutMsgHead.set_cmd(CMD_REQ_TELL_WORKER);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());

    if (false == SendTo(m_stReqMsgShell, oOutMsgHead, oOutMsgBody,this))
    {
        LOG4_ERROR("tell node info to peer node process fail, step: %p", this);
        return ;
    }


    TargetWorker oInTargetWorker;
    if (false == oInTargetWorker.ParseFromString(m_rspMsgBody.body()))
    {
        LOG4_ERROR("parse peer node workload pb fail, err: %d",
                   ERR_PARASE_PROTOBUF);
        return ;
    }

    if (oInTargetWorker.err_no() != ERR_OK)
    {
        LOG4_ERROR("error %d: %s!", oInTargetWorker.err_no(), 
                   oInTargetWorker.err_msg().c_str());
        return;
    }

    LOG4_DEBUG( "AddMsgShell(%s, fd %d, seq %llu)!",
               oInTargetWorker.worker_identify().c_str(), m_stReqMsgShell.iFd, m_stReqMsgShell.ulSeq);

    AddMsgShell(oInTargetWorker.worker_identify(), m_stReqMsgShell);
    UpDatePeerNodeInfo(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
    SendTo(m_stReqMsgShell); //send really data buf to peer node on this connection.
    return ;
}

bool StepConnRemoteNode::UpDatePeerNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo)
{
    return true;
}

//
}
