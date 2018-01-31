/*******************************************************************************
 * Project:  Starship
 * @file     HttpStep.hpp
 * @brief    Http服务的异步步骤基类
 * @author   
 * @date:    2015年10月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_HTTPSTEP_HPP_
#define SRC_STEP_HTTPSTEP_HPP_

#include "Step.hpp"
#include "protocol/http.pb.h"
#include "codec/HttpCodec.hpp"

namespace oss
{

class HttpStep: public Step
{
public:
    HttpStep();
    HttpStep(const oss::tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg);
    virtual ~HttpStep();
    
    /**< 在内部主动发起http请求时才关注, HttpStep 发送常规tcp req时无需关注 */
    virtual E_CMD_STATUS Callback(
                    const tagMsgShell& stMsgShell,
                    const HttpMsg& oHttpMsg,
                    void* data = NULL) = 0;

    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

    bool HttpPost(const std::string& strUrl, const std::string& strBody, const std::map<std::string, std::string>& mapHeaders);
    bool HttpGet(const std::string& strUrl);

protected:
    bool HttpRequest(const HttpMsg& oHttpMsg);

    bool SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg);

public:  
    /**
     * @note Step基类的方法, HttpStep 发送http req 时无须关注
     */
    virtual E_CMD_STATUS Callback(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL)
    {
        return(STATUS_CMD_COMPLETED);
    }


    /**
     * @brief: SendAck, 将结果返回给上游请求 
     *
     * @tparam T
     * @param sErr, 如果为空，sData将设置为正确的返回结果
     * @param sData, 如果为空，sErr设置错误信息
     */
    void SendAck(const std::string& sErr, const std::string& sData = "");

protected:
    oss::tagMsgShell m_stMsgShell;     /**< 接收上游请求时保存的上下文  */
    uint32_t m_uiMajor; 
    uint32_t m_uiMinor;
};

} /* namespace oss */

#endif /* SRC_STEP_HTTPSTEP_HPP_ */
