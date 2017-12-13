/*******************************************************************************
 * Project:  loss
 * @file     CTlv.hpp
 * @brief 
 * @author    
 * @date:    2014年8月21日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef CTLV_HPP_
#define CTLV_HPP_

#include "CBuffer.hpp"
#include "StreamCodec.hpp"

namespace loss
{

/**
 * @brief TLV应用层通信协议类
 * @author   
 * @date    2014年8月21日
 * @note TLV应用层通信协议类
 */
class CTlv : public CStreamCodec
{
public:
    CTlv();
    CTlv(unsigned int uiType, unsigned int uiLength, const loss::CBuffer& oBuff);
    virtual ~CTlv();

    static bool Encode(unsigned int uiType, unsigned int uiInBuffLength, const loss::CBuffer& oInBuff, loss::CBuffer& oOutBuff);
    virtual bool Encode(loss::CBuffer& oOutBuff);
    virtual bool Decode(loss::CBuffer& oInBuff);

    inline unsigned int GetType() const
    {
        return(m_uiType);
    }

    inline unsigned int GetLength() const
    {
        return(m_uiLength);
    }

    inline CBuffer& GetBuff()
    {
        return(m_oBuff);
    }

private:
    unsigned int m_uiType;
    unsigned int m_uiLength;
    CBuffer m_oBuff;
};

}  // end of namespace loss

#endif /* CTLV_HPP_ */
