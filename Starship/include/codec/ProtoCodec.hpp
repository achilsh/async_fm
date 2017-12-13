/*******************************************************************************
 * Project:  Starship
 * @file     ProtoCodec.hpp
 * @brief 
 * @author   
 * @date:    2015年10月6日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_PROTOCODEC_HPP_
#define SRC_CODEC_PROTOCODEC_HPP_

#include "StarshipCodec.hpp"

namespace oss
{

class ProtoCodec: public StarshipCodec
{
public:
    ProtoCodec(loss::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~ProtoCodec();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace oss */

#endif /* SRC_CODEC_PROTOCODEC_HPP_ */
