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
    //
    // 将内部通信协议转化为对外协议（对原有协议进行字段解读，并将值付给新协议字
    // 段）,  然后将新协议内存数据序列化为pBuff内容。
    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff);
    //功能相反，从pBuff内存中读取新协议数据，并将填充到内部协议的头部和body里去
    virtual E_CODEC_STATUS Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace oss */

#endif /* SRC_CODEC_CLIENTMSGCODEC_HPP_ */
