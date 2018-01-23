/**
 * @file: MethodThrift.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-23
 */

#ifndef SRC_METHODTHRIFT_TEST_H_
#define SRC_METHODTHRIFT_TEST_H_

#include "cmd/Method.hpp"

using namespace oss;

namespace im 
{
class StepTestQuery;
class MethodThrift: public oss::Method
{
    public:
     MethodThrift();
     virtual ~MethodThrift();
     
     virtual bool AnyMessage(
         const tagMsgShell& stMsgShell,
         const Thrift2Pb& oInThriftMsg);

    private:
     template<typename T>
     bool SendAck(const tagMsgShell& stMsgShell, 
                  Thrift2Pb& oInThriftMsg, const T& tRet);
    private:
     StepTestQuery* pStepTQry;
};

//

template<typename T>
bool MethodThrift::SendAck(const tagMsgShell& stMsgShell, 
                           Thrift2Pb& oInThriftMsg, const T& tRet)
{
    if (false == PacketThriftData(oInThriftMsg,tRet))
    {
        LOG4_ERROR("packet response faild");
        return false;
    }
    return (GetLabor()->SendTo(stMsgShell, oInThriftMsg));
}


OSS_EXPORT(im::MethodThrift);
////
}
#endif

