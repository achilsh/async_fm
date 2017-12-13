/*******************************************************************************
 * Project:  PluginServer
 * @file     Cmd.hpp
 * @brief    业务处理基类
 * @author   
 * @date:    2015年3月6日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef CMD_HPP_
#define CMD_HPP_

#include "log4cplus/loggingmacros.h"
#include "CW.hpp"
#include "protocol/msg.pb.h"
//#include "protocol/oss_sys.pb.h"
#include "OssError.hpp"
#include "OssDefine.hpp"
#include "labor/OssLabor.hpp"
#include "step/Step.hpp"

namespace oss
{

class Step;

class Cmd
{
public:
    Cmd();
    virtual ~Cmd();

    /**
     * @brief 初始化Cmd
     * @note Cmd类实例初始化函数，大部分Cmd不需要初始化，需要初始化的Cmd可派生后实现此函数，
     * 在此函数里可以读取配置文件（配置文件必须为json格式）。配置文件由Cmd的设计者自行定义，
     * 存放于conf/目录，配置文件名最好与Cmd名字保持一致，加上.json后缀。配置文件的更新同步
     * 会由框架自动完成。
     * @return 是否初始化成功
     */
    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief 命令处理入口
     * @note 框架层成功解析数据包后，根据MsgHead里的Cmd找到对应的Cmd类实例调用将数据包及
     * 数据包来源MsgShell传给AnyMessage处理。若处理过程不涉及网络IO之类需异步处理的耗时调
     * 用，则无需新创建Step类实例来处理。若处理过程涉及耗时异步调用，则应创建Step类实例，
     * 并向框架层注册Step类实例，调用Step.Start()后即返回。
     * @param stMsgShell 消息外壳
     * @param oInMsgHead 数据包头
     * @param oInMsgBody 数据包体
     * @return 命令是否处理成功
     */
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody) = 0;
public:
	int GetCmd() const
	{
		return(m_iCmd);
	}

protected:
    void SetClassName(const std::string& strClassName)
    {
        m_strClassName = strClassName;
    }

    log4cplus::Logger GetLogger()
    {
        return (*m_pLogger);
    }

    log4cplus::Logger* GetLoggerPtr()
    {
        return (m_pLogger);
    }

    const std::string& GetConfigPath() const
    {
        return(m_strConfigPath);
    }

    OssLabor* GetLabor()
    {
        return m_pLabor;
    }

    uint32 GetNodeId();
    uint32 GetWorkerIndex();

    /**
     * @brief 注册步骤
     * @param pStep 回调步骤
     * @return 是否注册成功
     */
    bool RegisterCallback(Step* pStep);

    /**
     * @brief 删除步骤
     * @param pStep 回调步骤
     */
    void DeleteCallback(Step* pStep);

    /**
     * @brief 预处理
     * @note 预处理用于将等待预处理对象与框架建立弱联系，使被处理的对象可以获得框架一些工具，如写日志指针
     * @param pStep 等待预处理的Step
     * @return 预处理结果
     */
    bool Pretreat(Step* pStep);

    /**
     * @brief 登记会话
     * @param pSession 会话实例
     * @return 是否登记成功
     */
    bool RegisterCallback(Session* pSession);

    /**
     * @brief 删除回调步骤
     * @note 在RegisterCallback()成功，但执行pStep->Start()失败时被调用
     * @param pSession 会话实例
     */
    void DeleteCallback(Session* pSession);

    /**
     * @brief 注册redis回调
     * @param strRedisNodeType redis节点类型
     * @param pRedisStep redis步骤实例
     * @return 是否注册成功
     */
    bool RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep);

    /**
     * @brief 注册redis回调
     * @param strHost redis节点IP
     * @param iPort redis端口
     * @param pRedisStep redis步骤实例
     * @return 是否注册成功
     */
    bool RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep);

    /**
     * @brief 获取会话实例
     * @param uiSessionId 会话ID
     * @return 会话实例（返回NULL表示不存在uiSessionId对应的会话实例）
     */
    Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "oss::Session");
    Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "oss::Session");

public:
    const std::string& ClassName() const
    {
        return(m_strClassName);
    }

private:
    void SetLabor(OssLabor* pLabor)
    {
        m_pLabor = pLabor;
    }

    void SetLogger(log4cplus::Logger* pLogger)
    {
        m_pLogger = pLogger;
    }

    void SetConfigPath(const std::string& strWorkPath)
    {
        if (m_strConfigPath == "")
        {
            m_strConfigPath = strWorkPath + std::string("/conf/");
        }
    }

    void SetCmd(int iCmd)
    {
        m_iCmd = iCmd;
    }

protected:
    char* m_pErrBuff;

private:
    OssLabor* m_pLabor;
    log4cplus::Logger* m_pLogger;
    std::string m_strConfigPath;
    int m_iCmd;
    std::string m_strClassName;

    friend class OssWorker;
};

} /* namespace oss */

#endif /* CMD_HPP_ */
