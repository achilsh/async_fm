/*******************************************************************************
 * Project:  AsyncServer
 * @file     Step.cpp
 * @brief 
 * @author   
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/

#include "hiredis/adapters/libev.h"
#include "Step.hpp"

namespace oss
{

Step::Step(const std::string& sCoName)
    : CoStep(sCoName), m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0), m_oSessionId(""),
      m_bCoRespStatus(true)
{
}

Step::Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody,
            const std::string& sCoName)
    : CoStep(sCoName), m_stReqMsgShell(stReqMsgShell), m_oReqMsgHead(oReqMsgHead), m_oReqMsgBody(oReqMsgBody),
      m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0), m_oSessionId("")
{
}

Step::~Step()
{
    if (m_pTimeoutWatcher != 0)
    {
        free(m_pTimeoutWatcher);
        m_pTimeoutWatcher = 0;
    }
}

bool Step::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    return(m_pLabor->RegisterCallback(pStep, dTimeout));
}

void Step::DeleteCallback(Step* pStep)
{
    return(m_pLabor->DeleteCallback(pStep));
}

bool Step::Pretreat(Step* pStep)
{
    return(m_pLabor->Pretreat(pStep));
}

bool Step::RegisterCallback(Session* pSession)
{
    return(m_pLabor->RegisterCallback(pSession));
}

void Step::DeleteCallback(Session* pSession)
{
    return(m_pLabor->DeleteCallback(pSession));
}

uint32 Step::GetNodeId()
{
    return(m_pLabor->GetNodeId());
}

uint32 Step::GetWorkerIndex()
{
    return(m_pLabor->GetWorkerIndex());
}

const std::string& Step::GetWorkerIdentify()
{
    if (m_strWorkerIdentify.size() < 5) // IP + port + worker_index长度一定会大于这个数即可，不在乎数值是什么
    {
        char szWorkerIdentify[64] = {0};
        snprintf(szWorkerIdentify, 64, "%s:%d:%d", m_pLabor->GetHostForServer().c_str(),
                        m_pLabor->GetPortForServer(), m_pLabor->GetWorkerIndex());
        m_strWorkerIdentify = szWorkerIdentify;
    }
    return(m_strWorkerIdentify);
}

const std::string& Step::GetNodeType() const
{
    return(m_pLabor->GetNodeType());
}

Session* Step::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(uiSessionId, strSessionClass));
}

Session* Step::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(strSessionId, strSessionClass));
}

bool Step::SendTo(const tagMsgShell& stMsgShell)
{
    return(m_pLabor->SendTo(stMsgShell));
}

bool Step::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendTo(stMsgShell, oMsgHead, oMsgBody));
}

//用于s-s 间连接建立过程的发送流程
bool Step::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody,
                  oss::Step* pStep)
{
    return  m_pLabor->SendTo(stMsgShell, oMsgHead, oMsgBody, pStep);
}

bool Step::SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendTo(strIdentify, oMsgHead, oMsgBody));
}

bool Step::SendToNext(const std::string& strNodeType,  MsgHead& oMsgHead,  MsgBody& oMsgBody)
{

#if 0 
    if (m_oSessionId.empty()) {
        if (m_oReqMsgBody.has_session()) {
            m_oSessionId = m_oReqMsgBody.session();
        } else {
            m_oSessionId = SESSION_ID_DEF;
        }
    }

    oMsgBody.set_session(m_oSessionId);
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    return(m_pLabor->SendToNext(strNodeType, oMsgHead, oMsgBody, this, rspMsgHead, rspMsgBody));
#else
    return  m_pLabor->SendToNext(strNodeType, oMsgHead, oMsgBody, this); //, rspMsgHead, rspMsgBody);
#endif

}

bool Step::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendToWithMod(strNodeType, uiModFactor, oMsgHead, oMsgBody));
}

uint32 Step::GetSequence()
{
    if (!m_bRegistered)
    {
        return(0);
    }
    if (0 == m_ulSequence)
    {
        if (NULL != m_pLabor)
        {
            m_ulSequence = m_pLabor->GetSequence();
        }
    }
    return(m_ulSequence);
}

bool Step::AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell)
{
	return(m_pLabor->AddMsgShell(strIdentify, stMsgShell));
}

void Step::DelMsgShell(const std::string& strIdentify)
{
    m_pLabor->DelMsgShell(strIdentify);
}

void Step::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pLabor->AddNodeIdentify(strNodeType, strIdentify);
}

void Step::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pLabor->DelNodeIdentify(strNodeType, strIdentify);
}

/*
void Step::AddRedisNodeConf(const std::string strNodeType, const std::string strHost, int iPort)
{
    GetLabor()->AddRedisNodeConf(strNodeType, strHost, iPort);
}

void Step::DelRedisNodeConf(const std::string strNodeType, const std::string strHost, int iPort)
{
    GetLabor()->DelRedisNodeConf(strNodeType, strHost, iPort);
}
*/

bool Step::AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
{
    return(GetLabor()->AddRedisContextAddr(strHost, iPort, ctx));
}

void Step::DelRedisContextAddr(const redisAsyncContext* ctx)
{
    GetLabor()->DelRedisContextAddr(ctx);
}

bool Step::RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep)
{
    return(GetLabor()->RegisterCallback(strIdentify, pRedisStep));
}

bool Step::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    return(GetLabor()->RegisterCallback(strHost, iPort, pRedisStep));
}

// alarm data is json
// general format: 
// {
//   "node_type":      "logic",
//   "ip":             "192.168.1.1"
//   "worker_id":      1
//   "note":"上面几项不需要业务填充，接口自行获取填充"
//   "call_interface": "GetInfo()",
//   "file_name":      "test.cpp",
//   "line":           123,
//   "time":           "2018-1-1 12:00:00,314"
//   "detail":         "get info fail"
// }
bool Step::SendBusiAlarmToManager(const loss::CJsonObject& jsReportData) {
  return GetLabor()->SendBusiAlarmToManager(jsReportData);
}

std::string Step::AddDetailContent(const std::string& sData, ...) {
  va_list va;
  va_start(va,sData);
  
  std::string retS;
  char buf[1024] = {0};
  int iLen = vsnprintf(buf, sizeof(buf), sData.c_str(), va);
  if (iLen < 0) {
    va_end(va);
    return retS;
  }

  va_end(va);
  retS.assign(buf, iLen);
}

void Step::SetId(const std::string& sId) {
    m_oSessionId = sId;
}


void Step::AfterFuncWork()
{
    GetLabor()->DeleteCoroutine(this);
}


bool Step::SetRespStatus(bool bStatus)
{
    m_bCoRespStatus = bStatus;
}

bool Step::GetRespStatus()
{
    return m_bCoRespStatus;
}

//获取异步回调的返回结果
MsgHead& Step::GetRespHead()
{
    return  m_rspMsgHead;
}

MsgBody& Step::GetRespBody()
{
    return m_rspMsgBody;
}

//当异步返回时，设置返回结果的头部信息和body信息
void Step::SetRespHead(MsgHead* rspMsgHead)
{
    if (rspMsgHead)
    {
        m_rspMsgHead.Clear();
        m_rspMsgHead.Swap(rspMsgHead);
    }
}

void Step::SetRespBody(MsgBody* rspMsgBody)
{
    if (rspMsgBody) 
    {
        m_rspMsgBody.Clear();
        m_rspMsgBody.Swap(rspMsgBody);
    }
}

//
} /* namespace oss */
