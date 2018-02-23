#include "StepTestQuery.h"
#include "thrift_serialize.h"
//
#include "hello_test_types.h"

using namespace oss;
using namespace Test; 
using namespace loss;

namespace im 
{

int StepTestQuery::m_Test = 0;

StepTestQuery::StepTestQuery(const oss::tagMsgShell& stMsgShell,
                            const HttpMsg& oHttpMsg,
                            const std::string& sCoName,
                            const std::string& sVal,
                            const int32_t httpPostTimes)
    :oss::Step(sCoName), oss::HttpStep(stMsgShell, oHttpMsg, sCoName), oss::RedisStep(sCoName),
    m_sKey(sVal), m_iPostTime(httpPostTimes)
     
{
}

//
StepTestQuery::~StepTestQuery() 
{

}

void StepTestQuery::SendAck(const std::string& sErr, const std::string &sEData) 
{
    oss::HttpStep::SendAck(sErr, sEData);
}

void StepTestQuery::CorFunc()
{
    MsgBody oOutMsgBody;
    MsgHead oOutMsgHead;
    LOG4_TRACE("===== 54 again =====");

    OneTest t;
    t.__set_fOne(m_sKey); 
    std::string sData;
    ThrifSerialize<OneTest>::ToString(t,sData);

    oOutMsgBody.set_body(sData);
    oOutMsgBody.set_session(m_sKey);

    oOutMsgHead.set_cmd(101); //this is command no.
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());

    LOG4_TRACE("send req to TestLogic, msg body serailze len: %d, in head body len: %d", 
               oOutMsgBody.ByteSize(), oOutMsgHead.msgbody_len());

    std::string strDstNodeType;
    if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) 
    {
        LOG4_ERROR("get node type fail by cmd: %u", 101);
        LOG4_ALARM_REPORT("not get node type for cmd: %u", 101);

        sData = "get node type fail by cm";
        LOG4_ALARM_REPORT("serialize fail");
        SendAck(sData);
        return ;
    }

    //test session id;
    SetId(m_sKey);

    int iTestRepeatTimes = 1;
    while (iTestRepeatTimes-- > 0) 
    {
        //tcp coroutine test
        if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody))
        {
            LOG4_ERROR("send data to TestLogic failed");
            LOG4_ALARM_REPORT("send to next fail");

            sData = "send data to TestLogic failed";
            SendAck(sData);
            return ;
        }
        LOG4_TRACE("coroutine send rpc req times: %d", iTestRepeatTimes);

        //http coroutine post test
        if (m_iPostTime > 0)
        {
            std::string sUrl = "http://192.168.1.106:25000/im/hello"; 
            std::map<std::string, std::string> mpHead;
            std::string sBody = "{\"Name\":\"ff\"}";
            if (false == HttpPost(sUrl, sBody, mpHead))
            {
                LOG4_ERROR("send http req fail");
            }

            if (m_msgRespHttp.has_body())
            {
                LOG4_TRACE("test recv http body: %s", m_msgRespHttp.body().c_str());
            }
        }

        //redis coroutine  write test 
        loss::RedisCmd* redisCmd = RedisCmd();
        if (NULL == redisCmd)
        {
            LOG4_ERROR("get redis cmd obj fail");
            return ;
        }

        redisCmd->SetHost("127.0.0.1");
        redisCmd->SetPort(6379);

        redisCmd->SetCmd("set");
        std::string sRedisKey = "RedisTest:" + m_sKey;
        redisCmd->Append(sRedisKey);
        redisCmd->Append("yes i am redis coroutine test");

        OssReply* redisRet = NULL; 
        if (false == ExecuteRedisCmd(redisRet))
        {
            LOG4_ERROR("excuate redis cmd fail, err: %s", GetErrMsg().c_str()); 
            sData = GetErrMsg();
            SendAck(sData);
            return ;
        }
        if (redisRet == NULL) 
        {
            LOG4_ERROR("redis ret is null");
            sData = "redis ret is null";
            SendAck(sData);
            return ;
        }

        LOG4_TRACE("ret type: %d", redisRet->Type());
        if (redisRet->Type() == INTEGER)
        {
            LOG4_TRACE("set ret: %ld", redisRet->Integer());
        }

        //redis coroutine read test 
    }

    OneTest tt;

    sData = m_rspMsgBody.body();
    if (0 != ThrifSerialize<OneTest>::FromString(sData,tt)) 
    {
        sData = "http parse ret body fail";
        LOG4_ALARM_REPORT("serialize fail");
        SendAck(sData);
    } 
    else  
    {
        sData = tt.fOne;
        SendAck("", sData);
    }
    return ;
}

//
}
