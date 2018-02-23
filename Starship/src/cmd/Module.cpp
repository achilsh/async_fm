/*******************************************************************************
 * Project:  Starship
 * @file     Module.cpp
 * @brief 
 * @author   
 * @date:    2015年10月19日
 * @note
 * Modify history:
 ******************************************************************************/
#include "Module.hpp"

namespace oss
{

Module::Module(): m_uiHttpMajor(0), m_uiHttpMinor(0)
{
}

Module::~Module()
{
}

bool Module::AnyMessageBase(const tagMsgShell& stMsgShell,
                            const HttpMsg& oInHttpMsg)
{
    m_tagMsgShell = stMsgShell;
    m_uiHttpMajor = oInHttpMsg.http_major();
    m_uiHttpMinor = oInHttpMsg.http_minor();
    return AnyMessage(m_tagMsgShell, oInHttpMsg);
}

void Module::SendAck(const std::string& sErr, const std::string& sData)
{
    const std::string& sRet = sErr.empty() == true ? sData: sErr;
    HttpMsg oOutHttpMsg;
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_status_code(200);
    oOutHttpMsg.set_http_major(m_uiHttpMajor);
    oOutHttpMsg.set_http_minor(m_uiHttpMinor);

    loss::CJsonObject retJson;
    retJson.Add("code", sRet);
    oOutHttpMsg.set_body(retJson.ToString());
    GetLabor()->SendTo(m_tagMsgShell,oOutHttpMsg);
}

///
} /* namespace oss */
