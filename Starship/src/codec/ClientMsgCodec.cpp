/*******************************************************************************
 * Project:  Starship
 * @file     ClientMsgCodec.cpp
 * @brief 
 * @author   
 * @date:    2015年10月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <netinet/in.h>
#include "ClientMsgCodec.hpp"
#include "ClientMsgHead.hpp"

namespace oss
{

ClientMsgCodec::ClientMsgCodec(loss::E_CODEC_TYPE eCodecType, const std::string& strKey)
    : StarshipCodec(eCodecType, strKey)
{
}

ClientMsgCodec::~ClientMsgCodec()
{
}

E_CODEC_STATUS ClientMsgCodec::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagClientMsgHead stClientMsgHead;
    stClientMsgHead.version = 1;        // version暂时无用
    stClientMsgHead.encript = (unsigned char)(oMsgHead.cmd() >> 24);
    stClientMsgHead.cmd = htons((unsigned short)(gc_uiCmdBit & oMsgHead.cmd()));
    stClientMsgHead.body_len = htonl((unsigned int)oMsgHead.msgbody_len());
    stClientMsgHead.seq = htonl(oMsgHead.seq());
    //stClientMsgHead.checksum = htons((unsigned short)stClientMsgHead.checksum);
    if (oMsgBody.ByteSize() > 1000000) // pb 最大限制
    {
        LOG4_ERROR("oMsgBody.ByteSize() > 1000000");
        return(CODEC_STATUS_ERR);
    }
    int iErrno = 0;
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = sizeof(stClientMsgHead);
    LOG4_TRACE("cmd %u, seq %u, len %u", oMsgHead.cmd(), oMsgHead.seq(), oMsgHead.msgbody_len());
    if (oMsgHead.msgbody_len() == 0)    // 无包体（心跳包等）
    {
        iWriteLen = pBuff->Write(&stClientMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stClientMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        pBuff->Compact(8192);
        return(CODEC_STATUS_OK);
    }
    iHadWriteLen += iWriteLen;
    if (stClientMsgHead.encript == 0)       // 不压缩也不加密
    {
        iWriteLen = pBuff->Write(&stClientMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stClientMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        iNeedWriteLen = oMsgBody.ByteSize();
        iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        iHadWriteLen += iWriteLen;
    }
    else
    {
        std::string strCompressData;
        std::string strEncryptData;
        if (gc_uiZipBit & oMsgHead.cmd())
        {
            if (!Zip(oMsgBody.SerializeAsString(), strCompressData))
            {
                LOG4_ERROR("zip error!");
                return(CODEC_STATUS_ERR);
            }
        }
        else if (gc_uiGzipBit & oMsgHead.cmd())
        {
            if (!Gzip(oMsgBody.SerializeAsString(), strCompressData))
            {
                LOG4_ERROR("gzip error!");
                return(CODEC_STATUS_ERR);
            }
        }
        if (gc_uiRc5Bit & oMsgHead.cmd())
        {
            if (strCompressData.size() > 0)
            {
                if (!Rc5Encrypt(strCompressData, strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return(CODEC_STATUS_ERR);
                }
            }
            else
            {
                if (!Rc5Encrypt(oMsgBody.SerializeAsString(), strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return(CODEC_STATUS_ERR);
                }
            }
        }

        if (strEncryptData.size() > 0)              // 加密后的数据包
        {
            stClientMsgHead.body_len = htonl((unsigned int)strEncryptData.size());
            iWriteLen = pBuff->Write(&stClientMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stClientMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = strEncryptData.size();
            iWriteLen = pBuff->Write(strEncryptData.c_str(), strEncryptData.size());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
        else if (strCompressData.size() > 0)        // 压缩后的数据包
        {
            stClientMsgHead.body_len = htonl((unsigned int)strCompressData.size());
            iWriteLen = pBuff->Write(&stClientMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stClientMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = strCompressData.size();
            iWriteLen = pBuff->Write(strCompressData.c_str(), strCompressData.size());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
        else    // 无效的压缩或加密算法，打包原数据
        {
            iWriteLen = pBuff->Write(&stClientMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stClientMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = oMsgBody.ByteSize();
            iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
    }
    LOG4_TRACE("oMsgBody.ByteSize() = %d, iWriteLen = %d(compress or encrypt maybe)", oMsgBody.ByteSize(), iWriteLen);
    pBuff->Compact(8192);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS ClientMsgCodec::Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes() = %u", __FUNCTION__, pBuff->ReadableBytes());
    size_t uiHeadSize = sizeof(tagClientMsgHead);
    if (pBuff->ReadableBytes() >= uiHeadSize)
    {
        tagClientMsgHead stClientMsgHead;
        int iReadIdx = pBuff->GetReadIndex();
        pBuff->Read(&stClientMsgHead, uiHeadSize);
        stClientMsgHead.cmd = ntohs(stClientMsgHead.cmd);
        stClientMsgHead.body_len = ntohl(stClientMsgHead.body_len);
        stClientMsgHead.seq = ntohl(stClientMsgHead.seq);
        stClientMsgHead.checksum = ntohs(stClientMsgHead.checksum);
        LOG4_TRACE("cmd %u, seq %u, len %u, pBuff->ReadableBytes() %u",
                        stClientMsgHead.cmd, stClientMsgHead.seq, stClientMsgHead.body_len,
                        pBuff->ReadableBytes());
        oMsgHead.set_cmd(((unsigned int)stClientMsgHead.encript << 24) | stClientMsgHead.cmd);
        oMsgHead.set_msgbody_len(stClientMsgHead.body_len);
        oMsgHead.set_seq(stClientMsgHead.seq);
        if (0 == stClientMsgHead.body_len)      // 心跳包无包体
        {
            pBuff->Compact(8192);
            return(CODEC_STATUS_OK);
        }
        if (pBuff->ReadableBytes() >= stClientMsgHead.body_len)
        {
            bool bResult = false;
            if (stClientMsgHead.encript == 0)       // 未压缩也未加密
            {
                bResult = oMsgBody.ParseFromArray(pBuff->GetRawReadBuffer(), stClientMsgHead.body_len);
            }
            else    // 有压缩或加密，先解密再解压，然后用MsgBody反序列化
            {
                std::string strUncompressData;
                std::string strDecryptData;
                if (gc_uiRc5Bit & oMsgHead.cmd())
                {
                    std::string strRawData;
                    strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stClientMsgHead.body_len);
                    if (!Rc5Decrypt(strRawData, strDecryptData))
                    {
                        LOG4_ERROR("Rc5Decrypt error!");
                        return(CODEC_STATUS_ERR);
                    }
                }
                if (gc_uiZipBit & oMsgHead.cmd())
                {
                    if (strDecryptData.size() > 0)
                    {
                        if (!Unzip(strDecryptData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                    else
                    {
                        std::string strRawData;
                        strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stClientMsgHead.body_len);
                        if (!Unzip(strRawData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                }
                else if (gc_uiGzipBit & oMsgHead.cmd())
                {
                    if (strDecryptData.size() > 0)
                    {
                        if (!Gunzip(strDecryptData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                    else
                    {
                        std::string strRawData;
                        strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stClientMsgHead.body_len);
                        if (!Gunzip(strRawData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                }

                if (strUncompressData.size() > 0)       // 解压后的数据
                {
                    oMsgHead.set_msgbody_len(strUncompressData.size());
                    bResult = oMsgBody.ParseFromString(strUncompressData);
                }
                else if (strDecryptData.size() > 0)     // 解密后的数据
                {
                    oMsgHead.set_msgbody_len(strDecryptData.size());
                    bResult = oMsgBody.ParseFromString(strDecryptData);
                }
                else    // 无效的压缩或解密算法，仍然解析原数据
                {
                    bResult = oMsgBody.ParseFromArray(pBuff->GetRawReadBuffer(), stClientMsgHead.body_len);
                }
            }
            if (bResult)
            {
                pBuff->SkipBytes(oMsgBody.ByteSize());
                pBuff->Compact(8192);
                return(CODEC_STATUS_OK);
            }
            else
            {
                LOG4_ERROR("cmd[%u], seq[%lu] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
                return(CODEC_STATUS_ERR);
            }
        }
        else
        {
            pBuff->SetReadIndex(iReadIdx);
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

} /* namespace oss */
