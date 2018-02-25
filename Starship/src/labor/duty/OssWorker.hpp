/*******************************************************************************
 * Project:  AsyncServer
 * @file     OssWorker.hpp
 * @brief    Oss工作者
 * @author   
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef OSSWORKER_HPP_
#define OSSWORKER_HPP_

#include <unordered_map>  /**< 引入c++11 hash map */
#include <map>
#include <list>
#include <dlfcn.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "hiredis/hiredis.h"

#include "Attribution.hpp"
#include "protocol/msg.pb.h"
#include "protocol/oss_sys.pb.h"
#include "labor/OssLabor.hpp"
#include "codec/StarshipCodec.hpp"

namespace oss
{

struct tagRedisAttr;
class CmdConnectWorker;

typedef Cmd* CreateCmd();

class OssWorker;
class CTimer;
struct tagSo
{
    void* pSoHandle;
    Cmd* pCmd;
    int iVersion;
    tagSo();
    ~tagSo();
};

struct tagModule
{
    void* pSoHandle;
    Module* pModule;
    int iVersion;
    tagModule();
    ~tagModule();
};

struct tagMethod
{
    void *pSoHandle;
    Method* pMethod;
    int iVersion;
    int iCmd;
    tagMethod();
    ~tagMethod();
};
struct tagIoWatcherData
{
    int iFd;
    uint32 ulSeq;
    OssWorker* pWorker;     // 不在结构体析构时回收

    tagIoWatcherData() : iFd(0), ulSeq(0), pWorker(NULL)
    {
    }
};

struct tagSessionWatcherData
{
    char szSessionName[32];
    char szSessionId[64];
    OssWorker* pWorker;     // 不在结构体析构时回收
    tagSessionWatcherData() : pWorker(NULL)
    {
        memset(szSessionName, 0, sizeof(szSessionName));
        memset(szSessionId, 0, sizeof(szSessionId));
    }
};

struct SerNodeInfo
{
    SerNodeInfo():strIp("0.0.0.0"),uiPort(6666),uiFreqence(0),uiWorkNums(0),
    uiFailReqNumsPersec(0), tmFailReqTimeStamp(0), uiTotalFailReqNums(0) {}
    ~SerNodeInfo(){}
    SerNodeInfo& operator = (const SerNodeInfo& item)
    {
        if(this != &item)
        {
            strIp.assign(item.strIp);
            uiPort = item.uiPort;
            uiFreqence = item.uiFreqence;
            uiWorkNums = item.uiWorkNums;
            uiFailReqNumsPersec = item.uiFailReqNumsPersec;
            tmFailReqTimeStamp = item.tmFailReqTimeStamp;
        }
        return *this;
    }
    
    SerNodeInfo(const SerNodeInfo& item)
    {
        strIp.assign(item.strIp);
        uiPort = item.uiPort;
        uiFreqence = item.uiFreqence;
        uiWorkNums = item.uiWorkNums;
        uiFailReqNumsPersec = item.uiFailReqNumsPersec;
        tmFailReqTimeStamp = item.tmFailReqTimeStamp;
    }
    void SetIp(const std::string& ip)
    {
        strIp.assign(ip);
    }
    std::string GetIp()const{return strIp;}
    unsigned int GetPort()const{return uiPort;}
    
    void SetPort(const unsigned int port)
    {
        uiPort = port;
    }
    void SetPeerWorkNums(const unsigned int workNums)
    {
        uiWorkNums = workNums;
    }

    void IncrFreqence(){ ++uiFreqence; }
    unsigned int GetFreqence()const{return uiFreqence;}
    unsigned int GetWorkNums()const{return uiWorkNums;}

    unsigned int GetFailReqNums() const { return uiTotalFailReqNums;}
    unsigned int GetPerSecFailReqNums()const { return uiFailReqNumsPersec; }
    void IncrFailReqNum() { 
        uiTotalFailReqNums++; 
        if (tmFailReqTimeStamp == time(NULL)) {
            uiFailReqNumsPersec++;  
        } else {
            uiFailReqNumsPersec=1; 
            tmFailReqTimeStamp = time(NULL);
        }
    }
    //
    void ResetFailReqNum() { uiTotalFailReqNums = 0; }
    time_t GetLastFailTm() const { return  tmFailReqTimeStamp; }
    void ResetFrequence() { uiFreqence = 0; }
    ////////////////////////
    std::string strIp;
    unsigned int uiPort;
    unsigned int uiFreqence;
    unsigned int uiWorkNums;    

    unsigned int uiFailReqNumsPersec; //req fail num per second
    time_t  tmFailReqTimeStamp;
    unsigned int uiTotalFailReqNums;
    static const int32_t TM_REUSE_FAIL_CONN = 300; //5 minute
    static const int32_t NUM_FAIL_MAXNUM_PERSEC = 10; //max fail num per second.
};

// add white list
class IDWhileList {
 public:
    IDWhileList();
    virtual ~IDWhileList();
    
    bool IsNodeInWhiteList(const std::string& ip,unsigned int uiPort); 
    bool IsIDInWhiteList(const std::string& sId);
    
    bool GetNodeById(SerNodeInfo* &serNode, int& iIndex, 
                     const std::set<SerNodeInfo*>& setNode,const std::string& sId);
    bool UpdateWHL(const std::string& sIpPort, const std::string& sId);
    friend std::ostringstream& operator << (std::ostringstream& os, const IDWhileList& idWHL);

 private:
    std::map<std::string, std::set<std::string> > m_mpIDNodeList; //key is id, val is ip:port
    std::map<std::string, std::set<std::string> > m_mpNodeIDList; //key is ip:port, val is id
};

class NodeTypeIDWhiteList {
 public:
    NodeTypeIDWhiteList();
    virtual ~NodeTypeIDWhiteList();
    void Clear();
    void Swap(NodeTypeIDWhiteList& whList);

    IDWhileList* GetWhiteList(const std::string& sNodeType);
    bool SetWhiteList(const std::string& sNodeType,
                      IDWhileList* whiteList);
    void DelOneWhiteList(const std::string& sNodeType);
    friend std::ostringstream& operator << (std::ostringstream& os, const NodeTypeIDWhiteList& nodeWHL);
 private:
    std::map<std::string, IDWhileList*> m_mpWhiteList; //key: srv_name
};

//统计正在处理的steps.
class StatiConnnectingStep 
{
    public:
     StatiConnnectingStep();
     virtual ~StatiConnnectingStep();
     bool Incr(Step* pDoStep, int32_t iFd, uint32_t iSeq);
     bool Decr(Step* pDoStep, int32_t iFd, uint32_t iSeq);
     int32_t GetstepNums(int32_t iFd, uint32_t iSeq);
     bool ResetOneConnStepNums(int32_t iFd, uint32_t iSeq);
     
     bool GetSendingSteps(std::vector<Step*>& vStep, 
                          int32_t iFd, uint32_t iSeq);
    private:
     //key is: "fd:seq", value is: frequence of same step.
     std::unordered_map<std::string, std::unordered_map<Step*,int32_t> > m_mpSendingConnStep; 
};


//--------------------------------------//
class OssWorker : public OssLabor
{
public:
    typedef std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > > T_MAP_NODE_TYPE_IDENTIFY;
public:
    OssWorker(const std::string& strWorkPath, int iControlFd, int iDataFd, 
              int iWorkerIndex, loss::CJsonObject& oJsonConf,loss::CJsonObject& oJsonSrvNmConf, bool isMonitor = false);
    virtual ~OssWorker();

    bool SetNodeTypeCmdCnf(const loss::CJsonObject& oJsonConf);
    bool SetWhiteListCnf(const std::string& sJsonConf);
    void Run();

    static void TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void StepTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void SessionTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void RedisConnectCallback(const redisAsyncContext *c, int status);
    static void RedisDisconnectCallback(const redisAsyncContext *c, int status);
    static void RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata);
    static void DelInvalidNodeCB(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void TimerTmOutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void RestartTimeOutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    
    void DoRestartWorker(struct ev_timer* watcher);
    //
    bool DelInvalidNode();
    void Terminated(struct ev_signal* watcher);
    bool CheckParent();
    bool IoRead(tagIoWatcherData* pData, struct ev_io* watcher);
    bool RecvDataAndDispose(tagIoWatcherData* pData, struct ev_io* watcher);
    bool FdTransfer();
    bool IoWrite(tagIoWatcherData* pData, struct ev_io* watcher);
    bool IoError(tagIoWatcherData* pData, struct ev_io* watcher);
    bool IoTimeout(struct ev_timer* watcher, bool bCheckBeat = true);
    bool StepTimeout(Step* pStep, struct ev_timer* watcher);
    bool SessionTimeout(Session* pSession, struct ev_timer* watcher);
    bool RedisConnect(const redisAsyncContext *c, int status);
    bool RedisDisconnect(const redisAsyncContext *c, int status);
    bool RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata);

    bool TimerTimeOut(CTimer* pTimer, struct ev_timer* watcher);
    virtual bool RegisterCallback(CTimer* pTimer);
    virtual bool DeleteCallback(CTimer* pTimer);

    /**< 协程step 注册:添加到协程管理器中并register step */

    /**
     * @brief: RegisterCoroutine 
     *  注册step,协程 
     *
     * @param pStep
     * @param dTimeout
     *
     * @return false,注册失败，true: 成功
     */
    virtual bool RegisterCoroutine(Step* pStep, double dTimeout = 0.0);

    /**< 从协程管理器中删除协程step, 并unregister step, 
     * , 如果已经注册成功，也会删除协程自身的资源
     **/
    virtual void DeleteCoroutine(Step* pStep);

    /**< 启动 */
    virtual bool ResumeCoroutine(Step* pStep);

    /**< 挂起操作 */
    virtual bool YieldCorountine(Step* pStep);

public:     // Cmd类和Step类只需关注这些方法
    virtual uint32 GetSequence()
    {
        ++m_ulSequence;
        if (m_ulSequence == 0)      // Server长期运行，sequence达到最大正整数又回到0
        {
            ++m_ulSequence;
        }
        return(m_ulSequence);
    }

    virtual log4cplus::Logger GetLogger()
    {
        return(m_oLogger);
    }

    virtual const std::string& GetNodeType() const
    {
        return(m_strNodeType);
    }

    virtual const loss::CJsonObject& GetCustomConf() const
    {
        return(m_oCustomConf);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_uiNodeId);
    }

    virtual const std::string& GetHostForServer() const
    {
        return(m_strHostForServer);
    }

    virtual int GetPortForServer() const
    {
        return(m_iPortForServer);
    }

    virtual int GetWorkerIndex() const
    {
        return(m_iWorkerIndex);
    }

    virtual int GetClientBeatTime() const
    {
        return((int)m_dIoTimeout);
    }

    virtual const std::string& GetWorkerIdentify()
    {
        if (m_strWorkerIdentify.size() == 0)
        {
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d:%d", GetHostForServer().c_str(),
                            GetPortForServer(), GetWorkerIndex());
            m_strWorkerIdentify = szWorkerIdentify;
        }
        return(m_strWorkerIdentify);
    }

    virtual bool Pretreat(Cmd* pCmd);
    virtual bool Pretreat(Step* pStep);
    virtual bool Pretreat(Session* pSession);
    
    virtual bool RegisterCallback(Step* pStep, ev_tstamp dTimeout = 0.0);
    virtual void DeleteCallback(Step* pStep);
    
    virtual bool UnRegisterCallback(Step* pStep);
    virtual bool RegisterCallback(Session* pSession);
   
    virtual void DeleteCallback(Session* pSession);
    virtual bool RegisterCallback(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep);
   
    virtual Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "oss::Session");
    virtual Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "oss::Session");
    virtual bool Disconnect(const tagMsgShell& stMsgShell, bool bMsgShellNotice = true);
    virtual bool Disconnect(const std::string& strIdentify, bool bMsgShellNotice = true);

public:     // Worker相关设置（由专用Cmd类调用这些方法完成Worker自身的初始化和更新）
    virtual bool SetProcessName(const loss::CJsonObject& oJsonConf);
    /** @brief 加载配置，刷新Server */
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    virtual bool AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell);
    virtual void DelMsgShell(const std::string& strIdentify);
    virtual void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);

    virtual bool RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep);
    virtual bool RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep);

    /*  redis 节点管理相关操作从框架中移除，交由DataProxy的SessionRedisNode来管理，框架只做到redis的连接管理
    virtual bool RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep);
    virtual void AddRedisNodeConf(const std::string& strNodeType, const std::string strHost, int iPort);
    virtual void DelRedisNodeConf(const std::string& strNodeType, const std::string strHost, int iPort);
    */
    virtual bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx);
    virtual void DelRedisContextAddr(const redisAsyncContext* ctx);

    virtual bool UpDateNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo);
    bool ParseDownStreamNodeInfo(loss::CJsonObject& jsonDownStream);

    // alarm data is json
    // general format: 
    // {
    //   "node_type":      "logic",
    //   "ip":             "192.168.1.1"
    //   "worker_id":      1
    //   "note":"上面几项不需要业务填充，接口自行获取填充"
    //   "call_interface": "GetInfo()",
    //   "file_name":      "test.cpp",
    //   "line":           123,
    //   "time":           "2018-1-1 12:00:00,314"
    //   "detail":         "get info fail"
    // }
    virtual bool SendBusiAlarmToManager(const loss::CJsonObject& jsAlarm);
public:     // 发送数据或从Worker获取数据
    /** @brief 自动连接成功后，把待发送数据发送出去 */
    virtual bool SendTo(const tagMsgShell& stMsgShell);

    virtual bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    //用于s-s 间连接建立中的数据传送
    virtual bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody,
                        oss::Step* pStep); 

    virtual bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
  

    /**
     * @brief: ExecuteRedisCmd 
     * 框架执行redis cmd, 主要包括命令的发送和结果的接收
     * 其中命令发送后将被挂起，当有有结果则唤醒挂起的流程
     * 对外来说是同步模式,底层实现是异步
     *
     * @param reply: redis命令执行的结果
     *
     * @param pRedisStep: 执行redis命令的协程实例
     *
     * @return: true 成功, false 失败,
     * 具体错误码和错误信息存放在协程实例内部
     */
    virtual bool ExecuteRedisCmd(OssReply*& reply, RedisStep* pRedisStep);
    virtual bool SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody,
                            Step* pDoStep);

    virtual bool SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, 
                               const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    virtual bool SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);

    virtual bool SendTo(const tagMsgShell& stMsgShell, const Thrift2Pb& oThriftMsg, ThriftStep* pThriftStep = NULL);

    virtual bool SentTo(const std::string& strHost, int iPort, const std::string& strUrlPath, 
                        const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);

    virtual bool SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    virtual bool AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, 
                          const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);

    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    virtual bool AutoConnect(const std::string& strIdentify);
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerFd(const tagMsgShell& stMsgShell);
    virtual bool GetMsgShell(const std::string& strIdentify, tagMsgShell& stMsgShell);
    virtual bool SetClientData(const tagMsgShell& stMsgShell, loss::CBuffer* pBuff);
    virtual bool HadClientData(const tagMsgShell& stMsgShell);
    virtual std::string GetClientAddr(const tagMsgShell& stMsgShell);
    virtual bool AbandonConnect(const std::string& strIdentify);

    virtual bool QueryNodeTypeByCmd(std::string& sNodeType, const int iCmd);

    double GetDelInvalidNodeTmerTm() 
    {
        m_tmDelInvalidNode;
    }
protected:
    bool Init(loss::CJsonObject& oJsonConf);
    bool InitSrvNmConf(loss::CJsonObject& oJsonConf);
    bool InitLogger(const loss::CJsonObject& oJsonConf);
    bool CreateEvents();
    void PreloadCmd();
    void Destroy();
    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(int iFd);
    bool AddIoWriteEvent(int iFd);
    bool RemoveIoWriteEvent(int iFd);
    bool AddIoReadEvent(std::unordered_map<int, tagConnectionAttr*>::iterator iter);
    bool AddIoWriteEvent(std::unordered_map<int, tagConnectionAttr*>::iterator iter);
    bool RemoveIoWriteEvent(std::unordered_map<int, tagConnectionAttr*>::iterator iter);
    bool DelEvents(ev_io** io_watcher_attr);
    bool AddIoTimeout(int iFd, uint32 ulSeq, tagConnectionAttr* pConnAttr, ev_tstamp dTimeout = 1.0);
    tagConnectionAttr* CreateFdAttr(int iFd, uint32 ulSeq, loss::E_CODEC_TYPE eCodecType = loss::CODEC_PROTOBUF);
    bool DestroyConnect(std::unordered_map<int, tagConnectionAttr*>::iterator iter, bool bMsgShellNotice = true);
    void MsgShellNotice(const tagMsgShell& stMsgShell, const std::string& strIdentify, loss::CBuffer* pClientData);

    /**
     * @brief 收到完整数据包后处理
     * @note
     * <pre>
     * 框架层接收并解析到完整的数据包后调用此函数做数据处理。数据处理可能有多重不同情况出现：
     * 1. 处理成功，仍需继续解析数据包；
     * 2. 处理成功，但无需继续解析数据包；
     * 3. 处理失败，仍需继续解析数据包；
     * 4. 处理失败，但无需继续解析数据包。
     * 是否需要退出解析数据包循环体取决于Dispose()的返回值。返回true就应继续解析数据包，返回
     * false则应退出解析数据包循环体。处理过程或处理完成后，如需回复对端，则直接使用pSendBuff
     * 回复数据即可。
     * </pre>
     * @param[in] stMsgShell 数据包来源消息外壳
     * @param[in] oInMsgHead 接收的数据包头
     * @param[in] oInMsgBody 接收的数据包体
     * @param[out] oOutMsgHead 待发送的数据包头
     * @param[out] oOutMsgBody 待发送的数据包体
     * @return 是否继续解析数据包（注意不是处理结果）
     */
    bool Dispose(const tagMsgShell& stMsgShell,
                 MsgHead& oInMsgHead, MsgBody& oInMsgBody,
                 MsgHead& oOutMsgHead, MsgBody& oOutMsgBody);

    /**
     * @brief 收到完整的hhtp包后处理
     * @param stMsgShell 数据包来源消息外壳
     * @param oInHttpMsg 接收的HTTP包
     * @param oOutHttpMsg 待发送的HTTP包
     * @return 是否继续解析数据包（注意不是处理结果）
     */
    bool Dispose(const tagMsgShell& stMsgShell,
                 HttpMsg& oInHttpMsg, HttpMsg& oOutHttpMsg);

    bool ParseThriftReq(bool& isDisConn, tagIoWatcherData* pData, const MsgBody& oInMsgBody, 
                        uint32_t uiPacketLen, const uint8_t *pPacketBuf);

    bool Dispose(const tagMsgShell& stMsgShell, const Thrift2Pb& inThriftMsg, Thrift2Pb& outThriftMsg, 
                 uint32_t uiPacketLen, const uint8_t *pPacketBuf);

    void LoadSo(loss::CJsonObject& oSoConf);
    tagSo* LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteCmd(int iCmd);
    void LoadModule(loss::CJsonObject& oModuleConf);
    void LoadMethod(loss::CJsonObject& oModuleConf);

    void LoadDownStream(loss::CJsonObject& oDownStreamCnf);
    tagModule* LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, 
                                  const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteModule(const std::string& strModulePath);
    //
    tagMethod* LoadSoAndGetMethod(const std::string& strMethod, const std::string& strSoPath,
                                  const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteMethod(const std::string& strMethod);

    bool GetOptimalNextWorkIndex(const std::string& strNodeType, unsigned int& uiDstWorkIndex,
                                 std::string& strWokerIndentity,const std::string& sSendSession);
    void RegisterDelInValidPeerNodeProc(double iDelayTm);
    
    bool UpdateSendToFailStat(const std::string& sNodeType, const std::string& sIpPortWkIndex); 
    void UpdateFailReqStat(const oss::tagMsgShell& msgShell);

    void AutoDelFailNode(const std::string& strNodeType);
    void FiltNodeExistANotExistB(std::unordered_map<std::string, std::set<SerNodeInfo*> >& nodeSetA,
                                 const std::unordered_map<std::string, std::set<SerNodeInfo*> >& nodeSetB);
    //ret: 2: ip,port node exit
    //     1: snodetype node exit
    //     other: not exist
    int32_t CheckNodeInAutoDelSet(const std::string& sNodeType,
                               const std::string& ip = "",
                               int32_t port = 0);
    //ret: 2: ip,port node exit
    //     1: snodetype node exit
    //     other: not exist
    int32_t CheckInToDelNodeList(const std::string& sNodeType,
                                 const std::string& ip,
                                 int32_t port);

    bool AppendToAddNodeInList(const std::string& sNodeType,
                               const SerNodeInfo *tempNodeInfo);

    bool UpdateNodeRouteTable(const std::string& sNodeType,
                               const SerNodeInfo *tempNodeInfo);
    bool DelStepFdRelation(Step* pStep);
    bool GetStepofConnFd(Step* pStep, tagMsgShell& connShell);
    void GetNodeMinimumFreqence(SerNodeInfo*  pSerNode, std::set<SerNodeInfo*>& sSerNode);
    void ProcessReqNumToMaxUint(SerNodeInfo* pSerNode, std::set<SerNodeInfo*>& sSerNode);

    bool ParseNodeTypeCmdConf(const loss::CJsonObject& jsNodeTypeCmd);
    //juse monitor call this interface, busi worker not do.
    bool SendAlarmInfoFromManager(const loss::CJsonObject& jsAlarm);
    // all busi worker send alarm info to manager, then
    // sent to monitor by manager.
    bool SendBusiAlarmReport(const loss::CJsonObject& jsReportData); 
    //resume yeilded step on peer close or recv exception.
    void WakeYieldedStepsOnException(int iFd, uint32 ulSeq, const std::string& sErr);
private:
    bool ProcControlCmd(const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    
    bool ADDWhiteList(loss::CJsonObject& jsArrWlist, const std::string& sSrvName,
                      NodeTypeIDWhiteList& newWHL); 
    void RestartChildProc();
private:
    char* m_pErrBuff;
    uint32 m_ulSequence;
    log4cplus::Logger m_oLogger;
    bool m_bInitLogger;
    ev_tstamp m_dIoTimeout;             ///< IO（连接）超时配置
    ev_tstamp m_dStepTimeout;           ///< 步骤超时

    std::string m_strNodeType;          ///< 节点类型
    std::string m_strHostForServer;     ///< 对其他Server服务的IP地址（用于生成当前Worker标识）
    std::string m_strWorkerIdentify;    ///< 进程标识
    int m_iPortForServer;               ///< Server间通信监听端口（用于生成当前Worker标识）
    std::string m_strWorkPath;          ///< 工作路径
    loss::CJsonObject m_oCustomConf;    ///< 自定义配置
    uint32 m_uiNodeId;                  ///< 节点ID
    int m_iManagerControlFd;            ///< 与Manager父进程通信fd（控制流）
    int m_iManagerDataFd;               ///< 与Manager父进程通信fd（数据流）
    int m_iWorkerIndex;
    int m_iWorkerPid;
    ev_tstamp m_dMsgStatInterval;       ///< 客户端连接发送数据包统计时间间隔
    int32  m_iMsgPermitNum;             ///< 客户端统计时间内允许发送消息数量

    int m_iRecvNum;                     ///< 接收数据包（head+body）数量
    int m_iRecvByte;                    ///< 接收字节数（已到达应用层缓冲区）
    int m_iSendNum;                     ///< 发送数据包（head+body）数量（只到达应用层缓冲区，不一定都已发送出去）
    int m_iSendByte;                    ///< 发送字节数（已到达系统发送缓冲区，可认为已发送出去）

    struct ev_loop* m_loop;
    CmdConnectWorker* m_pCmdConnect;
    
    //std::unordered_map<loss::E_CODEC_TYPE, StarshipCodec*> m_mapCodec;   ///< 编解码器
    std::unordered_map<int32_t, StarshipCodec*> m_mapCodec;   ///< 编解码器
    typedef std::unordered_map<int32_t, StarshipCodec*>::iterator  IterTypeCodec;

    std::unordered_map<int, tagConnectionAttr*> m_mapFdAttr;   ///< 连接的文件描述符属性
    typedef std::unordered_map<int, tagConnectionAttr*>::iterator    TypeItConnAttr;

    std::unordered_map<int, uint32> m_mapInnerFd;              ///< 服务端之间连接的文件描述符（用于区分连接是服务内部还是外部客户端接入）
    std::map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）

    std::unordered_map<int32, Cmd*> m_mapCmd;                  ///< 预加载逻辑处理命令（一般为系统级命令）
    std::unordered_map<int, tagSo*> m_mapSo;         ///< 动态加载业务逻辑处理命令
    std::unordered_map<std::string, tagModule*> m_mapModule;   ///< 动态加载的http逻辑处理模块

    std::map<std::string, tagMethod*> m_mapMethod;   ///< 动态加载的thrift协议模块
    //
    std::map<int, tagSo*> m_mpDelSo;                 ///< 已经被下线的命令字，资源不立刻删除。服用下一次使用

    std::unordered_map<uint32, Step*> m_mapCallbackStep;
   
    std::map<int32, std::list<uint32> > m_mapHttpAttr;       ///< TODO 以类似处理redis回调的方式来处理http回调
    typedef std::map<int32, std::list<uint32> > TypeHttpConn; ///key: connect fd

    std::map<redisAsyncContext*, tagRedisAttr*> m_mapRedisAttr;    ///< Redis连接属性
    std::map<std::string, std::map<std::string, Session*> > m_mapCallbackSession;

    /* 节点连接相关信息 */
    //std::map<std::string, tagMsgShell> m_mapMsgShell;            // key为Identify
    std::unordered_map<std::string, tagMsgShell> m_mapMsgShell;            // key为Identify
    std::map<std::string, std::string> m_mapIdentifyNodeType;    // key为Identify，value为node_type
    T_MAP_NODE_TYPE_IDENTIFY m_mapNodeIdentify;
    std::unordered_map<std::string,std::set<SerNodeInfo*> > m_NodeType_NodeInfo; //key is Nodetype, val:
    std::unordered_map<std::string,std::set<SerNodeInfo*> > m_ToDel_NodeTypeNodeInfo; //key is Nodetype 
    std::unordered_map<std::string, std::set<SerNodeInfo*> > m_AutoDel_NodeTypeNodeInfo; // key is nodetype
    std::unordered_map<std::string, std::set<SerNodeInfo*> > m_AppendNodeTypeNodeInfo; //
    /* redis节点信息 */
    // std::map<std::string, std::set<std::string> > m_mapRedisNodeConf;        ///< redis节点配置，key为node_type，value为192.168.16.22:9988形式的IP+端口
    typedef std::unordered_map<std::string, const redisAsyncContext*>  TypeMpRedisIdContext;
    TypeMpRedisIdContext  m_mapRedisContext;       ///< redis连接，key为identify(192.168.16.22:9988形式的IP+端口)
    
    typedef std::unordered_map<const redisAsyncContext*, std::string> TypeMpRedisContextId;
    TypeMpRedisContextId  m_mapContextIdentify;    ///< redis标识，与m_mapRedisContext的key和value刚好对调
    
    std::unordered_map<Step*, tagMsgShell> m_mpSendingStepConnFd; //step和已经建立连接的关系
    StatiConnnectingStep m_mpStatiConnStep; //已经建立连接和step间关系
    
    typedef std::unordered_map<int, std::string>::iterator ITerCmdNodeType;
    std::unordered_map<int, std::string> m_mpCmdNodeType;  ///< 定义所有命令字对应部署的服务名

    typedef std::unordered_map<std::string, std::set<int> >::iterator ITerNodeTypeCmd;
    std::unordered_map<std::string, std::set<int> > m_mpNodeTypeCmd; ///< 定义所有服务名下所有的命令字
    
    bool m_isMonitor;
    std::map<std::string, CTimer*> m_mpTimers;
    NodeTypeIDWhiteList m_NodeTypeWhiteList;
    bool m_ChildRestarting;
    
    
    double m_tmDelInvalidNode;
};

} /* namespace oss */

#endif /* OSSWORKER_HPP_ */
