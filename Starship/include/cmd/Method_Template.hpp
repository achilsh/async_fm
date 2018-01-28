/**
 * @file: Method_Template.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-23
 */

#ifndef _TEMPLATE_CMD_METHOD_HPP_
#define _TEMPLATE_CMD_METHOD_HPP_

#include "Method.hpp"
#include "thrift/rpcpacket.h"

namespace oss 
{

template<typename T>
bool Method::PacketThriftData(Thrift2Pb& oInThriftMsg, const T& tData)
{
    bool bRet = true;
    if (oInThriftMsg.has_thrift_interface_name() == false)
    {
        LOG4_ERROR("pb not has set thrift_interface_name()");
        return false;
    }
    if (oInThriftMsg.has_thrift_seq() == false)
    {
        LOG4_ERROR("pb not has set thrift_seq()");
        return false;
    }

    const uint32_t max_sendlen = 1024*1024*2; 
    static uint8_t *buffer = new uint8_t[max_sendlen];
    if( buffer == NULL )
    {
        return false;
    }

    //::memset( buffer, 0, max_sendlen ); 
    rpcpacket data;
   
    data.m_strFunc = oInThriftMsg.thrift_interface_name();
    data.m_seqid = oInThriftMsg.thrift_seq(); 

    int32_t packlen = data.packet((uint8_t *)buffer,max_sendlen, tData);
    if(  packlen >  0)
    {
        oInThriftMsg.set_thrift_rsp_params(buffer, packlen);
    }
    else 
    {
        LOG4_ERROR("packet fail, interface: %s, seq: %u",
                   data.m_strFunc.c_str(),data.m_seqid);
        bRet = false;
    }

    return bRet;
}

template<typename T>
bool Method::GetThriftParams(T& tParam, const Thrift2Pb& oInThriftMsg,
                             uint32_t uiPacketLen, const uint8_t *pPacketBuf)
{
    if (oInThriftMsg.has_thrift_interface_name() == false)
    {
        LOG4_ERROR("pb not has set thrift_interface_name()");
        return false;
    }
    if (oInThriftMsg.has_thrift_seq() == false)
    {
        LOG4_ERROR("pb not has set thrift_seq()");
        return false;
    }
    
    TMessageType messageType;
    int32_t      seqid;
    string       strFunc;

    boost::shared_ptr<TMemoryBuffer>    transport = boost::shared_ptr<TMemoryBuffer>(new TMemoryBuffer());;
    boost::shared_ptr<TBinaryProtocol>  protocol  = boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(transport));

    try
    {
        //去掉头四个字节
        transport.get()->resetBuffer((uint8_t*)(pPacketBuf + THRIFT_TFRAME_HEAD_LEN), uiPacketLen - THRIFT_TFRAME_HEAD_LEN);
        protocol.get()->readMessageBegin(strFunc, messageType, seqid);

        tParam.read(protocol.get());
        protocol.get()->readMessageEnd();
        protocol.get()->getTransport()->readEnd();
    }
    catch(exception &e)
    {
        LOG4_ERROR("upackt len(%d) catch exception", uiPacketLen);
        return false;
    } 

    return true;
}

template<typename T> bool Method::SendAck(const tagMsgShell& stMsgShell,
                                    Thrift2Pb& oInThriftMsg, const T& tRet) 
{
    if (false == PacketThriftData(oInThriftMsg,tRet))
    {
        LOG4_ERROR("packet response faild");
        return false;
    }
    return (GetLabor()->SendTo(stMsgShell, oInThriftMsg));
}


//----------------------//
}

#endif

