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

class ThriftStep: virtual public  Step
{
 
public:
    ThriftStep(const std::string& sCoName);

    ThriftStep(const oss::tagMsgShell& stMsgShell, 
               unsigned int iSeq, 
               const std::string& sName,
               const std::string& sCoName);
    virtual ~ThriftStep();
   
#if 0
    virtual E_CMD_STATUS Callback(
        const tagMsgShell& stMsgShell,
        const Thrift2Pb& oThriftMsg,
        void* data = NULL) = 0;
    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;
#endif
    
    // 采用协程模式
    /**
     * @brief: CorFunc
     *  由业务的子类来实现，
     *  该接口已经被协程调用
     *
     *  协程只需在该接口内部写同步逻辑即可
     */ 
    virtual void CorFunc() = 0;
public:
    /**< 向client返回thrift 数据, 子类调用 */
    template<typename T> bool SendAck(const T& tData);

    /**< 向发送client 发送回报接口 */
    bool SendTo(const Thrift2Pb& oThriftMsg);
   
    /**< 从通用pb协议中解析thrift 数据 */
    template<typename T>
    bool PacketThriftData(Thrift2Pb& oInThriftMsg,const T& tData);
    
    /**< 向thrift协议中添加 方法名 */
    void SetThriftInterfaceName(const std::string& sName, Thrift2Pb& oInThriftMsg)
    {
        oInThriftMsg.set_thrift_interface_name(sName);
    }

    /**< 向thrift协议中添加seq */
    void SetThriftSeq(const int iSeq, Thrift2Pb& oInThriftMsg)
    {
        oInThriftMsg.set_thrift_seq(iSeq);
    }
public:  

#if 0
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
#endif

protected:
    oss::tagMsgShell m_stMsgShell;  /**< client 连接句柄信息 */
    unsigned int m_iSeq;            /**< client thrift seq */
    std::string m_sName;            /**< client thrift method name */
};
}

///////
#include "ThriftStepTemplate.hpp"
//////

#endif

