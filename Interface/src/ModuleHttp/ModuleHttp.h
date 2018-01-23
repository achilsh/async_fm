/*******************************************************************************
 * Project:  InterfaceServer
 * @file     ModuleHttp.hpp
 * @brief 
 * @author   
 * @date:    2015Äê11ÔÂ19ÈÕ
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_MODULEHTTP_MODULEHTTP_HPP_
#define SRC_MODULEHTTP_MODULEHTTP_HPP_

#include "cmd/Module.hpp"
#include "ctimer/CTimer.hpp"

using namespace oss;

namespace im
{
    class TestTimer: public oss::CTimer {
     public:
      TestTimer(const std::string& sTimerId, 
                ev_tstamp dTimerTimeout,int x);
      virtual ~TestTimer();
      virtual oss::E_CMD_STATUS TimerDoWork(); 
     private:
      int m_X;
    };


    class StepTestQuery; 
    class ModuleHttp: public oss::Module
    {
     public:
      ModuleHttp();
      virtual ~ModuleHttp();

      virtual bool Init();
      virtual bool AnyMessage(
          const oss::tagMsgShell& stMsgShell,
          const HttpMsg& oInHttpMsg);
      void SendAck(const std::string sErr = "");
     private:
      StepTestQuery* pStepTQry;
      oss::tagMsgShell m_tagMsgShell;
      HttpMsg m_oInHttpMsg;

      TestTimer* pTimer;
      std::string m_sTimerId;
    };

} /* namespace im */

OSS_EXPORT(im::ModuleHttp)
#endif /* SRC_MODULEHELLO_MODULEHELLO_HPP_ */
