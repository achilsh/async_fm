#include "StepSendAlarm.hpp"

namespace oss {
////
StepSendAlarm::StepSendAlarm(const loss::CJsonObject& jsData,const int iCmd)
    :Step("send_alarm_step"), m_SendData(jsData), m_iCmd(iCmd) 
{

}

StepSendAlarm::~StepSendAlarm() 
{
  //
}

//协程专用函数数 
void StepSendAlarm::CorFunc()
{
    if (m_SendData.IsEmpty()) {
        return ;
    }

    std::string sNodeType;
    if (false == GetLabor()->QueryNodeTypeByCmd(sNodeType, m_iCmd)) 
    {
        LOG4_ERROR("get node type failed by cmd: %u", m_iCmd);
        return ;
    } 

    //------//
    MsgBody oMsgBody;
    oMsgBody.set_body(m_SendData.ToString());
    //------//

    MsgHead oMsgHead;
    oMsgHead.set_cmd(m_iCmd);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());

    MsgHead respMsgHead;
    MsgBody respInMsgBody;
    if (false == SendToNext(sNodeType, oMsgHead, oMsgBody));
    {
        LOG4_ERROR("send alarm msg to node: %s, err msg: %s", 
                   sNodeType.c_str(),
                   GetErrMsg().c_str());
        return ;
    }

    return ;
}


////////////

}
