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
                unsigned int iSeq, const std::string& sName,
                const std::string& sCoName);

  virtual ~StepTestQuery();

  //采用协程模式
  void CorFunc();
  void SendAck(const std::string& sErr, const std::string &sData = "");

private:
    Test::demosvr_pingping_args m_Params;
};
///////
}
#endif
