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
    virtual ~HttpStep();

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

public:  // Step基类的方法，HttpStep中无需关注
    /**
     * @note Step基类的方法，HttpStep中无须关注
     */
    virtual E_CMD_STATUS Callback(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL)
    {
        return(STATUS_CMD_COMPLETED);
    }
};

} /* namespace oss */

#endif /* SRC_STEP_HTTPSTEP_HPP_ */
