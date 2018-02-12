#ifndef STEPTESTQUERY_H 
#define STEPTESTQUERY_H
#include "step/HttpStep.hpp"

namespace im 
{
    class StepTestQuery: public oss::HttpStep 
    {
     public: 
      StepTestQuery(
          const oss::tagMsgShell& stMsgShell,
          const HttpMsg& oHttpMsg,
          const std::string& sCoName,
          const std::string& sVal,
          const int32_t httpPostTimes);
      virtual ~StepTestQuery();
	  /**
	   * @brief: CorFunc
	   *  由业务的子类来实现，
	   *  该接口已经被协程调用
	   *
	   *  协程只需在该接口内部写同步逻辑即可
	   */
	  virtual void CorFunc();

#if 0
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
#endif 
      static int m_Test;
      void SendAck(const std::string& sErr, const std::string &sData = "");
     private:
      std::string m_sKey;
      // test:  send post or get op 

      int32_t m_iPostTime;
    };
}

#endif
