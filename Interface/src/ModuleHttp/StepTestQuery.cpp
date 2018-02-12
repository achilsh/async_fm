#include "StepTestQuery.h"
#include "thrift_serialize.h"
//
#include "hello_test_types.h"
using namespace Test; 

namespace im 
{

int StepTestQuery::m_Test = 0;

StepTestQuery::StepTestQuery(const oss::tagMsgShell& stMsgShell,
                            const HttpMsg& oHttpMsg,
                            const std::string& sCoName,
                            const std::string& sVal,
                            const int32_t httpPostTimes)
    :oss::HttpStep(stMsgShell, oHttpMsg, sCoName), m_sKey(sVal),
    m_iPostTime(httpPostTimes)
{
}

//
StepTestQuery::~StepTestQuery() 
{

}

#if 0
oss::E_CMD_STATUS StepTestQuery::Timeout() 
{
  LOG4_ERROR("StepTestQuery tm out ");
  LOG4_ALARM_REPORT("resp tm out, key: %s", m_sKey.c_str());
  SendAck("step Test query time out");
  return  oss::STATUS_CMD_FAULT;
}
//
oss::E_CMD_STATUS StepTestQuery::Emit(int err, 
                       const std::string& strErrMsg ,
                       const std::string& strErrShow ) 
{
  MsgBody oOutMsgBody;
  MsgHead oOutMsgHead;
  LOG4_TRACE("===== 54 again =====");
  /***
  loss::CJsonObject tData; 
  tData.Add("key",m_sKey);
  oOutMsgBody.set_body(tData.ToString());
  ***/
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
  if (false == GetLabor()->QueryNodeTypeByCmd(strDstNodeType, 101)) {
    LOG4_ERROR("get node type fail by cmd: %u", 101);
    LOG4_ALARM_REPORT("not get node type for cmd: %u", 101);
    return oss::STATUS_CMD_FAULT;
  }

  //test session id;
  SetId(m_sKey);

  if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody, this)) {
    LOG4_ERROR("send data to TestLogic failed");
    LOG4_ALARM_REPORT("send to next fail");
    
    return oss::STATUS_CMD_FAULT;
  }

  return oss::STATUS_CMD_RUNNING;
}

oss::E_CMD_STATUS StepTestQuery::Callback(
         const oss::tagMsgShell& stMsgShell,
         const MsgHead& oInMsgHead,
         const MsgBody& oInMsgBody,
         void* data) 
{
  /*****
  loss::CJsonObject jsData;
  std::string sData = oInMsgBody.body();
  if (false == jsData.Parse(sData)) {
    sData = "http parse ret body fail";
    SendAck(sData);
  } else {
    sData= jsData("key");
    SendAck("", sData);
  }
  ****/
  OneTest t;
  std::string sData = oInMsgBody.body();
  if (0 != ThrifSerialize<OneTest>::FromString(sData,t)) 
  {
    sData = "http parse ret body fail";
    LOG4_ALARM_REPORT("serialize fail");
    SendAck(sData);
  } 
  else  
  {
    sData = t.fOne;
    SendAck("", sData);
  }
  return oss::STATUS_CMD_COMPLETED;
}

#endif

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
        if (false == SendToNext(strDstNodeType, oOutMsgHead, oOutMsgBody))
        {
            LOG4_ERROR("send data to TestLogic failed");
            LOG4_ALARM_REPORT("send to next fail");

            sData = "send data to TestLogic failed";
            LOG4_ALARM_REPORT("serialize fail");
            SendAck(sData);
            return ;
        }
        LOG4_TRACE("coroutine send rpc req times: %d", iTestRepeatTimes);

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
