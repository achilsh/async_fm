/*******************************************************************************
 * Project:  Starship
 * @file     ClientMsgCodec.hpp
 * @brief    与手机客户端通信协议编解码器
 * @author   
 * @date:    2015年10月9日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CLIENTMSGCODEC_HPP_
#define SRC_CODEC_CLIENTMSGCODEC_HPP_

#include "StarshipCodec.hpp"

namespace oss
{

class ClientMsgCodec: public StarshipCodec
{
public:
    ClientMsgCodec(loss::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~ClientMsgCodec();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace oss */

#endif /* SRC_CODEC_CLIENTMSGCODEC_HPP_ */
