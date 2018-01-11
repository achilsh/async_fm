#include "base_timer.h"

namespace BASE_TIMER {

BaseTimer::BaseTimer(struct ev_loop* ptrLoop, ev_tstamp timerTm)
    : m_pLoop(ptrLoop), m_pTimerWatcher(NULL), m_TmoutTm(timerTm), m_Init(false) {
}

BaseTimer::~BaseTimer() {
  if (m_pTimerWatcher) {
    free (m_pTimerWatcher); m_pTimerWatcher = NULL;
  }
}

ev_tstamp BaseTimer::GetTimeout() {
  return m_TmoutTm;
}

bool BaseTimer::Init() {
  if (m_pLoop == NULL) {
    return false;
  }

  if (m_pTimerWatcher == NULL) {
    m_pTimerWatcher =  (ev_timer*)malloc(sizeof(ev_timer));
    if (m_pTimerWatcher == NULL) {
      return false;
    }
  }

  ev_timer_init(m_pTimerWatcher, BaseTimerCallBack, GetTimeout(), 0.0);
  m_Init = true;
  return true;
}

bool BaseTimer::StartTimer() {
  if (m_Init == false) {
    if (Init() == false) {
      return false;
    }
  }

  if (PreStart() == false) {
    return false;
  }
  
  m_pTimerWatcher->data = this;
  ev_timer_start(m_pLoop, m_pTimerWatcher);
  return true;
}

void BaseTimer::BaseTimerCallBack(struct ev_loop* loop, 
                                  struct ev_timer* watcher, int revents) {
  if (watcher->data == NULL) {
    return ;
  }
  //
  BaseTimer* ptrTimer = (BaseTimer*)watcher->data;
  ptrTimer->TimeOutProc();
}

//
bool BaseTimer::TimeOutProc() {
  if (this->TimeOut() == RET_TIMER_RUNNING) {
    
    ev_timer_stop(m_pLoop, m_pTimerWatcher);
    ev_timer_set(m_pTimerWatcher, GetTimeout(), 0);
    ev_timer_start(m_pLoop, m_pTimerWatcher);

  } else {
    ev_timer_stop(m_pLoop, m_pTimerWatcher);
  }
  return true;
}
////////////
}
