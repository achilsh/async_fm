/**
 * @file: ThriftCodec.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-22
 */

#ifndef __THRIFT_CODECS_HPP__
#define __THRIFT_CODECS_HPP__

#include "StarshipCodec.hpp"
#include "protocol/thrift2pb.pb.h"

namespace oss
{

class ThriftCodec: public StarshipCodec
{
    public:
     ThriftCodec(loss::E_CODEC_TYPE eCodecType,const std::string& strKey = "That's a lizard.");
     virtual ~ThriftCodec();

     virtual E_CODEC_STATUS Encode(const Thrift2Pb& oThriftMsg, loss::CBuffer* pBuff);
     virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff);
     //将thrift 协议信息写入到msgbody  中的body
     virtual E_CODEC_STATUS Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
};
//
}

#endif
