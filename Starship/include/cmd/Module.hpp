/*******************************************************************************
 * Project:  Starship
 * @file     Module.hpp
 * @brief    Http服务模块基类
 * @author   
 * @date:    2015年10月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMD_MODULE_HPP_
#define SRC_CMD_MODULE_HPP_

#include "util/http/http_parser.h"
#include "protocol/http.pb.h"
#include "Cmd.hpp"
#include "step/HttpStep.hpp"

namespace oss
{

class Module: public Cmd
{
public:
    Module();
    virtual ~Module();

    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief http服务模块处理入口
     * @param stMsgShell 来源消息外壳
     * @param oHttpMsg 接收到的http数据包
     * @return 是否处理成功
     */
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const HttpMsg& oInHttpMsg) = 0;

    /**
     * @brief 从Cmd基类继承的命令处理入口
     * @note 在Module类中不需要实现此版本AnyMessage
     */
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody)
    {
        return(false);
    }

public:
    const std::string& GetModulePath() const
    {
        return(m_strModulePath);
    }

    void SetModulePath(const std::string& strModulePaht)
    {
        m_strModulePath = strModulePaht;
    }

private:
    std::string m_strModulePath;
};

} /* namespace oss */

#endif /* SRC_CMD_MODULE_HPP_ */
