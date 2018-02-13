#ifndef STEPTESTQUERY_H 
#define STEPTESTQUERY_H
#include "step/HttpStep.hpp"
#include "step/RedisStep.hpp"


namespace im 
{
    class StepTestQuery: public oss::HttpStep, public oss::RedisStep 
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

      static int m_Test;
      void SendAck(const std::string& sErr, const std::string &sData = "");
     private:
      std::string m_sKey;
      // test:  send post or get op 

      int32_t m_iPostTime;
    };
}

#endif
