#include "StepTestQuery.h"
#include "hello_test_types.h"
#include "thrift_util/thrift_serialize.h"

using namespace  oss;
using namespace Test;

namespace im
{

StepTestQuery::StepTestQuery(const oss::tagMsgShell& stMsgShell,
                             const demosvr_pingping_args& pingping_args,
                             unsigned int iSeq, const std::string& sName)
    :m_stMsgShell(stMsgShell),m_Params(pingping_args), m_iSeq(iSeq), m_sName(sName)
{
}

StepTestQuery::~StepTestQuery()
{
}

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

    oOutMsgHead.set_cmd(101); //this is command no.
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());

    std::string strDstNodeType;
    if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) {
        LOG4_ERROR("get node type fail by cmd: %u", 101);
        LOG4_ALARM_REPORT("not get node type for cmd: %u", 101);
        return oss::STATUS_CMD_FAULT;
    }

    SetId(m_Params.pi.b);
    if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody, this)) {
        LOG4_ERROR("send data to TestLogic failed");
        LOG4_ALARM_REPORT("send to next fail");
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
        sData = "http parse ret body fail";
        LOG4_ALARM_REPORT("serialize fail");
        SendAck(sData);
    } else  {
        sData = t.fOne;
        SendAck("", sData);
    }
    return oss::STATUS_CMD_COMPLETED;
}

void StepTestQuery::SendAck(const std::string& sErr, const std::string &sData)
{
    Thrift2Pb outThriftMsg;
    outThriftMsg.Clear();
    
    demosvr_pingping_result  pingping_result;
    pingping_result.__isset.success = true;
    
    if (sErr.empty()) 
    {
        pingping_result.success.retcode = -1;
    } 
    else
    {
        pingping_result.success.a = 122222 ;
        pingping_result.success.b = "is ok";
        pingping_result.success.retcode  = 0;
    }

    SetThriftSeq(m_iSeq, outThriftMsg);
    SetThriftInterfaceName(m_sName, outThriftMsg);

    if (false == PacketThriftData(outThriftMsg, pingping_result))
    {
        LOG4_ERROR("packet response faild");
        return ;
    }
    (GetLabor()->SendTo(m_stMsgShell, outThriftMsg));
}

//////
}
