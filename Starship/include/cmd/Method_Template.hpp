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

template<typename T>
bool Method::GetThriftParams(T& tParam, const Thrift2Pb& oInThriftMsg)
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
    
    if (oInThriftMsg.has_thrift_req_params() == false)
    {
        LOG4_ERROR("pb not has param");
        return false;
    }

    std::string sparam = oInThriftMsg.thrift_req_params();
    rpcpacket data;
    std::string sMethod;

    int iRet = data.GetFunc(const_cast<char*>(sparam.c_str()), sparam.size(), sMethod);
    if (iRet != 0) 
    {
        return false;
    }
    
    iRet = data.GetParam(tParam);
    if (iRet != 0) 
    {
        LOG4_ERROR("get param fail for method: %s", sMethod.c_str());
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

