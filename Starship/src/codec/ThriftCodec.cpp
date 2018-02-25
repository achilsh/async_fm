#include <arpa/inet.h>
#include "ThriftCodec.hpp"
#include "thrift/rpcpacket.h"

namespace oss
{

const  uint32_t ThriftCodec::iMaxBufLen;
uint8_t* ThriftCodec::pParseBuf = NULL;

ThriftCodec::ThriftCodec(loss::E_CODEC_TYPE eCodecType,
                         const std::string& strKey) 
    : StarshipCodec(eCodecType, strKey), m_uiRecPacketLen(0)
{
    if (pParseBuf == NULL) 
    {
        pParseBuf = new uint8_t[iMaxBufLen];
    }
}

ThriftCodec::~ThriftCodec()
{
}

E_CODEC_STATUS ThriftCodec::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff)
{
    return CODEC_STATUS_OK;
}

//由业务做实际的packet, 这里只负责装载一次
//thrift reponse info writen int to thrift_rsp_params;
E_CODEC_STATUS ThriftCodec::Encode(const Thrift2Pb& oThriftMsg, loss::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (oThriftMsg.has_thrift_rsp_params() == false) 
    {
        LOG4_ERROR("response has not any item");
        return  CODEC_STATUS_ERR;
    }

    int iToSendLen = oThriftMsg.thrift_rsp_params().size();

    int iRet = pBuff->Write(oThriftMsg.thrift_rsp_params().c_str(), iToSendLen);
    if (iRet != iToSendLen)
    {
        LOG4_ERROR("buff write thrift msg resp len != needtowrite len");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - 0);
        return CODEC_STATUS_ERR;
    }

    pBuff->Compact(8192);      // 超过32KB则重新分配内存
    return CODEC_STATUS_OK;
}

E_CODEC_STATUS ThriftCodec::HandleInput(loss::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    m_uiRecPacketLen = 0;
    
    if (pBuff->ReadableBytes() == 0)
    {
        LOG4_DEBUG("no data...");
        return(CODEC_STATUS_PAUSE);
    }
    uint32_t uiRecvPkgLen = pBuff->ReadableBytes();
    if (uiRecvPkgLen< 4) 
    {
        LOG4_DEBUG("thrift len not recv...");
        return(CODEC_STATUS_PAUSE);
    }

    m_uiRecPacketLen = ::ntohl(*((int32_t*)pBuff->GetRawReadBuffer())) + 4;
    if (uiRecvPkgLen < m_uiRecPacketLen)
    {
        LOG4_DEBUG("thrift pkg not recv complete...");
        return(CODEC_STATUS_PAUSE);
    }

    return CODEC_STATUS_OK;
}

E_CODEC_STATUS ThriftCodec::Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    E_CODEC_STATUS retCheckInput = HandleInput(pBuff);
    if (retCheckInput != CODEC_STATUS_OK)
    {
        return retCheckInput;
    }

    rpcpacket reqMethod;
    std::string sInterfaceName;

    int32_t iRet = reqMethod.GetFunc(const_cast<char*>(pBuff->GetRawReadBuffer()), 
                                     m_uiRecPacketLen, sInterfaceName);

    if (iRet != 0 || sInterfaceName.empty())
    {
        LOG4_ERROR("get interface name from msg fail");
        return  CODEC_STATUS_ERR;
    }

    int iSeqNum = reqMethod.m_seqid;
    if (iSeqNum < 0) 
    {
        LOG4_ERROR("get seq from msg fail");
        return  CODEC_STATUS_ERR;
    }

    Thrift2Pb  thrifMsg;
    thrifMsg.set_thrift_interface_name(sInterfaceName);
    thrifMsg.set_thrift_seq(iSeqNum);
    //暂时不使用统一格式存放thrift的msg.
    //thrifMsg.set_thrift_req_params(pBuff->GetRawReadBuffer(), m_uiRecPacketLen);
    oMsgBody.set_body(thrifMsg.SerializeAsString());
    
    ::memcpy(pParseBuf, pBuff->GetRawReadBuffer(), m_uiRecPacketLen);
    
    oMsgHead.set_seq(iSeqNum);
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    
    pBuff->SkipBytes(m_uiRecPacketLen);
    pBuff->Compact(8192);      // 超过32KB则重新分配内存
    return CODEC_STATUS_OK;
}
////
}
