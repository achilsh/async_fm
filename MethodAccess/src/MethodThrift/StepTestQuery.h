/**
 * @file: StepTestQuery.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-23
 */
#ifndef _method_query_test_step_
#define _method_query_test_step_

#include "step/ThriftStep.hpp"
#include "demosvr.h"

namespace im 
{

class StepTestQuery: public oss::ThriftStep
{
public:
  StepTestQuery(const oss::tagMsgShell& stMsgShell,
                const Test::demosvr_pingping_args& pingping_args,
                unsigned int iSeq, const std::string& sName);
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
      const Thrift2Pb& oThriftMsg,
      void* data) 
  {
      return oss::STATUS_CMD_COMPLETED;
  }
  void SendAck(const std::string& sErr, const std::string &sData = "");
private:
    oss::tagMsgShell m_stMsgShell;
    Test::demosvr_pingping_args m_Params;
    unsigned int m_iSeq;
    std::string m_sName;
};
///////
}
#endif
