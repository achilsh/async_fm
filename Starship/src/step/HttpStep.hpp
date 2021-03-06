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

class HttpStep: virtual public Step
{
public:
    HttpStep(const std::string& sCoName);
    HttpStep(const oss::tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg, const std::string& sCoName);
    virtual ~HttpStep();
    
    /**
     * @brief: CorFunc
     *  由业务的子类来实现，
     *  该接口已经被协程调用
     *
     *  协程只需在该接口内部写同步逻辑即可
     */ 
    virtual void CorFunc() = 0;

    bool HttpPost(const std::string& strUrl, const std::string& strBody, const std::map<std::string, std::string>& mapHeaders);
    bool HttpGet(const std::string& strUrl);

protected:
    bool HttpRequest(const HttpMsg& oHttpMsg);

    
    /**
     * @brief: SendTo 
     *  
     *  是http 上游请求的回包.
     *
     * @param stMsgShell
     * @param oHttpMsg
     *
     * @return 
     */
    bool SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg);

public:  
    /**
     * @brief: SendAck, 将结果返回给上游请求 
     *
     * @tparam T
     * @param sErr, 如果为空，sData将设置为正确的返回结果
     * @param sData, 如果为空，sErr设置错误信息
     */
    void SendAck(const std::string& sErr, const std::string& sData = "");

    /**
     * @brief: SetHttpRespBody 
     * 该函数用于 接收http response 报文
     *
     * 接收到的http报文存放数据成员:  m_msgRespHttp
     *
     * @param respHttp: 接收到的 http response 
     */
    void SetHttpRespBody(HttpMsg* respHttp);
protected:
    oss::tagMsgShell m_stMsgShell;     /**< 接收上游请求时保存的上下文  */
    uint32_t m_uiMajor; 
    uint32_t m_uiMinor;
    HttpMsg m_msgRespHttp;
};

} /* namespace oss */

#endif /* SRC_STEP_HTTPSTEP_HPP_ */
