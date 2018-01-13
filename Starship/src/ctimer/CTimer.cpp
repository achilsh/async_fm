#include "CTimer.hpp"

namespace oss  {

CTimer::CTimer(const std::string& sTimerId, ev_tstamp dTimerTimeout) 
    : m_IsStarted(false), m_TmOutVal(dTimerTimeout),
    m_pLogger(NULL), m_sTimerId(sTimerId), m_pLabor(NULL), 
    m_pTimeoutWatcher(NULL){
}

CTimer::~CTimer() {
  if (m_pTimeoutWatcher) {
    free(m_pTimeoutWatcher);
    m_pTimeoutWatcher = NULL;
  }
  m_IsStarted = false;
  m_pLogger = NULL;
  m_pLabor = NULL;
}

oss::E_CMD_STATUS CTimer::TimerTmOut() {
  this->TimerDoWork();
  return oss::STATUS_CMD_RUNNING;
}

bool CTimer::StartTimer(OssLabor* pLabor) {
  if (pLabor == NULL) {
    return false;
  }

  SetLabor(pLabor);
  
  if (pLabor->RegisterCallback(this) == true) {
    SetStarted();
    return true;
  }
  return false;
}

bool CTimer::StopTimer() {
  LOG4_TRACE("del timer: %s", m_sTimerId.c_str());
  return  m_pLabor->DeleteCallback(this);
}

bool CTimer::StopTimer(CTimer* pTimer) {
  return m_pLabor->DeleteCallback(pTimer);
}

}
