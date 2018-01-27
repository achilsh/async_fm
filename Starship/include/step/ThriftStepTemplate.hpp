/**
 * @file: ThriftStepTemplate.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-23
 */

#ifndef _THRIFTSTEP_TEMPLATE_HPP_
#define _THRIFTSTEP_TEMPLATE_HPP_

#include "ThriftStep.hpp"
#include "thrift/rpcpacket.h"

namespace oss
{

template<typename T>
bool ThriftStep::PacketThriftData(Thrift2Pb& oInThriftMsg, const T& tData)
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
    uint8_t *buffer = new uint8_t[max_sendlen];
    if( buffer == NULL )
    {
        return false;
    }

    ::memset( buffer, 0, max_sendlen ); 
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

    delete [] buffer; 
    return bRet;
}

template<typename T> bool ThriftStep::SendAck(const T& tData) 
{
    Thrift2Pb outThriftMsg;
    outThriftMsg.Clear();

    SetThriftSeq(m_iSeq, outThriftMsg);
    SetThriftInterfaceName(m_sName, outThriftMsg);

    SetThriftSeq(m_iSeq, outThriftMsg);
    SetThriftInterfaceName(m_sName, outThriftMsg);

    if (false == PacketThriftData(outThriftMsg, tData))
    {
        LOG4_ERROR("packet response faild");
        return false;
    }

    return this->SendTo(outThriftMsg);
}

/////
}




#endif
