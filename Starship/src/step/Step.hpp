/*******************************************************************************
 * Project:  AsyncServer
 * @file     Step.hpp
 * @brief    异步步骤基类
 * @author   
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef STEP_HPP_
#define STEP_HPP_

#include <map>
#include <set>
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "ev.h"         // need ev_tstamp

#include "OssError.hpp"
#include "OssDefine.hpp"
#include "labor/OssLabor.hpp"
#include "cmd/CW.hpp"
#include "protocol/msg.pb.h"
#include "session/Session.hpp"

#include "CoStep.hpp"

namespace oss
{

class OssWorker;
class Cmd;
class RedisStep;

/**
 * @brief 步骤基类
 * @note 全异步Server框架基于状态机设计模式实现，Step类就是框架的状态机基类。Step类还保存了业务层连接标
 * 识到连接层实际连接的对应关系，业务层通过Step类可以很方便地把数据发送到指定连接。
 */
class Step :public CoStep
{
public:
    Step(const std::string& sCoName);

    Step(const tagMsgShell& stReqMsgShell, 
         const MsgHead& oReqMsgHead,
         const MsgBody& oReqMsgBody,
         const std::string& sCoName);
    virtual ~Step();

    /**
     * @brief: CorFunc
     *  由业务的子类来实现，
     *  该接口已经被协程调用
     *
     *  协程只需在该接口内部写同步逻辑即可
     */ 
    virtual void CorFunc() = 0;

public:
    /**
     * @brief 设置步骤最近刷新时间
     */
    void SetActiveTime(ev_tstamp dActiveTime)
    {
        m_dActiveTime = dActiveTime;
    }

    /**
     * @brief 获取步骤刷新时间
     */
    ev_tstamp GetActiveTime() const
    {
        return(m_dActiveTime);
    }

    /**
     * @brief 设置步骤超时时间
     */
    void SetTimeout(ev_tstamp dTimeout)
    {
        m_dTimeout = dTimeout;
    }

    /**
     * @brief 获取步骤超时时间
     */
    ev_tstamp GetTimeout() const
    {
        return(m_dTimeout);
    }

    /**
     * @brief 延迟超时时间
     */
    void DelayTimeout()
    {
        m_dActiveTime += m_dTimeout;
    }

    const std::string& ClassName() const
    {
        return(m_strClassName);
    }

    void SetClassName(const std::string& strClassName)
    {
        m_strClassName = strClassName;
    }

    /**
     * @brief 是否注册了回调
     * @return 注册结果
     */
    bool IsRegistered() const
    {
        return(m_bRegistered);
    }

protected:
    /**
     * @brief 登记回调步骤
     * @note  登记回调步骤。如果StepA.Callback()调用之后仍有后续步骤，则需在StepA.Callback()
     * 中new一个 新的具体步骤oss::Step子类实例StepB，调用oss::Step基类的RegisterCallback()
     * 方法将该新实例登记并执行该新实例的StepB.Start()方法，若StepB.Start()执行成功则后续
     * StepB.Callback()会被调用，若StepB.Start()执行失败，则调用oss::Step基类的
     * DeleteCallback()将刚登记的StepB删除掉并执行对应的错误处理。
     * 若RegisterCallback()登记失败（失败可能性微乎其微）则应将StepB销毁,重新new StepB实例并登记，
     * 若多次（可自定义）登记失败则应放弃登记，并将StepB销毁；若RegisterCallback()登记成功则一定不可以
     * 销毁StepB（销毁操作会自动在回调StepB.Callback()之后执行）
     * @param pStep 回调步骤实例
     * @return 是否登记成功
     */
    bool RegisterCallback(Step* pStep, ev_tstamp dTimeout = 0.0);

    /**
     * @brief 删除回调步骤
     * @note 在RegisterCallback()成功，但执行pStep->Start()失败时被调用
     * @param pStep 回调步骤实例
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
     * @brief 获取日志类实例
     * @note 派生类写日志时调用
     * @return 日志类实例
     */
    log4cplus::Logger& GetLogger()
    {
        return (*m_pLogger);
    }

    log4cplus::Logger* GetLoggerPtr()
    {
        return (m_pLogger);
    }

    uint32 GetNodeId();
    uint32 GetWorkerIndex();

    /**
     * @brief 获取当前Worker进程标识符
     * @note 当前Worker进程标识符由 IP:port:worker_index组成，例如： 192.168.18.22:30001.2
     * @return 当前Worker进程标识符
     */
    const std::string& GetWorkerIdentify();

    /**
     * @brief 获取当前节点类型
     * @return 当前节点类型
     */
    const std::string& GetNodeType() const;

    /**
     * @brief 获取框架层操作者实例
     * @note 获取框架层操作者实例，业务层Step派生类偶尔会用到此函数。调用此函数时请先从Step基类查找是否有
     * 可替代的方法，能不获取框架层操作者实例则尽量不要获取。
     * @return 框架层操作者实例
     */
    OssLabor* GetLabor()
    {
        return(m_pLabor);
    }

    /**
     * @brief 获取会话实例
     * @param uiSessionId 会话ID
     * @return 会话实例（返回NULL表示不存在uiSessionId对应的会话实例）
     */
    Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "oss::Session");
    Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "oss::Session");

    /**
     * @brief 连接成功后发送
     * @note 当前Server往另一个Server发送数据而两Server之间没有可用连接时，框架层向对端发起连接（发起连接
     * 的过程是异步非阻塞的，connect()函数返回的时候并不知道连接是否成功），并将待发送数据存放于应用层待发
     * 送缓冲区。当连接成功时将待发送数据从应用层待发送缓冲区拷贝到应用层发送缓冲区并发送。此函数由框架层自
     * 动调用，业务逻辑层无须关注。
     * @param stMsgShell 消息外壳
     * @return 是否发送成功
     */
    bool SendTo(const tagMsgShell& stMsgShell);

    /**
     * @brief 发送数据
     * @note 使用指定连接将数据发送。如果能直接得知stMsgShell（比如刚从该连接接收到数据，欲回确认包），就
     * 应调用此函数发送。此函数是SendTo()函数中最高效的一个。
     * @param stMsgShell 消息外壳
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);


    /**
     * @brief: SendTo 
     *
     * 用于在s-s 间连接建立中数据发送
     * @param stMsgShell
     * @param oMsgHead
     * @param oMsgBody
     * @param pStep
     *
     * @return 
     */
    bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody,
                oss::Step* pStep);
    /**
     * @brief 发送数据
     * @note 指定连接标识符将数据发送。此函数先查找与strIdentify匹配的stMsgShell，如果找到就调用
     * SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
     * 发送，如果未找到则调用SendToWithAutoConnect(const std::string& strIdentify,
     * const MsgHead& oMsgHead, const MsgBody& oMsgBody)连接后再发送。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    /**
     * @brief 发送到下一个同一类型的节点
     * @note 发送到下一个同一类型的节点，适用于对同一类型节点做轮询方式发送以达到简单的负载均衡。
     * @param strNodeType 节点类型
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendToNext(const std::string& strNodeType, MsgHead& oMsgHead, MsgBody& oMsgBody);

    /**
     * @brief 以取模方式选择发送到同一类型节点
     * @note 以取模方式选择发送到同一类型节点，实现简单有要求的负载均衡。
     * @param strNodeType 节点类型
     * @param uiModFactor 取模因子
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
public:
    /**
     * @brief 获取当前Step的序列号
     * @note 获取当前Step的序列号，对于同一个Step实例，每次调用GetSequence()获取的序列号是相同的。
     * @return 序列号
     */
    uint32 GetSequence();

    /**
     * @brief 添加指定标识的消息外壳
     * @note 添加指定标识的消息外壳由Cmd类实例和Step类实例调用，该调用会在Step类中添加一个标识
     * 和消息外壳的对应关系，同时Worker中的连接属性也会添加一个标识。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param stMsgShell  消息外壳
     * @return 是否添加成功
     */
    bool AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell);

    /**
     * @brief 删除指定标识的消息外壳
     * @note 删除指定标识的消息外壳由Worker类实例调用，在IoError或IoTimeout时调
     * 用。
     */
    void DelMsgShell(const std::string& strIdentify);

    /**
     * @brief 注册redis回调
     * @param strIdentify redis节点标识(192.168.16.22:9988形式的IP+端口)
     * @param pRedisStep redis步骤实例
     * @return 是否注册成功
     */
    bool RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep);

    /**
     * @brief 注册redis回调
     * @param strHost redis节点IP
     * @param iPort redis端口
     * @param pRedisStep redis步骤实例
     * @return 是否注册成功
     */
    bool RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep);

    /**
     * @brief 添加标识的节点类型属性
     * @note 添加标识的节点类型属性，用于以轮询方式向同一节点类型的节点发送数据，以
     * 实现简单的负载均衡。只有Server间的各连接才具有NodeType属性，客户端到Access节
     * 点的连接不具有NodeType属性。
     * @param strNodeType 节点类型
     * @param strIdentify 连接标识符
     */
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);

    /**
     * @brief 删除标识的节点类型属性
     * @note 删除标识的节点类型属性，当一个节点下线，框架层会自动调用此函数删除标识
     * 的节点类型属性。
     * @param strNodeType 节点类型
     * @param strIdentify 连接标识符
     */
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);

    /**
     * @brief 添加指定标识的redis context地址
     * @note 添加指定标识的redis context由Worker调用，该调用会在Step类中添加一个标识
     * 和redis context的对应关系。
     */
    bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx);

    /**
     * @brief 删除指定标识的redis context地址
     * @note 删除指定标识的到redis地址的对应关系（此函数被调用时，redis context的资源已被释放或将被释放）
     * 用。
     */
    void DelRedisContextAddr(const redisAsyncContext* ctx);
    
    // alarm data is json
    // general format: 
    // {
    //   "node_type":      "logic",
    //   "ip":             "192.168.1.1"
    //   "worker_id":      1
    //   "call_interface": "GetInfo()",
    //   "file_name":      "test.cpp",
    //   "line":           123,
    //   "time":           "2018-1-1 12:00:00,314"
    //   "note":"上面几项不需要业务填充，接口自行获取填充"
    //   "detail":         "get info fail"
    // }
    bool SendBusiAlarmToManager(const loss::CJsonObject& jsReportData);
    std::string AddDetailContent(const std::string& sData, ...);

    //------ 对外提供一个接口用于将全局session id(唯一) 存入本次step上下文中 ----//
    void SetId(const std::string& sId);
    
    //获取异步回调的状态，false: 发生错误，比如超时; true:  有结果返回 
    bool GetRespStatus();
    //当异步返回，设置返回的状态;
    bool SetRespStatus(bool bStatus);

    //获取异步回调的返回结果:消息头部和消息体
    MsgHead& GetRespHead();
    MsgBody& GetRespBody();
    
    //当异步返回时，设置返回结果的头部信息和body信息
    void SetRespHead(MsgHead* rspMsgHead);
    void SetRespBody(MsgBody* rspMsgBody);

private:
    /**
     * @brief 设置框架层操作者
     * @note 设置框架层操作者，由框架层调用，业务层派生类可直接忽略此函数
     * @param pLabor 框架层操作者，一般为OssWorker的实例
     */
    void SetLabor(OssLabor* pLabor)
    {
        m_pLabor = pLabor;
    }

    /**
     * @brief 设置日志类实例
     * @note 设置框架层操作者，由框架层调用，业务层派生类可直接忽略此函数
     * @param logger 日志类实例
     */
    void SetLogger(log4cplus::Logger* pLogger)
    {
        m_pLogger = pLogger;
    }

    /**
     * @brief 设置为已注册状态
     */
    void SetRegistered()
    {
        m_bRegistered = true;
    }

    /**
     * @brief 设置为未注册状态
     * @note 当且仅当UnRegisterCallback(Step*)时由框架调用
     */
    void UnsetRegistered()
    {
        m_bRegistered = false;
    }
protected:
    /**< 用于业务执行后，回收相关资源 */
    virtual void AfterFuncWork();

protected:  
    // 请求端的上下文信息，通过Step构造函数初始化，若调用的是不带参数的构造函数Step()，则这几个成员不会被初始化
    tagMsgShell m_stReqMsgShell;
    MsgHead m_oReqMsgHead;
    MsgBody m_oReqMsgBody;
private:
    bool m_bRegistered;
    uint32 m_ulSequence;
    ev_tstamp m_dActiveTime;
    ev_tstamp m_dTimeout;
    std::string m_strWorkerIdentify;
    OssLabor* m_pLabor;
    log4cplus::Logger* m_pLogger;
    ev_timer* m_pTimeoutWatcher;
    std::string m_strClassName;

    std::string m_oSessionId; //全网唯一的ID，用户跟踪 消息轨迹;其值来源于: 外部接口set, 从m_oReqMsgBody解析出来，默认值

protected:
    MsgHead m_rspMsgHead;     //用于存放异步回调后返回值，消息头部. 在协程模式下新增
    MsgBody m_rspMsgBody;     //用于存放异步回调后返回值, 消息体. 在协程模式下新增
    bool m_bCoRespStatus;     //used to store coroutine's response status; default: true, if false, asyn result timeout.
                              //TODO:  考虑是否返回状态的枚举值
    friend class OssWorker;
};

} /* namespace oss */

#endif /* STEP_HPP_ */
