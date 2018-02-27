/**
 * @file: CTimer.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-13
 */
#ifndef __CTIMER_HPP_
#define __CTIMER_HPP_

#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "ev.h"         // need ev_tstamp
#include "OssDefine.hpp"
#include "labor/OssLabor.hpp"


namespace oss {

class CTimer {
  public:
   CTimer(const std::string& sTimerId, ev_tstamp dTimerTimeout = 60.0);
   virtual ~CTimer();
   
   //this interface writen by derived class 
   virtual E_CMD_STATUS TimerDoWork() = 0; 
  
   //those interface is called by client 
   bool StartTimer(OssLabor* pLabor);
  
   bool StopTimer();
   
   bool StopTimer(CTimer* pTimer);
  
   //called by framework
   E_CMD_STATUS TimerTmOut();
   
   std::string GetTimerId() const { 
     return m_sTimerId;
   }

   ev_tstamp GetTimerTm() const { 
     return m_TmOutVal;
   }
  
   void SetLogger(log4cplus::Logger* pLogger) 
   {
     m_pLogger = pLogger;
   }

   void SetLabor(OssLabor* pLabor) {
     m_pLabor = pLabor;
   }
   OssLabor* GetLabor() { 
     return m_pLabor;
   }
   ev_timer* GetWatcher() { 
     return m_pTimeoutWatcher;
   }

   void SetStarted() {
     m_IsStarted = true;
   }

   bool IsStarted() const { 
     return m_IsStarted; 
   }

   void SetTimeoutWatcher(ev_timer* pWatcher) 
   {
     m_pTimeoutWatcher = pWatcher;
   }

   log4cplus::Logger& GetLogger() 
   {
     return *m_pLogger;
   }
  private:
   //
   bool m_IsStarted;
   ev_tstamp m_TmOutVal;
   log4cplus::Logger* m_pLogger;
   std::string m_sTimerId;
   OssLabor* m_pLabor;
   ev_timer* m_pTimeoutWatcher;
};
}
#endif
