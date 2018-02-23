/*******************************************************************************
 * Project:  AsyncServer
 * @file     Session.cpp
 * @brief 
 * @author   
 * @date:    2015年7月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "Session.hpp"

namespace oss
{

Session::Session(uint32 ulSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : m_bRegistered(false), m_dSessionTimeout(dSessionTimeout), m_activeTime(0),
      m_strSessionClassName(strSessionClass), m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0)
{
    char szSessionId[16] = {0};
    snprintf(szSessionId, sizeof(szSessionId), "%u", ulSessionId);
    m_strSessionId = szSessionId;
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : m_bRegistered(false), m_dSessionTimeout(dSessionTimeout), m_activeTime(0),
      m_strSessionId(strSessionId), m_strSessionClassName(strSessionClass), m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0)
{
}

Session::~Session()
{
    if (m_pTimeoutWatcher != 0)
    {
        free(m_pTimeoutWatcher);
        m_pTimeoutWatcher = 0;
    }
}

bool Session::RegisterCallback(Session* pSession)
{
    return(m_pLabor->RegisterCallback(pSession));
}

void Session::DeleteCallback(Session* pSession)
{
    m_pLabor->DeleteCallback(pSession);
}

bool Session::Pretreat(Step* pStep)
{
    return(m_pLabor->Pretreat(pStep));
}


} /* namespace oss */
