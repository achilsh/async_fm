#include "StepTestQuery.h"
#include "hello_test_types.h"
#include "thrift_util/thrift_serialize.h"

using namespace  oss;
using namespace Test;

namespace im
{

StepTestQuery::StepTestQuery(const oss::tagMsgShell& stMsgShell,
                             const demosvr_pingping_args& pingping_args,
                             unsigned int iSeq, const std::string& sName,
                             const std::string& sCoName)
    :ThriftStep(stMsgShell, iSeq, sName, sCoName),m_Params(pingping_args)
{
}

StepTestQuery::~StepTestQuery()
{
}

//采用协程模式
void StepTestQuery::CorFunc()
{
    MsgBody oOutMsgBody;
    MsgHead oOutMsgHead;

    OneTest t;
    t.__set_fOne(m_Params.pi.b);
    
    std::string sData;
    ThrifSerialize<OneTest>::ToString(t,sData);
    oOutMsgBody.set_body(sData);
    oOutMsgBody.set_session(m_Params.pi.b);

    oOutMsgHead.set_cmd(101); //this is command no.
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());

    std::string strDstNodeType;
    if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) 
    {
        LOG4_ERROR("get node type fail by cmd: %u", 101);
        LOG4_ALARM_REPORT("not get node type for cmd: %u", 101);
        return ;
    }

    if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody)) 
    {
        LOG4_ERROR("send data to TestLogic failed");
        LOG4_ALARM_REPORT("send to next fail");
        SendAck("SendToNext fail");
        return ;
    }

    OneTest tRetData;
    std::string sRetData = m_rspMsgBody.body();

    if (0 != ThrifSerialize<OneTest>::FromString(sRetData,tRetData)) 
    {
        sData = "thrift parse ret body fail";
        LOG4_ALARM_REPORT("serialize fail");
        SendAck(sData);
    } else 
    {
        sData = t.fOne;
        SendAck("", tRetData.fOne);
    }
    return ;
}

#if 0
oss::E_CMD_STATUS StepTestQuery::Timeout()
{
    LOG4_ERROR("StepTestQuery tm out ");
    LOG4_ALARM_REPORT("resp tm out, key: %s", m_Params.pi.b.c_str());
    SendAck("step Test query time out");
    return  oss::STATUS_CMD_FAULT;
}

oss::E_CMD_STATUS StepTestQuery::Emit(int err,
                                      const std::string& strErrMsg ,
                                      const std::string& strErrShow)
{
    MsgBody oOutMsgBody;
    MsgHead oOutMsgHead;

    OneTest t;
    t.__set_fOne(m_Params.pi.b);
    std::string sData;
    ThrifSerialize<OneTest>::ToString(t,sData);
    oOutMsgBody.set_body(sData);
    oOutMsgBody.set_session(m_Params.pi.b);

    oOutMsgHead.set_cmd(101); //this is command no.
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());

    std::string strDstNodeType;
    if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) {
        LOG4_ERROR("get node type fail by cmd: %u", 101);
        LOG4_ALARM_REPORT("not get node type for cmd: %u", 101);
        return oss::STATUS_CMD_FAULT;
    }

    //SetId(m_Params.pi.b);
    if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody, this)) {
        LOG4_ERROR("send data to TestLogic failed");
        LOG4_ALARM_REPORT("send to next fail");
        SendAck("SendToNext fail");
        return oss::STATUS_CMD_FAULT;
    }

    return oss::STATUS_CMD_RUNNING;
}

oss::E_CMD_STATUS StepTestQuery::Callback(const oss::tagMsgShell& stMsgShell,
    const MsgHead& oInMsgHead,const MsgBody& oInMsgBody, void* data)
{
    OneTest t;
    std::string sData = oInMsgBody.body();
    if (0 != ThrifSerialize<OneTest>::FromString(sData,t)) 
    {
        sData = "thrift parse ret body fail";
        LOG4_ALARM_REPORT("serialize fail");
        SendAck(sData);
    } else 
    {
        sData = t.fOne;
        SendAck("", t.fOne);
    }
    return oss::STATUS_CMD_COMPLETED;
}
#endif


void StepTestQuery::SendAck(const std::string& sErr, const std::string &sData)
{
    demosvr_pingping_result  pingping_result;
    pingping_result.__isset.success = true;
    
    if (sErr.empty()) 
    {
        pingping_result.success.retcode = 0;
        pingping_result.success.a = 2222;
        pingping_result.success.b = sData;
    } 
    else
    {
        pingping_result.success.a = 122222 ;
        pingping_result.success.b = sErr;
        pingping_result.success.retcode  = -1;
    }

    ThriftStep::SendAck(pingping_result);
}

//////
}
