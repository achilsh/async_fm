/*******************************************************************************
 * Project:  AsyncServer
 * @file     OssManager.hpp
 * @brief    Oss管理者
 * @author   
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef OSSMANAGER_HPP_
#define OSSMANAGER_HPP_

#include <unordered_map> //c++11 hash map
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include "ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"

#include "util/json/CJsonObject.hpp"
#include "util/CBuffer.hpp"
#include "protocol/msg.pb.h"
#include "protocol/oss_sys.pb.h"
#include "OssError.hpp"
#include "OssDefine.hpp"
#include "Attribution.hpp"
#include "labor/OssLabor.hpp"
#include "OssWorker.hpp"
#include "cmd/Cmd.hpp"

#include "Shm/include/lib_shm.h"
///
#define SRV_NAME_CONF_FILE "/etc/srv_name/srvname.json"
#define NODETYPE_CMD_CONF_FILE "/etc/srv_name/nodetype_cmd.json"
#define WHITE_LIST_CONF_FILE   "/etc/srv_name/white_list.json"

using namespace LIB_SHM;

namespace oss
{

class CmdConnectWorker;
class OssManager;

class ShmVerCheck {
    public:
     ShmVerCheck(LIB_SHM::LibShm& shm);
     virtual ~ShmVerCheck();
     int32_t CheckShmVer(const std::string& sKey);
     void SetShmKVInitVal();
     std::string GetErrMsg() { return m_Err; }
     int64_t GetVerVal(const std::string& sKey);
    private:
     LIB_SHM::LibShm& m_ShmHandle;
     std::string m_Err;
     std::map<std::string, int64_t> m_mpShmKV;
};

struct tagClientConnWatcherData
{
    in_addr_t iAddr;
    OssManager* pManager;     // 不在结构体析构时回收

    tagClientConnWatcherData() : iAddr(0), pManager(NULL)
    {
    }
};

struct tagManagerIoWatcherData
{
    int iFd;
    uint32 ulSeq;
    OssManager* pManager;     // 不在结构体析构时回收

    tagManagerIoWatcherData() : iFd(0), ulSeq(0), pManager(NULL)
    {
    }
};

enum WorkerRestarTimesStaus {
  RESTART_TIMES_INIT = 0,
  RESTART_TIMES_USEUP = 1,
  RESTART_TIMES_CANUSE = 2
};

class CWorkerRestartStat {
 public:
    CWorkerRestartStat(const int iMaxRestarTms);
    virtual ~CWorkerRestartStat() {}
    int32_t CheckRestartTimesStatus();
    bool  UpdateRestartStatus();
    int32_t GetRestartTimes() { return m_iCurReStartTimes; }
 private:
    CWorkerRestartStat(const CWorkerRestartStat& stat);
    CWorkerRestartStat& operator = (const CWorkerRestartStat& stat);
    
    time_t m_iRestartTimeStamp;
    int    m_iCurReStartTimes;
    int    m_iMaxRestartTimes; 
    int    m_iRestartStatus;
    static const int ST_STAT_TM_INTER = 60; //用每分钟内统计失败

};

struct OssMonitor {
  int iWorkerId;
  int iPid;
  tagWorkerAttr  monWorkerAttr;
};

class NormalRestartWorker
{
 public:
    NormalRestartWorker();
    virtual ~NormalRestartWorker();
    bool AddRestartWorker(int32_t iPid);
    bool AllWorkersRestart();
    bool UpdateWorkerRestartSatus(int32_t iPid, int iStatus);
    bool IsWorkerRestarting(int32_t iPid);
    bool IsNoRestart();
private:
    void SetNoRestart();
    
    std::map<int, bool> m_mpWorkers;
    bool m_bIsRestarting;
};

//-------------------------------------//
class OssManager : public OssLabor
{
public:
    OssManager(const std::string& strConfFile);
    virtual ~OssManager();

    static void ChildTerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void SelfDefSignalCallBack(struct ev_loop* loop, struct ev_signal* watcher, int revents); 
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    
    bool ChildTerminated(struct ev_signal* watcher);
    bool SelfDefSignal(struct ev_signal* watcher);

    bool IoRead(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool FdTransfer(int iFd);
    bool AcceptServerConn(int iFd);
    bool RecvDataAndDispose(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoWrite(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoError(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoTimeout(tagManagerIoWatcherData* pData, struct ev_timer* watcher);
    bool ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, struct ev_timer* watcher);

    void Run();

public:     // Manager相关设置（由专用Cmd类调用这些方法完成Manager自身的初始化和更新）
    bool InitLogger(const loss::CJsonObject& oJsonConf);
    virtual bool SetProcessName(const loss::CJsonObject& oJsonConf);
    /** @brief 加载配置，刷新Server */
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    virtual bool SendTo(const tagMsgShell& stMsgShell);

    virtual bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    virtual bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody,oss::Step* pStep);
                        //oss::Step* pStep, MsgHead& rspMsgHead, MsgBody& rspMsgBody);

    virtual bool SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep){return(true);};
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerFd(const tagMsgShell& stMsgShell){};

    virtual bool QueryNodeTypeByCmd(std::string& sNodeType, const int iCmd);

    void SetWorkerLoad(int iPid, loss::CJsonObject& oJsonLoad);
    void AddWorkerLoad(int iPid, int iLoad = 1);
    const std::unordered_map<int, tagWorkerAttr>& GetWorkerAttr() const;

    bool GetMinLoadWorkIndex(unsigned int& workerIndex);
protected:
    bool GetConf();
    bool GetSrvNameConf();
    bool GetNodeTypeCmdConf();
    bool GetWhiteListConf();
    bool Init();
    void Destroy();
    void CreateWorker();
    bool CreateEvents();
    bool RegisterToCenter();
    bool RestartWorker(int iPid);
    
    bool RestartMonitWork(int iPid);

    bool AddIoReadEvent(std::map<int, tagConnectionAttr*>::iterator iter);
    bool AddIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter);

    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(int iFd);
    bool AddIoWriteEvent(int iFd);
    //bool AddIoErrorEvent(int iFd);
    bool RemoveIoWriteEvent(int iFd);
    bool DelEvents(ev_io** io_watcher_addr);
    bool DelEvents(ev_io* &io_watcher);
    bool AddIoTimeout(int iFd, uint32 ulSeq, ev_tstamp dTimeout = 1.0);
    bool AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout = 60.0);
    tagConnectionAttr* CreateFdAttr(int iFd, uint32 ulSeq);
    bool DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter);
    std::pair<int, int> GetMinLoadWorkerDataFd();
    bool CheckWorker();
    void RefreshServer();
    bool ReportToCenter();  // 向管理中心上报负载信息
    bool SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody);    // 向Worker发送数据
    bool DisposeDataFromWorker(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, loss::CBuffer* pSendBuff);
    bool DisposeDataAndTransferFd(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, loss::CBuffer* pSendBuff);
    bool DisposeDataFromCenter(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, loss::CBuffer* pSendBuff, loss::CBuffer* pWaitForSendBuff);
    //
    void CreateMonitorWorker();
    void SendAlarmToMonitWorker(const loss::CJsonObject& jsAlarm);
    uint32 GetSequence()
    {
        return(m_ulSequence++);
    }

    log4cplus::Logger GetLogger()
    {
        return(m_oLogger);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_uiNodeId);
    }

    void CreateSignalProcHandle();
    void ReflushHostCnf();
    void ReflushSrvCnf();
    int32_t CheckSrvNameVerIncr(); // > 0: incr, =0: not change, -1: err
    int32_t CheckNodeTypeCmdVerIncr(); // > 0: incr, =0: not change, -1: err

private:
    int32_t ShmCheckVer(const std::string& sKey);
    bool SetSocketFdKeepAlive(int iFd);
    bool SetSocketFdOpt(int iFd, int& iOptVal, int iOptName, int iLevel = IPPROTO_TCP);
    bool IsNormalRestartWork(int iExitWorkPid, int iStatus);
    void RestartAllWorkers();
private:
    loss::CJsonObject m_oLastConf;          ///< 上次加载的配置
    loss::CJsonObject m_oCurrentConf;       ///< 当前加载的配置

    loss::CJsonObject m_oLastSrvNameCnf; 
    loss::CJsonObject m_oCurrentSrvNameCnf;

    loss::CJsonObject  m_oLastNodeTypeCmdCnf;
    loss::CJsonObject  m_oCurrentNodeTypeCmdCnf;
    
    loss::CJsonObject  m_oLastWhiteListCnf;
    loss::CJsonObject  m_oCurrentWhiteListCnf;
   
    uint32 m_ulSequence;
    char* m_pErrBuff;
    log4cplus::Logger m_oLogger;
    bool m_bInitLogger;
    ev_tstamp m_dIoTimeout;             ///< IO超时配置

    std::string m_strConfFile;              ///< 配置文件
    std::string m_strWorkPath;              ///< 工作路径
    std::string m_strNodeType;              ///< 节点类型
    uint32 m_uiNodeId;                      ///< 节点ID（由center分配）
    std::string m_strHostForServer;         ///< 对其他Server服务的IP地址，对应 m_iS2SListenFd
    std::string m_strHostForClient;         ///< 对Client服务的IP地址，对应 m_iC2SListenFd
    int32 m_iPortForServer;                 ///< Server间通信监听端口，对应 m_iS2SListenFd
    int32 m_iPortForClient;                 ///< 对Client通信监听端口，对应 m_iC2SListenFd
    uint32 m_uiWorkerNum;                   ///< Worker子进程数量
    loss::E_CODEC_TYPE m_eCodec;            ///< 接入端编解码器
    ev_tstamp m_dAddrStatInterval;          ///< IP地址数据统计时间间隔
    int32  m_iAddrPermitNum;                ///< IP地址统计时间内允许连接次数
    int m_iLogLevel;
    int m_iRefreshInterval;                 ///< 刷新Server的间隔周期
    int m_iLastRefreshCalc;                 ///< 上次刷新Server后的运行周期数
    int m_iRestartConf;                  ///< 是否重启workers in conf.

    int m_iS2SListenFd;                     ///< Server to Server监听文件描述符（Server与Server之间的连接较少，但每个Server的每个Worker均与其他Server的每个Worker相连）
    int m_iC2SListenFd;                     ///< Client to Server监听文件描述符（Client与Server之间的连接较多，但每个Client只需连接某个Server的某个Worker）
    struct ev_loop* m_loop;
//    CmdConnectWorker* m_pCmdConnect;

    std::unordered_map<int, tagWorkerAttr> m_mapWorker;       ///< 业务逻辑工作进程及进程属性，key为pid
    //std::map<int, int> m_mapWorkerRestartNum;       ///< 进程被重启次数，key为WorkerIdx
    std::map<int, CWorkerRestartStat*> m_mapWorkerRestartNum;       ///< 进程被重启次数，key为WorkerIdx
    std::map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号,用于管理本服务worker和manager间通信.
    std::map<int, int> m_mapWorkerIdPid;            ///< 子进程编号对应的进程号
    std::map<std::string, tagMsgShell> m_mapCenterMsgShell; ///< 到center服务器的连接

    std::map<int, tagConnectionAttr*> m_mapFdAttr;  ///< 连接的文件描述符属性
    std::map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）
    std::unordered_map<in_addr_t, uint32> m_mapClientConnFrequency; ///< 客户端连接频率
    std::map<int32, Cmd*> m_mapCmd;

    std::vector<int> m_vecFreeWorkerIdx;            ///< 空闲进程编号
    bool m_bReloadCnf;
    LIB_SHM::LibShm m_srvNameShmFd;
    //
    typedef std::map<int, std::string>::iterator ITerCmdNodeType;
    std::map<int, std::string> m_mpCmdNodeType;  ///< 定义所有命令字对应部署的服务名
    std::map<std::string, std::set<int> > m_mpNodeTypeCmd; ///< 定义所有服务名下所有的命令字

    //
    OssMonitor m_WorkMonitor; //use single process to moitor all child process;
    ShmVerCheck m_shmCheck;

    NormalRestartWorker m_workersNormalRestart; // workers restart container
};

} /* namespace oss */

#endif /* OSSMANAGER_HPP_ */
