/**
 * @file: ThriftStep.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-22
 */

#ifndef __SRC_THRIFTSTEP_HPP__
#define __SRC_THRIFTSTEP_HPP__

#include "Step.hpp"
#include "protocol/thrift2pb.pb.h"
#include "codec/ThriftCodec.hpp"

namespace oss
{

class ThriftStep: public Step
{
 
public:
    ThriftStep();
    virtual ~ThriftStep();
    
    virtual E_CMD_STATUS Callback(
        const tagMsgShell& stMsgShell,
        const Thrift2Pb& oThriftMsg,
        void* data = NULL) = 0;
    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    bool SendTo(const tagMsgShell& stMsgShell, const Thrift2Pb& oThriftMsg);
    
    template<typename T>
    bool PacketThriftData(Thrift2Pb& oInThriftMsg,const T& tData);
    
    //
    void SetThriftInterfaceName(const std::string& sName, Thrift2Pb& oInThriftMsg)
    {
        oInThriftMsg.set_thrift_interface_name(sName);
    }

    void SetThriftSeq(const int iSeq, Thrift2Pb& oInThriftMsg)
    {
        oInThriftMsg.set_thrift_seq(iSeq);
    }
public:  
    /**
     * @note Step基类的方法，ThriftStep中无须关注
     */
    virtual E_CMD_STATUS Callback(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL)
    {
        return(STATUS_CMD_COMPLETED);
    }
};
}

///////
#include "ThriftStepTemplate.hpp"
//////

#endif

