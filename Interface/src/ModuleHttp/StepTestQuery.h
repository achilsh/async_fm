#ifndef STEPTESTQUERY_H 
#define STEPTESTQUERY_H
#include "step/HttpStep.hpp"

namespace im {
  class StepTestQuery: public oss::HttpStep {
    public: 
     StepTestQuery(
         const oss::tagMsgShell& stMsgShell,
         const HttpMsg& oHttpMsg,
         const std::string& sVal);
     virtual ~StepTestQuery();

     virtual oss::E_CMD_STATUS Timeout();
     virtual  oss::E_CMD_STATUS Emit(int err, 
                                     const std::string& strErrMsg = "",
                                     const std::string& strErrShow = "");
     virtual oss::E_CMD_STATUS Callback(
         const oss::tagMsgShell& stMsgShell,
         const MsgHead& oInMsgHead,
         const MsgBody& oInMsgBody,
         void* data = NULL);

     virtual oss::E_CMD_STATUS Callback(
         const oss::tagMsgShell& stMsgShell,
         const HttpMsg& oHttpMsg,
         void* data = NULL)
     {
       return oss::STATUS_CMD_COMPLETED;
     }

     void SendAck(const std::string& sErr, const std::string &sData = "");
    private:
     oss::tagMsgShell m_stMsgShell;
     HttpMsg m_oHttpMsg;
     std::string m_sKey;
  };
}
#endif
