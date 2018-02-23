/*******************************************************************************
 * Project:  PluginServer
 * @file     Cmd.cpp
 * @brief 
 * @author   
 * @date:    2015年3月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "Cmd.hpp"

namespace oss
{

Cmd::Cmd()
    : m_pErrBuff(NULL), m_pLabor(0), m_pLogger(0), m_iCmd(0)
{
    m_pErrBuff = new char[gc_iErrBuffLen];
}

Cmd::~Cmd()
{
    if (m_pErrBuff != NULL)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = NULL;
    }
}

uint32 Cmd::GetNodeId()
{
    return(m_pLabor->GetNodeId());
}

uint32 Cmd::GetWorkerIndex()
{
    return(m_pLabor->GetWorkerIndex());
}

bool Cmd::RegisterCallback(Step* pStep)
{
    return(m_pLabor->RegisterCallback(pStep));
}

void Cmd::DeleteCallback(Step* pStep)
{
    m_pLabor->DeleteCallback(pStep);
}

bool Cmd::RegisterCoroutine(Step* pStep)
{
    return(m_pLabor->RegisterCoroutine(pStep));
}

void Cmd::DeleteCoroutine(Step* pStep)
{
    return(m_pLabor->DeleteCoroutine(pStep));
}

bool Cmd::Pretreat(Step* pStep)
{
    return(m_pLabor->Pretreat(pStep));
}

bool Cmd::RegisterCallback(Session* pSession)
{
    return(m_pLabor->RegisterCallback(pSession));
}

void Cmd::DeleteCallback(Session* pSession)
{
    return(m_pLabor->DeleteCallback(pSession));
}

Session* Cmd::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(uiSessionId, strSessionClass));
}

Session* Cmd::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(strSessionId, strSessionClass));
}

bool Cmd::RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep)
{
    return(GetLabor()->RegisterCallback(strRedisNodeType, pRedisStep));
}

bool Cmd::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    return(GetLabor()->RegisterCallback(strHost, iPort, pRedisStep));
}


// alarm data is json
// general format: 
// {
//   "node_type":"logic",
//   "ip": "192.168.1.1"
//   "worker_id": 1
//   "note":"上面几项不需要业务填充，接口自行获取填充"
//   "call_interface": "GetInfo()",
//   "detail": "get info fail"
// }
bool Cmd::SendBusiAlarmToManager(const loss::CJsonObject& jsReportData) {
  return GetLabor()->SendBusiAlarmToManager(jsReportData);
}

std::string Cmd::AddDetailContent(const std::string& sData, ...) {
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

} /* namespace oss */
