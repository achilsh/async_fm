/*******************************************************************************
 * Project:  AsyncServer
 * @file     OssManager.cpp
 * @brief 
 * @author   
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#include "unix_util/proctitle_helper.h"
#include "unix_util/process_helper.h"
#include "unix_util/SocketOptSet.hpp"
#include "OssManager.hpp"
#include "cmd/sys_cmd/CmdConnectWorker.hpp"
#include <sys/socket.h>

namespace oss
{

ShmVerCheck::ShmVerCheck(LIB_SHM::LibShm& shm) :m_ShmHandle(shm) {
    SetShmKVInitVal();
}

ShmVerCheck::~ShmVerCheck() {
}

void ShmVerCheck::SetShmKVInitVal() {
    m_mpShmKV[SRV_NAME_CONF_FILE] = 0;
    m_mpShmKV[NODETYPE_CMD_CONF_FILE] = 0;
    m_mpShmKV[WHITE_LIST_CONF_FILE] = 0;
}

//return 1: ver change; 0: no change, -1: error
int32_t ShmVerCheck::CheckShmVer(const std::string& sKey) {
  int64_t iLVerNo = 0;
  m_Err.clear();
  if (false == m_ShmHandle.GetValue(sKey, iLVerNo)) {
      m_Err = m_ShmHandle.GetErrMsg();
      return  -1;
  }
  if (m_mpShmKV[sKey] != iLVerNo) {
      m_mpShmKV[sKey] = iLVerNo; 
      return  1;
  }
  return 0;
}

int64_t ShmVerCheck::GetVerVal(const std::string& sKey)  {
    return m_mpShmKV[sKey];
}
///////

const int CWorkerRestartStat::ST_STAT_TM_INTER; 

CWorkerRestartStat::CWorkerRestartStat(const int iMaxRestarTms)
  :m_iRestartTimeStamp(::time(NULL)), m_iCurReStartTimes(0), 
    m_iMaxRestartTimes(iMaxRestarTms), m_iRestartStatus(RESTART_TIMES_INIT) {
}

int32_t CWorkerRestartStat::CheckRestartTimesStatus() {
  if (::time(NULL) - m_iRestartTimeStamp < ST_STAT_TM_INTER) {
    if (m_iCurReStartTimes + 1  >=  m_iMaxRestartTimes) {
      m_iRestartStatus = RESTART_TIMES_USEUP;
      m_iRestartTimeStamp = ::time(NULL);

    } else {
      m_iRestartStatus = RESTART_TIMES_CANUSE;
    }

  } else {
    m_iRestartTimeStamp = ::time(NULL);
    m_iCurReStartTimes = 0;
    m_iRestartStatus = RESTART_TIMES_INIT;
  }
  return m_iRestartStatus;
}

bool  CWorkerRestartStat::UpdateRestartStatus () {
  m_iCurReStartTimes ++;
  m_iRestartStatus = RESTART_TIMES_CANUSE;
  return true;
}

void OssManager::ChildTerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        OssManager* pManager = (OssManager*)watcher->data;
        pManager->ChildTerminated(watcher);
    }
}

void OssManager::SelfDefSignalCallBack(struct ev_loop* loop, struct ev_signal* watcher, int revents) {
  if (watcher->data != NULL)
  {
    OssManager* pManager = (OssManager*)watcher->data;
    pManager->SelfDefSignal(watcher);
  }
}

void OssManager::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        OssManager* pManager = (OssManager*)watcher->data;
        pManager->CheckWorker();
        pManager->ReportToCenter();
    }
}

void OssManager::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)watcher->data;
        OssManager* pManager = pData->pManager;
        if (revents & EV_READ)
        {
            pManager->IoRead(pData, watcher);
        }
        if (revents & EV_WRITE)
        {
            pManager->IoWrite(pData, watcher);
        }
        if (revents & EV_ERROR)
        {
            pManager->IoError(pData, watcher);
        }
    }
}

void OssManager::IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)watcher->data;
        OssManager* pManager = pData->pManager;
        pManager->IoTimeout(pData, watcher);
    }
}

void OssManager::PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        OssManager* pManager = (OssManager*)(watcher->data);
#ifndef NODE_TYPE_CENTER
        //pManager->ReportToCenter();
        pManager->CheckWorker();
        pManager->RefreshServer();
#endif
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void OssManager::ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagClientConnWatcherData* pData = (tagClientConnWatcherData*)watcher->data;
        OssManager* pManager = pData->pManager;
        pManager->ClientConnFrequencyTimeout(pData, watcher);
    }
}

OssManager::OssManager(const std::string& strConfFile)
    : m_ulSequence(0), m_pErrBuff(NULL), m_bInitLogger(false), m_dIoTimeout(480), m_strConfFile(strConfFile),
      m_uiNodeId(0), m_iPortForServer(9988), m_iPortForClient(9987), m_uiWorkerNum(10),
      m_eCodec(loss::CODEC_PROTOBUF), m_dAddrStatInterval(60.0), m_iAddrPermitNum(10),
      m_iLogLevel(log4cplus::INFO_LOG_LEVEL), m_iRefreshInterval(60), m_iLastRefreshCalc(0),
      m_iS2SListenFd(-1), m_iC2SListenFd(-1), m_loop(NULL)
      ,m_srvNameShmFd(OP_R), m_shmCheck(m_srvNameShmFd)
{
    m_iRestartConf = 0;
    
    if (strConfFile == "")
    {
        std::cerr << "error: no config file!" << std::endl;
        exit(1);
    }

    if (!GetConf())
    {
        std::cerr << "GetConf() error!" << std::endl;
        exit(-1);
    }

    if (!GetSrvNameConf()) 
    {
      std::cerr << "GetSrvNameConf() error!" << std::endl;
    }
    if (!GetNodeTypeCmdConf()) 
    {
      std::cerr << "GetNodeTypeCmdConf() error! " << std::endl;
    }

    if (!GetWhiteListConf()) 
    {
      std::cerr << "GetWhiteListConf() error! " << std::endl;
    }

    m_pErrBuff = new char[gc_iErrBuffLen];
    ngx_setproctitle(m_oCurrentConf("server_name").c_str());
    daemonize(m_oCurrentConf("server_name").c_str());
    
    Init();

    CreateEvents();
    CreateWorker();

    CreateMonitorWorker();

    #ifdef USE_CENTER_NODE
    RegisterToCenter();
    #endif
}

OssManager::~OssManager()
{
    Destroy();
}

bool OssManager::ChildTerminated(struct ev_signal* watcher)
{
    pid_t   iPid = 0;
    int     iStatus = 0;
    int     iReturnCode = 0;
    std::map<int, int> mpPidStatus;
    //WUNTRACED
    while((iPid = waitpid(-1, &iStatus, WNOHANG)) > 0)
    {
        if (WIFEXITED(iStatus))
        {
            iReturnCode = WEXITSTATUS(iStatus);
        }
        else if (WIFSIGNALED(iStatus))
        {
            iReturnCode = WTERMSIG(iStatus);
        }
        else if (WIFSTOPPED(iStatus))
        {
            iReturnCode = WSTOPSIG(iStatus);
        }

        LOG4_WARN("error: [%d],worker pid: [%d] exit and sent signal: [%d] with code: [%d]!",
                  iStatus, iPid, watcher->signum, iReturnCode);

        mpPidStatus.insert(std::make_pair(iPid, iStatus));
    }

    for(std::map<int, int>::iterator it = mpPidStatus.begin(); it != mpPidStatus.end(); ) 
    {
        int iPid = it->first;
        int iStatus = it->second;
        if (iPid == m_WorkMonitor.iPid) {
            RestartMonitWork(iPid);
            mpPidStatus.erase(it++);
        } 
        else 
        {
            if (m_workersNormalRestart.UpdateWorkerRestartSatus(iPid, iStatus))
            {
                RestartWorker(iPid);
                if (m_workersNormalRestart.IsNoRestart())
                {
                    LOG4_TRACE("recv signal for restart all workers done");
                    return true;
                }
                mpPidStatus.erase(it++);
                continue;
            }

            loss::CJsonObject  jsAlarm;
            jsAlarm.Add("worker_pid", iPid);
            jsAlarm.Add("err_code", iReturnCode);
            jsAlarm.Add("recv_signal", watcher->signum);
            jsAlarm.Add("worker_id", m_mapWorker[iPid].iWorkerIndex);
            jsAlarm.Add("node_type", m_strNodeType);
            jsAlarm.Add("ip", m_strHostForServer);
            jsAlarm.Add("err_msg", "core dump");

            SendAlarmToMonitWorker(jsAlarm);
            RestartWorker(iPid);
            mpPidStatus.erase(it++);
        }
    }
    return(true);
}

bool OssManager::IsNormalRestartWork(int iExitWorkPid,int iStatus)
{
    return false;
}

void OssManager::SendAlarmToMonitWorker(const loss::CJsonObject& jsAlarm) 
{
    if (jsAlarm.IsEmpty()) {
        LOG4_ERROR("send alarm to monitor data is empty");
        return ;
    }

    MsgHead oMsgHead;
    MsgBody oMsgBody;
    oMsgBody.set_body(jsAlarm.ToString());
    oMsgHead.set_cmd(CMD_REQ_ALARM_SEDN_MON);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());

    int iErrno = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = 0;
    tagWorkerAttr& stWorkerAttr = m_WorkMonitor.monWorkerAttr;
    std::map<int, tagConnectionAttr*>::iterator worker_conn_iter;
    worker_conn_iter = m_mapFdAttr.find(stWorkerAttr.iControlFd);
    if (worker_conn_iter != m_mapFdAttr.end()) {
        worker_conn_iter->second->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
        worker_conn_iter->second->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
        iNeedWriteLen = worker_conn_iter->second->pSendBuff->ReadableBytes();
        LOG4_DEBUG("send cmd %d seq %llu to worker %d", oMsgHead.cmd(), oMsgHead.seq(), stWorkerAttr.iWorkerIndex);
        iWriteLen = worker_conn_iter->second->pSendBuff->WriteFD(worker_conn_iter->first, iErrno);
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_ERROR("send to fd %d error %d: %s",
                           worker_conn_iter->first, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            }
            else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
            {
                worker_conn_iter->second->dActiveTime = ev_now(m_loop);
                AddIoWriteEvent(worker_conn_iter);
            }
        }
        else if (iWriteLen > 0)
        {
            worker_conn_iter->second->dActiveTime = ev_now(m_loop);
            if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
            {
                LOG4_DEBUG("send to worker %d success, data len %d, cmd %d seq %llu",
                           stWorkerAttr.iWorkerIndex, iWriteLen, oMsgHead.cmd(), oMsgHead.seq());
                RemoveIoWriteEvent(worker_conn_iter->first);
            }
            else    // 内容未写完，添加或保持监听fd写事件
            {
                AddIoWriteEvent(worker_conn_iter);
            }
        }
        worker_conn_iter->second->pSendBuff->Compact(32784);
    } else {
        LOG4_ERROR("not find fd for manager to monitor");
    }
}


bool OssManager::SelfDefSignal(struct ev_signal* watcher) {
  if (watcher->signum == SIGUSR1) 
  {
    ReflushHostCnf();
    LOG4_INFO("reload srv host conf by self def signal SIGUSR1 done");
  }
  else if (watcher->signum == SIGUSR2) 
  {
    ReflushSrvCnf();
    m_iLastRefreshCalc = 0;
    LOG4_INFO("reload srv name, node type,white list conf by self signal SIGUSR2 done");
  } else {
    LOG4_INFO("no define signal process");
  }
  return true;
}
////

bool OssManager::IoRead(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (watcher->fd == m_iS2SListenFd)
    {
        return(AcceptServerConn(watcher->fd));
    }
#ifdef NODE_TYPE_ACCESS
    else if (watcher->fd == m_iC2SListenFd)
    {
        return(FdTransfer(watcher->fd));
    }
#endif
    else
    {
       return(RecvDataAndDispose(pData, watcher));
    }
}

bool OssManager::FdTransfer(int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        return(false);
    }
    strncpy(szIpAddr, inet_ntoa(stClientAddr.sin_addr), 16);
    
    SetSocketFdKeepAlive(iAcceptFd);
   
    int iKeepIdle = 1000;
    SetSocketFdOpt(iAcceptFd, iKeepIdle, TCP_KEEPIDLE);

    int iKeepInterval = 10; 
    SetSocketFdOpt(iAcceptFd, iKeepInterval, TCP_KEEPINTVL);

    int iKeepCount = 3;
    SetSocketFdOpt(iAcceptFd, iKeepCount, TCP_KEEPCNT);

    int iTcpNoDelay = 1; //关闭Nagle算法
    SetSocketFdOpt(iAcceptFd, iTcpNoDelay, TCP_NODELAY);

    std::unordered_map<in_addr_t, uint32>::iterator iter = m_mapClientConnFrequency.find(stClientAddr.sin_addr.s_addr);
    if (iter == m_mapClientConnFrequency.end())
    {
        m_mapClientConnFrequency.insert(std::pair<in_addr_t, uint32>(stClientAddr.sin_addr.s_addr, 1));
        AddClientConnFrequencyTimeout(stClientAddr.sin_addr.s_addr, m_dAddrStatInterval);
    }
    else
    {
        iter->second++;
#ifdef NODE_TYPE_ACCESS
        if (iter->second > (uint32)m_iAddrPermitNum)
        {
            LOG4_ERROR("client addr %d had been connected more than %u times in %f seconds, it's not permitted",
                      stClientAddr.sin_addr.s_addr, m_iAddrPermitNum, m_dAddrStatInterval);
            ::close(iAcceptFd);
            //FixMe: 是否需要延长拒接的时间长度
            return(false);
        }
#endif
    }

    std::pair<int, int> worker_pid_Datafd = GetMinLoadWorkerDataFd();
    if (worker_pid_Datafd.second > 0)
    {
        LOG4_DEBUG("send new fd %d to worker communication fd %d",
                   iAcceptFd, worker_pid_Datafd.second);
        int iCodec = m_eCodec;
        //
        int iErrno = send_fd_with_attr(worker_pid_Datafd.second, iAcceptFd, szIpAddr, 16, iCodec);
        if (iErrno == 0)
        {
            AddWorkerLoad(worker_pid_Datafd.first);
            close(iAcceptFd);
            return(true);
        }
        else
        {
            LOG4_ERROR("error %d: %s", iErrno, strerror_r(iErrno, m_pErrBuff, 1024));
        }
    }
    ::close(iAcceptFd);
    return(false);
}

bool OssManager::AcceptServerConn(int iFd)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        return(false);
    }
    else
    {
        SetSocketFdKeepAlive(iAcceptFd);
        
        int iKeepIdle = 60;
        SetSocketFdOpt(iAcceptFd, iKeepIdle, TCP_KEEPIDLE, SOL_TCP);
        
        int iKeepInterval = 5;
        SetSocketFdOpt(iAcceptFd, iKeepInterval, TCP_KEEPINTVL, SOL_TCP);
        
        int iKeepCount = 3;
        SetSocketFdOpt(iAcceptFd, iKeepCount, TCP_KEEPCNT, SOL_TCP);
        
        int iTcpNoDelay = 1;
        SetSocketFdOpt(iAcceptFd, iTcpNoDelay, TCP_NODELAY);
        
        uint32 ulSeq = GetSequence();
        x_sock_set_block(iAcceptFd, 0);
        if (CreateFdAttr(iAcceptFd, ulSeq))
        {
            std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iAcceptFd);
            if(AddIoTimeout(iAcceptFd, ulSeq))     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            {
                if (!AddIoReadEvent(iter))
                {
                    DestroyConnect(iter);
                    return(false);
                }
                return(true);
            }
            else
            {
                DestroyConnect(iter);
                return(false);
            }
        }
        else    // 没有足够资源分配给新连接，直接close掉
        {
            close(iAcceptFd);
        }
    }
    return(false);
}

bool OssManager::RecvDataAndDispose(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_DEBUG("func: %s, fd: %d, seq: %llu",  __FUNCTION__, pData->iFd, pData->ulSeq);
    int iErrno = 0;
    int iReadLen = 0;
    std::map<int, tagConnectionAttr*>::iterator conn_iter;
    conn_iter = m_mapFdAttr.find(pData->iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd attr for %d!", pData->iFd);
    }
    else
    {
        if (pData->ulSeq != conn_iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %llu not match the conn attr seq %llu",
                            pData->ulSeq, conn_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pManager = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        iReadLen = conn_iter->second->pRecvBuff->ReadFD(pData->iFd, iErrno);
        LOG4_TRACE("recv from fd %d data len %d. "
                   "and conn_iter->second->pRecvBuff->ReadableBytes() = %d", pData->iFd, iReadLen,
                   conn_iter->second->pRecvBuff->ReadableBytes());
        if (iReadLen > 0)
        {
            MsgHead oInMsgHead;
            MsgBody oInMsgBody;
            while (conn_iter->second->pRecvBuff->ReadableBytes() >= gc_uiMsgHeadSize)
            {
                LOG4_TRACE("conn_iter->second->pRecvBuff->ReadableBytes() = %d, iHeadSize = %d",
                                conn_iter->second->pRecvBuff->ReadableBytes(), gc_uiMsgHeadSize);
                bool bResult = oInMsgHead.ParseFromArray(conn_iter->second->pRecvBuff->GetRawReadBuffer(), gc_uiMsgHeadSize);
                if (bResult)
                {
                    if (conn_iter->second->pRecvBuff->ReadableBytes() >= gc_uiMsgHeadSize + oInMsgHead.msgbody_len())
                    {
                        if (0 == oInMsgHead.msgbody_len())  // 无包体的数据包
                        {
                            bResult = true;
                        }
                        else
                        {
                            bResult = oInMsgBody.ParseFromArray(
                                            conn_iter->second->pRecvBuff->GetRawReadBuffer() + gc_uiMsgHeadSize, oInMsgHead.msgbody_len());
                        }
                        if (bResult)
                        {
                            conn_iter->second->dActiveTime = ev_now(m_loop);
                            bool bContinue = false;     // 是否继续解析下一个数据包
                            tagMsgShell stMsgShell;
                            stMsgShell.iFd = pData->iFd;
                            stMsgShell.ulSeq = conn_iter->second->ulSeq;

                            std::map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(watcher->fd);
                            if (worker_fd_iter == m_mapWorkerFdPid.end())   // 其他Server发过来要将连接传送到某个指定Worker进程信息
                            {
                                std::map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(conn_iter->second->strIdentify);
                                if (center_iter == m_mapCenterMsgShell.end())       // 非与center连接; 同类服务节点发送来的数据
                                {
                                    bContinue = DisposeDataAndTransferFd(stMsgShell, oInMsgHead, oInMsgBody, conn_iter->second->pSendBuff);
                                }
                                else
                                {
                                    bContinue = DisposeDataFromCenter(stMsgShell, oInMsgHead, oInMsgBody,
                                                    conn_iter->second->pSendBuff, conn_iter->second->pWaitForSendBuff);
                                }
                            }
                            else    // 本节点中的Worker进程发过来的消息
                            {
                                bContinue = DisposeDataFromWorker(stMsgShell, oInMsgHead, oInMsgBody, conn_iter->second->pSendBuff);
                            }
                            conn_iter->second->pRecvBuff->SkipBytes(gc_uiMsgHeadSize + oInMsgBody.ByteSize());
                            conn_iter->second->pRecvBuff->Compact(32784);   // 超过32KB则重新分配内存
                            conn_iter->second->pSendBuff->Compact(32784);
                            if (!bContinue)
                            {
                                DestroyConnect(conn_iter);
                                return(false);
                            }
                        }
                        else
                        {
                            LOG4_ERROR("oInMsgBody.ParseFromArray() failed, data is broken from fd %d, close it!", pData->iFd);
                            DestroyConnect(conn_iter);
                            break;
                        }
                    }
                    else
                    {
                        break;  // 头部数据已完整，但body部分数据不完整
                    }
                }
                else
                {
                    LOG4_ERROR("oInMsgHead.ParseFromArray() failed, data is broken from fd %d, close it!", pData->iFd);
                    DestroyConnect(conn_iter);
                    break;
                }
            }
            return(true);
        }
        else if (iReadLen == 0)
        {
            LOG4_DEBUG("fd %d closed by peer, error %d %s!",
                       pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            DestroyConnect(conn_iter);
        }
        else
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_ERROR("recv from fd %d error %d: %s",
                                pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                DestroyConnect(conn_iter);
            }
        }
    }
    return(true);
}

bool OssManager::IoWrite(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s(%d)", __FUNCTION__, pData->iFd);
    std::map<int, tagConnectionAttr*>::iterator attr_iter =  m_mapFdAttr.find(pData->iFd);
    if (attr_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (pData->ulSeq != attr_iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %llu not match the conn attr seq %llu",
                            pData->ulSeq, attr_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pManager = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        int iErrno = 0;
        int iWriteLen = 0;
        iWriteLen = attr_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
        LOG4_TRACE("iWriteLen = %d, send to fd %d error %d: %s", iWriteLen,
                        pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_ERROR("send to fd %d error %d: %s",
                                pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                DestroyConnect(attr_iter);
            }
            else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
            {
                attr_iter->second->dActiveTime = ev_now(m_loop);
                AddIoWriteEvent(attr_iter);
            }
        }
        else if (iWriteLen > 0)
        {
            attr_iter->second->dActiveTime = ev_now(m_loop);
            if (iWriteLen == (int)attr_iter->second->pSendBuff->ReadableBytes())  // 已无内容可写，取消监听fd写事件
            {
                RemoveIoWriteEvent(pData->iFd);
            }
            else    // 内容未写完，添加或保持监听fd写事件
            {
                AddIoWriteEvent(attr_iter);
            }
        }
        else    // iWriteLen == 0 写缓冲区为空
        {
//            LOG4_TRACE("pData->iFd %d, watcher->fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            pData->iFd, watcher->fd, attr_iter->second->pWaitForSendBuff->ReadableBytes());
            if (attr_iter->second->pWaitForSendBuff->ReadableBytes() > 0)    // 存在等待发送的数据，说明本次写事件是connect之后的第一个写事件
            {
                std::map<uint32, int>::iterator index_iter = m_mapSeq2WorkerIndex.find(attr_iter->second->ulSeq);
                if (index_iter != m_mapSeq2WorkerIndex.end())
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    //AddInnerFd(stMsgShell); 只有Worker需要
                    std::map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(attr_iter->second->strIdentify);
                    if (center_iter == m_mapCenterMsgShell.end())
                    {
                        m_mapCenterMsgShell.insert(std::pair<std::string, tagMsgShell>(attr_iter->second->strIdentify, stMsgShell));
                    }
                    else
                    {
                        center_iter->second = stMsgShell;
                    }
                    //m_pCmdConnect->Start(stMsgShell, index_iter->second);
                    MsgHead oMsgHead;
                    MsgBody oMsgBody;
                    ConnectWorker oConnWorker;
                    oConnWorker.set_worker_index(index_iter->second);
                    oMsgBody.set_body(oConnWorker.SerializeAsString());
                    oMsgHead.set_cmd(CMD_REQ_CONNECT_TO_WORKER);
                    oMsgHead.set_seq(GetSequence());
                    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
                    m_mapSeq2WorkerIndex.erase(index_iter);
                    LOG4_DEBUG("send after connect");
                    SendTo(stMsgShell, oMsgHead, oMsgBody);
                }
            }
        }
        return(true);
    }
}

bool OssManager::IoError(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (pData->ulSeq != iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %llu not match the conn attr seq %llu",
                            pData->ulSeq, iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pManager = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        DestroyConnect(iter);
    }

    std::map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(pData->iFd);
    if (worker_fd_iter != m_mapWorkerFdPid.end())
    {
        kill(worker_fd_iter->first, SIGINT);
    }
    return(true);
}

bool OssManager::IoTimeout(tagManagerIoWatcherData* pData, struct ev_timer* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    bool bRes = false;
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        bRes = false;
    }
    else
    {
        if (iter->second->ulSeq != pData->ulSeq)      // 文件描述符数值相等，但已不是原来的文件描述符
        {
            bRes = false;
        }
        else
        {
            ev_tstamp after = iter->second->dActiveTime - ev_now(m_loop) + m_dIoTimeout;
            if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
            {
                ev_timer_stop (m_loop, watcher);
                ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
                ev_timer_start (m_loop, watcher);
                return(true);
            }
            else    // IO已超时，关闭文件描述符并清理相关资源
            {
                LOG4_DEBUG("%s()", __FUNCTION__);
                DestroyConnect(iter);
            }
            bRes = true;
        }
    }

    ev_timer_stop(m_loop, watcher);
    pData->pManager = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

bool OssManager::ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, struct ev_timer* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    bool bRes = false;
    std::unordered_map<in_addr_t, uint32>::iterator iter = m_mapClientConnFrequency.find(pData->iAddr);
    if (iter == m_mapClientConnFrequency.end())
    {
        bRes = false;
    }
    else
    {
        m_mapClientConnFrequency.erase(iter);
        bRes = true;
    }

    ev_timer_stop(m_loop, watcher);
    pData->pManager = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

void OssManager::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

bool OssManager::InitLogger(const loss::CJsonObject& oJsonConf)
{
    if (m_bInitLogger)  // 已经被初始化过，只修改日志级别
    {
        int32 iLogLevel = 0;
        oJsonConf.Get("log_level", iLogLevel);
        m_oLogger.setLogLevel(iLogLevel);
        return(true);
    }
    else
    {
        int32 iMaxLogFileSize = 0;
        int32 iMaxLogFileNum = 0;
        int32 iLogLevel = 0;
        std::string strLogname = oJsonConf("log_path") + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_level", iLogLevel);
        log4cplus::initialize();
        log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        append->setName(strLogname);
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        append->setLayout(layout);
        //log4cplus::Logger::getRoot().addAppender(append);
        m_oLogger = log4cplus::Logger::getInstance(strLogname);
        m_oLogger.addAppender(append);
        m_oLogger.setLogLevel(iLogLevel);
        LOG4_INFO("%s begin...", oJsonConf("server_name").c_str());
        m_bInitLogger = true;
        return(true);
    }
}

bool OssManager::SetProcessName(const loss::CJsonObject& oJsonConf)
{
    ngx_setproctitle(oJsonConf("server_name").c_str());
    return(true);
}

void OssManager::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool OssManager::SendTo(const tagMsgShell& stMsgShell)
{
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            int iErrno = 0;
            int iWriteLen = 0;
            int iNeedWriteLen = (int)(iter->second->pWaitForSendBuff->ReadableBytes());
            int iWriteIdx = iter->second->pSendBuff->GetWriteIndex();
            iWriteLen = iter->second->pSendBuff->Write(iter->second->pWaitForSendBuff, iter->second->pWaitForSendBuff->ReadableBytes());
            if (iWriteLen == iNeedWriteLen)
            {
                iNeedWriteLen = (int)iter->second->pSendBuff->ReadableBytes();
                iWriteLen = iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(stMsgShell.iFd);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(iter);
                    }
                }
                return(true);
            }
            else
            {
                LOG4_ERROR("write to send buff error, iWriteLen = %d!", iWriteLen);
                iter->second->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
        }
    }
    return(false);
}

bool OssManager::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody,oss::Step* pStep)
                        //oss::Step* pStep, MsgHead& rspMsgHead, MsgBody& rspMsgBody)
{
    return true;
}

bool OssManager::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(cmd %u, seq %u)", __FUNCTION__, oMsgHead.cmd(), oMsgHead.seq());
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            int iErrno = 0;
            int iWriteLen = 0;
            int iNeedWriteLen = oMsgHead.ByteSize();
            int iWriteIdx = iter->second->pSendBuff->GetWriteIndex();
            iWriteLen = iter->second->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("write to send buff error, iWriteLen = %d!", iWriteLen);
                iter->second->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
            iNeedWriteLen = oMsgBody.ByteSize();
            iWriteLen = iter->second->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            if (iWriteLen == iNeedWriteLen)
            {
                iNeedWriteLen = (int)iter->second->pSendBuff->ReadableBytes();
                iWriteLen = iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                LOG4_TRACE("iWriteLen = %d, send to fd %d error %d: %s", iWriteLen,
                                stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(stMsgShell.iFd);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(iter);
                    }
                }
                return(true);
            }
            else
            {
                LOG4_ERROR("write to send buff error!");
                iter->second->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %llu not match the sequence %llu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, iter->second->ulSeq);
            return(false);
        }
    }
}

bool OssManager::SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            iter->second->strIdentify = strIdentify;
            return(true);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %llu not match the sequence %llu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, iter->second->ulSeq);
            return(false);
        }
    }
}

bool OssManager::AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);

    std::map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(iFd);
    if (worker_fd_iter != m_mapWorkerFdPid.end())
    {
        LOG4_TRACE("iFd = %d found in m_mapWorkerFdPid", iFd);
    }

    x_sock_set_block(iFd, 0);
    int reuse = 1;
    ::setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
        if(AddIoTimeout(iFd, ulSeq, 1.5))
        {
            if (!AddIoReadEvent(iter))
            {
                DestroyConnect(iter);
                return(false);
            }
            if (!AddIoWriteEvent(iter))
            {
                DestroyConnect(iter);
                return(false);
            }
            iter->second->pWaitForSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
            iter->second->pWaitForSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            iter->second->strIdentify = strIdentify;
            LOG4_TRACE("fd %d seq %u identify %s."
                            "iter->second->pWaitForSendBuff->ReadableBytes()=%u", iFd, ulSeq, strIdentify.c_str(),
                            iter->second->pWaitForSendBuff->ReadableBytes());
            m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
            std::map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(strIdentify);
            if (center_iter != m_mapCenterMsgShell.end())
            {
                center_iter->second.iFd = iFd;
                center_iter->second.ulSeq = ulSeq;
            }
            connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            return(true);
        }
        else
        {
            DestroyConnect(iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool OssManager::GetConf()
{
    char szFilePath[256] = {0};
    //char szFileName[256] = {0};
    if (m_strWorkPath.length() == 0)
    {
        if (getcwd(szFilePath, sizeof(szFilePath)))
        {
            m_strWorkPath = szFilePath;
            //std::cout << "work dir: " << m_strWorkPath << std::endl;
        }
        else
        {
            return(false);
        }
    }
    m_oLastConf = m_oCurrentConf;
    //snprintf(szFileName, sizeof(szFileName), "%s/%s", m_strWorkPath.c_str(), m_strConfFile.c_str());
    std::ifstream fin(m_strConfFile.c_str());
    if (fin.good())
    {
        std::stringstream ssContent;
        ssContent << fin.rdbuf();
        if (!m_oCurrentConf.Parse(ssContent.str()))
        {
            ssContent.str("");
            fin.close();
            m_oCurrentConf = m_oLastConf;
            return(false);
        }
        ssContent.str("");
        fin.close();
    }
    else
    {
        return(false);
    }

    if (m_oLastConf.ToString() != m_oCurrentConf.ToString())
    {
        m_oCurrentConf.Get("io_timeout", m_dIoTimeout);
        if (m_oLastConf.ToString().length() == 0)
        {
            if (m_oCurrentConf("process_num").empty()) {
              m_uiWorkerNum = PROC_NUM_WORKER_DEF;
            } else {
              m_uiWorkerNum = strtoul(m_oCurrentConf("process_num").c_str(), NULL, 10);
            }
            m_oCurrentConf.Get("node_type", m_strNodeType);
            m_oCurrentConf.Get("inner_host", m_strHostForServer);
            m_oCurrentConf.Get("inner_port", m_iPortForServer);
            m_oCurrentConf.Get("access_host", m_strHostForClient);
            m_oCurrentConf.Get("access_port", m_iPortForClient);
        }
        int32 iCodec;
        if (m_oCurrentConf.Get("access_codec", iCodec))
        {
            m_eCodec = loss::E_CODEC_TYPE(iCodec);
        }
        m_oCurrentConf["permission"]["addr_permit"].Get("stat_interval", m_dAddrStatInterval);
        m_oCurrentConf["permission"]["addr_permit"].Get("permit_num", m_iAddrPermitNum);

        int isRestartWorkers = 0;
        m_iRestartConf = 0;
        if (m_oCurrentConf.Get("restart_worker", isRestartWorkers))
        {
            m_iRestartConf = isRestartWorkers;
        }
        if (m_oCurrentConf.Get("log_level", m_iLogLevel))
        {
            switch (m_iLogLevel)
            {
                case log4cplus::DEBUG_LOG_LEVEL:
                    break;
                case log4cplus::INFO_LOG_LEVEL:
                    break;
                case log4cplus::TRACE_LOG_LEVEL:
                    break;
                case log4cplus::WARN_LOG_LEVEL:
                    break;
                case log4cplus::ERROR_LOG_LEVEL:
                    break;
                case log4cplus::FATAL_LOG_LEVEL:
                    break;
                default:
                    m_iLogLevel = log4cplus::INFO_LOG_LEVEL;
            }
        }
        else
        {
            m_iLogLevel = log4cplus::INFO_LOG_LEVEL;
        }
    }

    return(true);
}

bool OssManager::GetWhiteListConf() {
    m_oLastWhiteListCnf = m_oCurrentWhiteListCnf;
    //just read white list conf file. 
    std::ifstream srvFin(WHITE_LIST_CONF_FILE);
    if (srvFin.good()){
        std::stringstream ssContent;
        ssContent << srvFin.rdbuf();
        std::string sContent = ssContent.str();
        ssContent.str("");
        
        if (sContent.empty())
        {
            std::cerr << "not config white list" << std::endl;
            return true;
        }
        if (!m_oCurrentWhiteListCnf.Parse(sContent)) {
            srvFin.close();
            m_oCurrentWhiteListCnf = m_oLastWhiteListCnf;
            std::cerr << "load white list cnf failed,file: "<< WHITE_LIST_CONF_FILE << std::endl;
            return false;
        }
        srvFin.close();
    } else {
        std::cerr << "load white list cnf failed,file: "<< WHITE_LIST_CONF_FILE << std::endl;
        return false;
    }
    return true;
}

bool OssManager::GetSrvNameConf() 
{
    m_oLastSrvNameCnf = m_oCurrentSrvNameCnf;
    //just read srv name conf file. 
    std::ifstream srvFin(SRV_NAME_CONF_FILE);
    if (srvFin.good())
    {
        std::stringstream sSrvNameCnfContent;
        sSrvNameCnfContent << srvFin.rdbuf();
        std::string sContent = sSrvNameCnfContent.str();
        sSrvNameCnfContent.str("");
        //std::cout << "\033[34mdata: " << sSrvNameCnfContent.str() << std::endl;
        if (sContent.empty())
        {
            srvFin.close();
            m_oCurrentSrvNameCnf = m_oLastSrvNameCnf;
            return false;
        }

        if (!m_oCurrentSrvNameCnf.Parse(sContent)) 
        {
            std::cerr << "\033[31m load srv name cnf failed,file: "<< SRV_NAME_CONF_FILE 
                << ",err msg: " << m_oCurrentSrvNameCnf.GetErrMsg() << ", orig data: " 
                << sContent << std::flush <<  std::endl;
            m_oCurrentSrvNameCnf = m_oLastSrvNameCnf;

            srvFin.close();
            return false;
        }

        //std::cout << "cur conf: " << m_oCurrentSrvNameCnf.ToString() << std::endl;
        srvFin.close();
    } 
    else 
    {
        m_oCurrentSrvNameCnf = m_oLastSrvNameCnf;
        std::cerr << "load srv name cnf failed,file: "<< SRV_NAME_CONF_FILE  << std::endl;
        return false;
    }
    return true;
}

bool OssManager::GetNodeTypeCmdConf() 
{
    m_oLastNodeTypeCmdCnf = m_oCurrentNodeTypeCmdCnf;
    std::ifstream srvFin(NODETYPE_CMD_CONF_FILE);
    if (srvFin.good()) {
        std::stringstream sNodeTypeCmd;
        sNodeTypeCmd << srvFin.rdbuf();

        if (!m_oCurrentNodeTypeCmdCnf.Parse(sNodeTypeCmd.str())) 
        {
            sNodeTypeCmd.str("");
            srvFin.close();
            m_oCurrentNodeTypeCmdCnf = m_oLastNodeTypeCmdCnf;
            std::cerr << "load node type cmd conf failed, file: " << NODETYPE_CMD_CONF_FILE << std::endl;
            return false;
        }

        sNodeTypeCmd.str("");
        srvFin.close();
    } else {
        std::cerr << "load node type cmd conf failed, file: " << NODETYPE_CMD_CONF_FILE << std::endl;
        return false;
    }
    return true;
}

bool OssManager::Init()
{
    char szLogName[256] = {0};
    snprintf(szLogName, sizeof(szLogName), "%s/log/%s.log", m_strWorkPath.c_str(), getproctitle());
    std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
    log4cplus::initialize();
    log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                    szLogName, atol(m_oCurrentConf("max_log_file_size").c_str()),
                    atoi(m_oCurrentConf("max_log_file_num").c_str())));
    append->setName(szLogName);
    std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
    append->setLayout(layout);
    //log4cplus::Logger::getRoot().addAppender(append);
    m_oLogger = log4cplus::Logger::getInstance(szLogName);
    m_oLogger.addAppender(append);
    m_oLogger.setLogLevel(m_iLogLevel);
    LOG4_INFO("%s begin, and work path %s", m_oCurrentConf("server_name").c_str(), m_strWorkPath.c_str());

    socklen_t addressLen = 0;
    int queueLen = 100;
    int reuse = 1;

#ifdef NODE_TYPE_ACCESS
    // 接入节点才需要监听客户端连接
    struct sockaddr_in stAddrOuter;
    struct sockaddr *pAddrOuter;
    stAddrOuter.sin_family = AF_INET;
    stAddrOuter.sin_port = htons(m_iPortForClient);
    stAddrOuter.sin_addr.s_addr = inet_addr(m_strHostForClient.c_str());
    pAddrOuter = (struct sockaddr*)&stAddrOuter;
    addressLen = sizeof(struct sockaddr);
    m_iC2SListenFd = socket(pAddrOuter->sa_family, SOCK_STREAM, 0);
    if (m_iC2SListenFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        int iErrno = errno;
        exit(iErrno);
    }
    reuse = 1;
    ::setsockopt(m_iC2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (bind(m_iC2SListenFd, pAddrOuter, addressLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        close(m_iC2SListenFd);
        m_iC2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    if (listen(m_iC2SListenFd, queueLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        close(m_iC2SListenFd);
        m_iC2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    
    LOG4_TRACE("init access node, c2s fd: %d",m_iC2SListenFd);
#endif

    struct sockaddr_in stAddrInner;
    struct sockaddr *pAddrInner;
    stAddrInner.sin_family = AF_INET;
    stAddrInner.sin_port = htons(m_iPortForServer);
    stAddrInner.sin_addr.s_addr = inet_addr(m_strHostForServer.c_str());
    pAddrInner = (struct sockaddr*)&stAddrInner;
    addressLen = sizeof(struct sockaddr);
    m_iS2SListenFd = socket(pAddrInner->sa_family, SOCK_STREAM, 0);
    if (m_iS2SListenFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        int iErrno = errno;
        exit(iErrno);
    }
    reuse = 1;
    ::setsockopt(m_iS2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (bind(m_iS2SListenFd, pAddrInner, addressLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        close(m_iS2SListenFd);
        m_iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    if (listen(m_iS2SListenFd, queueLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        close(m_iS2SListenFd);
        m_iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }

//    m_pCmdConnect = new CmdConnectWorker();
//    if (m_pCmdConnect == NULL)
//    {
//        return(false);
//    }
//    m_pCmdConnect->SetLogger(m_oLogger);
//    m_pCmdConnect->SetLabor(this);

 #ifdef USE_CENTER_NODE //this will use conf 
    // 创建到Center的连接信息
    for (int i = 0; i < m_oCurrentConf["center"].GetArraySize(); ++i)
    {
        std::string strIdentify = m_oCurrentConf["center"][i]("host") + std::string(":")
            + m_oCurrentConf["center"][i]("port") + std::string(".0");     // CenterServer只有一个Worker
        tagMsgShell stMsgShell;
        LOG4_TRACE("m_mapCenterMsgShell.insert(%s, fd %d, seq %llu) = %u",
                        strIdentify.c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
        m_mapCenterMsgShell.insert(std::pair<std::string, tagMsgShell>(strIdentify, stMsgShell));
    }
 #endif
    if (false == m_srvNameShmFd.Init(SRV_NAME_VER_SHM_KEY, SRV_NAME_VER_SHM_SZ)) {
      LOG4_ERROR("init srv name shm failed, shm_key: 0x%x, size: %d",SRV_NAME_VER_SHM_KEY,SRV_NAME_VER_SHM_SZ);
    }

    return(true);
}

void OssManager::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    for (std::map<int32, Cmd*>::iterator cmd_iter = m_mapCmd.begin();
                    cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
    }
    m_mapCmd.clear();
    
    m_mapWorkerIdPid.clear();
    m_mapWorker.clear();
    m_mapWorkerFdPid.clear();
    m_mapWorkerRestartNum.clear();
    for (std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.begin();
                    iter != m_mapFdAttr.end(); ++iter)
    {
        DestroyConnect(iter);
    }
    m_mapFdAttr.clear();
    m_mapClientConnFrequency.clear();
    m_vecFreeWorkerIdx.clear();
    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }
    if (m_pErrBuff != NULL)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = NULL;
    }
}

void OssManager::CreateWorker()
{
    LOG4_TRACE("%s", __FUNCTION__);
    int iPid = 0;

    for (unsigned int i = 0; i < m_uiWorkerNum; ++i)
    {
        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        }

        iPid = fork();
        if (iPid == 0)   // 子进程
        {
            ev_loop_destroy(m_loop);
            close(m_iS2SListenFd);
#ifdef NODE_TYPE_ACCESS
            close(m_iC2SListenFd);
#endif

            close(iControlFds[0]);
            close(iDataFds[0]);
            
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            
            OssWorker worker(m_strWorkPath, iControlFds[1], iDataFds[1], i, m_oCurrentConf, m_oCurrentSrvNameCnf);
            worker.SetNodeTypeCmdCnf(m_oCurrentNodeTypeCmdCnf);
            worker.SetWhiteListCnf(m_oCurrentWhiteListCnf.ToString());
            worker.Run();
            exit(-2);
        }
        else if (iPid > 0)   // 父进程
        {
            close(iControlFds[1]);
            close(iDataFds[1]);
            
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            
            tagWorkerAttr stWorkerAttr;
            stWorkerAttr.iWorkerIndex = i;
            stWorkerAttr.iControlFd = iControlFds[0];
            stWorkerAttr.iDataFd = iDataFds[0];
            
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iPid, stWorkerAttr));
            
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iPid));
            
            m_mapWorkerIdPid.insert(std::pair<int,int>(i, iPid));
            
            CreateFdAttr(iControlFds[0], GetSequence());
            CreateFdAttr(iDataFds[0], GetSequence());

            AddIoReadEvent(iControlFds[0]);
            AddIoReadEvent(iDataFds[0]);
            LOG4_INFO("worker id: %u, manager data fd: %u, control fd: %u", i, iDataFds[0], iControlFds[0]);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        }
    }
}

void OssManager::CreateMonitorWorker() {
  LOG4_TRACE("%s()", __FUNCTION__);
  int iPid = 0;
  
  int iMonitorId = m_uiWorkerNum;

  int iControlFds[2];
  int iDataFds[2];
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
  {
    LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
  }
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
  {
    LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
  }

  iPid = fork();
  if (iPid ==0) {
    ev_loop_destroy(m_loop);
    close(m_iS2SListenFd);

#ifdef NODE_TYPE_ACCESS
    close(m_iC2SListenFd);
#endif
    close(iControlFds[0]);
    close(iDataFds[0]);
    x_sock_set_block(iControlFds[1], 0);
    x_sock_set_block(iDataFds[1], 0);
    OssWorker worker(m_strWorkPath, iControlFds[1], iDataFds[1], 
                     iMonitorId, m_oCurrentConf, m_oCurrentSrvNameCnf,true);
    worker.SetNodeTypeCmdCnf(m_oCurrentNodeTypeCmdCnf);
    worker.SetWhiteListCnf(m_oCurrentWhiteListCnf.ToString());
    worker.Run();
    exit(-2);
  } else if (iPid >0) {
    close(iControlFds[1]);
    close(iDataFds[1]);
    x_sock_set_block(iControlFds[0], 0);
    x_sock_set_block(iDataFds[0], 0);

    m_WorkMonitor.monWorkerAttr.iWorkerIndex = iMonitorId;
    m_WorkMonitor.monWorkerAttr.iControlFd = iControlFds[0];
    m_WorkMonitor.monWorkerAttr.iDataFd = iDataFds[0];

    //used to recevie msg process.
    m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iPid));
    m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iPid));

    CreateFdAttr(iControlFds[0], GetSequence());
    CreateFdAttr(iDataFds[0], GetSequence());
    AddIoReadEvent(iControlFds[0]);
    AddIoReadEvent(iDataFds[0]);
  } else {
    LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
  }
}

bool OssManager::CreateEvents()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
    if (m_loop == NULL)
    {
        return(false);
    }
    CreateFdAttr(m_iS2SListenFd, GetSequence());
    AddIoReadEvent(m_iS2SListenFd);
#ifdef NODE_TYPE_ACCESS
    LOG4_TRACE("create conn obj for c2s fd: %d", m_iC2SListenFd);
    CreateFdAttr(m_iC2SListenFd, GetSequence());
    AddIoReadEvent(m_iC2SListenFd);
#endif
    CreateSignalProcHandle(); 
    AddPeriodicTaskEvent();
    return(true);
}

void OssManager::CreateSignalProcHandle() {
  ev_signal* signal_watcher = new ev_signal();
  ev_signal_init (signal_watcher, ChildTerminatedCallback, SIGCHLD);
  signal_watcher->data = (void*)this;
  ev_signal_start (m_loop, signal_watcher);

  signal_watcher = new ev_signal();
  ev_signal_init (signal_watcher, SelfDefSignalCallBack, SIGUSR1);
  signal_watcher->data = (void*)this;
  ev_signal_start (m_loop, signal_watcher);


  signal_watcher = new ev_signal();
  ev_signal_init (signal_watcher, SelfDefSignalCallBack, SIGUSR2);
  signal_watcher->data = (void*)this;
  ev_signal_start (m_loop, signal_watcher);
}

bool OssManager::RegisterToCenter()
{
    if (m_mapCenterMsgShell.size() == 0)
    {
        return(true);
    }
    LOG4_DEBUG("%s()", __FUNCTION__);
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    loss::CJsonObject oReportData;
    loss::CJsonObject oMember;
    oReportData.Add("node_type", m_strNodeType);
    oReportData.Add("node_id", m_uiNodeId);
    oReportData.Add("node_ip", m_strHostForServer);
    oReportData.Add("node_port", m_iPortForServer);
    oReportData.Add("access_ip", m_strHostForClient);
    oReportData.Add("access_port", m_iPortForClient);
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", loss::CJsonObject("{}"));
    oReportData.Add("worker", loss::CJsonObject("[]"));
    std::unordered_map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        iLoad += worker_iter->second.iLoad;
        iConnect += worker_iter->second.iConnect;
        iRecvNum += worker_iter->second.iRecvNum;
        iRecvByte += worker_iter->second.iRecvByte;
        iSendNum += worker_iter->second.iSendNum;
        iSendByte += worker_iter->second.iSendByte;
        iClientNum += worker_iter->second.iClientNum;
        oMember.Clear();
        oMember.Add("load", worker_iter->second.iLoad);
        oMember.Add("connect", worker_iter->second.iConnect);
        oMember.Add("recv_num", worker_iter->second.iRecvNum);
        oMember.Add("recv_byte", worker_iter->second.iRecvByte);
        oMember.Add("send_num", worker_iter->second.iSendNum);
        oMember.Add("send_byte", worker_iter->second.iSendByte);
        oMember.Add("client", worker_iter->second.iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
    oMsgBody.set_body(oReportData.ToString());
    oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    std::map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.begin();
    for (; center_iter != m_mapCenterMsgShell.end(); ++center_iter)
    {
        if (center_iter->second.iFd == 0)
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            AutoSend(center_iter->first, oMsgHead, oMsgBody);
        }
        else
        {
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            SendTo(center_iter->second, oMsgHead, oMsgBody);
        }
    }
    return(true);
}

bool OssManager::RestartWorker(int iPid)
{
    LOG4_DEBUG("%s(pid %u)", __FUNCTION__, iPid);
    int iNewPid = 0;
    char errMsg[1024] = {0};
    std::unordered_map<int, tagWorkerAttr>::iterator worker_iter;
    std::map<int, int>::iterator fd_iter;
    std::map<int, tagConnectionAttr*>::iterator conn_iter;
    std::map<int, CWorkerRestartStat*>::iterator restart_num_iter;

    worker_iter = m_mapWorker.find(iPid);

    if (worker_iter != m_mapWorker.end())
    {
        int iWorkerIndex = worker_iter->second.iWorkerIndex;
        
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iControlFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }

        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iDataFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }

        DestroyConnect(m_mapFdAttr.find(worker_iter->second.iControlFd));
        DestroyConnect(m_mapFdAttr.find(worker_iter->second.iDataFd));
        
        close(worker_iter->second.iControlFd);
        close(worker_iter->second.iDataFd);
        m_mapWorker.erase(iPid);

        if (m_mapWorkerIdPid.find(iWorkerIndex) != m_mapWorkerIdPid.end()) {
          m_mapWorkerIdPid.erase(iWorkerIndex);
        }

        restart_num_iter = m_mapWorkerRestartNum.find(iWorkerIndex);
        if (restart_num_iter != m_mapWorkerRestartNum.end())
        {
            if (restart_num_iter->second->CheckRestartTimesStatus() == RESTART_TIMES_USEUP) { 
                LOG4_FATAL("worker %d had been restarted %d times, it will not be restart again!",
                                iWorkerIndex, restart_num_iter->second->GetRestartTimes());
                m_vecFreeWorkerIdx.push_back(iWorkerIndex);
                --m_uiWorkerNum;
                delete restart_num_iter->second;
                m_mapWorkerRestartNum.erase(restart_num_iter);
                return(false);
            }
        }

        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }

        iNewPid = fork();
        if (iNewPid == 0)   // 子进程
        {
            ev_loop_destroy(m_loop);
            close(m_iS2SListenFd);
#ifdef NODE_TYPE_ACCESS
            close(m_iC2SListenFd);
#endif
            close(iControlFds[0]);
            close(iDataFds[0]);
            
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
           
            OssWorker worker(m_strWorkPath, iControlFds[1], iDataFds[1], iWorkerIndex, m_oCurrentConf, m_oCurrentSrvNameCnf);
            worker.SetNodeTypeCmdCnf(m_oCurrentNodeTypeCmdCnf);
            worker.SetWhiteListCnf(m_oCurrentWhiteListCnf.ToString());
            worker.Run();
            
            exit(-2);   // 子进程worker没有正常运行
        }
        else if (iNewPid > 0)   // 父进程
        {
            LOG4_INFO("restart worker id: %d, old pid: %u, new pid: %u successfully", 
                      iWorkerIndex, iPid, iNewPid);
            ev_loop_fork(m_loop);

            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            
            tagWorkerAttr stWorkerAttr;
            
            stWorkerAttr.iWorkerIndex = iWorkerIndex;
            stWorkerAttr.iControlFd = iControlFds[0];
            stWorkerAttr.iDataFd = iDataFds[0];
            stWorkerAttr.tBeatTime = ev_now(m_loop);

            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iNewPid, stWorkerAttr));
            
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iNewPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iNewPid));
            m_mapWorkerIdPid[iWorkerIndex] = iNewPid;

            tagConnectionAttr* pConnAttr = NULL;  
            pConnAttr = CreateFdAttr(iControlFds[0], GetSequence());
            if (pConnAttr != NULL) 
            {
                AddIoReadEvent(iControlFds[0]);
            }

            pConnAttr = CreateFdAttr(iDataFds[0], GetSequence());
            if (pConnAttr != NULL)
            {
                AddIoReadEvent(iDataFds[0]);
            }
            
            if (restart_num_iter == m_mapWorkerRestartNum.end())
            {
                CWorkerRestartStat* StatRestart = new CWorkerRestartStat(60);
                StatRestart->UpdateRestartStatus();
                m_mapWorkerRestartNum.insert(std::pair<int, CWorkerRestartStat*>(iWorkerIndex, StatRestart));
            }
            else
            {
                restart_num_iter->second->UpdateRestartStatus();
            }
            return(true);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
    }
    return(false);
}

bool OssManager::RestartMonitWork(int iPid) {
    LOG4_DEBUG("%s()", __FUNCTION__);
    tagWorkerAttr& attr = m_WorkMonitor.monWorkerAttr;
    close(attr.iControlFd);
    close(attr.iDataFd);

    std::map<int, int>::iterator fd_iter;
    fd_iter = m_mapWorkerFdPid.find(attr.iControlFd);
    if (fd_iter != m_mapWorkerFdPid.end())
    {
        m_mapWorkerFdPid.erase(fd_iter);
    }
    fd_iter = m_mapWorkerFdPid.find(attr.iDataFd);
    if (fd_iter != m_mapWorkerFdPid.end())
    {
        m_mapWorkerFdPid.erase(fd_iter);
    }

    DestroyConnect(m_mapFdAttr.find(attr.iControlFd));
    DestroyConnect(m_mapFdAttr.find(attr.iDataFd));

    CreateMonitorWorker();
}

bool OssManager::AddPeriodicTaskEvent()
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    //
    ev_timer_init (timeout_watcher, PeriodicTaskCallback, 
                   NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    timeout_watcher->data = (void*)this;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool OssManager::AddIoReadEvent(int iFd)
{
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, iFd);
    ev_io* io_watcher = NULL;
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
    if (iter != m_mapFdAttr.end())
    {
        if (NULL == iter->second->pIoWatcher)
        {
            io_watcher = new ev_io();
            if (io_watcher == NULL)
            {
                LOG4_ERROR("new io_watcher error!");
                return(false);
            }
            tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
            if (pData == NULL)
            {
                LOG4_ERROR("new tagIoWatcherData error!");
                delete io_watcher;
                return(false);
            }
            pData->iFd = iFd;
            pData->ulSeq = iter->second->ulSeq;
            pData->pManager = this;
            ev_io_init (io_watcher, IoCallback, iFd, EV_READ);
            iter->second->pIoWatcher = io_watcher;
            io_watcher->data = (void*)pData;
            ev_io_start (m_loop, io_watcher);
        }
        else
        {
            io_watcher = iter->second->pIoWatcher;
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
            ev_io_start (m_loop, io_watcher);
        }
    }
    return(true);
}

bool OssManager::AddIoWriteEvent(int iFd)
{
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, iFd);
    ev_io* io_watcher = NULL;
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
    if (iter != m_mapFdAttr.end())
    {
        if (NULL == iter->second->pIoWatcher)
        {
            io_watcher = new ev_io();
            if (io_watcher == NULL)
            {
                LOG4_ERROR("new io_watcher error!");
                return(false);
            }
            tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
            if (pData == NULL)
            {
                LOG4_ERROR("new tagIoWatcherData error!");
                delete io_watcher;
                return(false);
            }
            pData->iFd = iFd;
            pData->ulSeq = iter->second->ulSeq;
            pData->pManager = this;

            ev_io_init (io_watcher, IoCallback, iFd, EV_WRITE);
            iter->second->pIoWatcher = io_watcher;
            io_watcher->data = (void*)pData;
            ev_io_start (m_loop, io_watcher);
        }
        else
        {
            io_watcher = iter->second->pIoWatcher;
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
            ev_io_start (m_loop, io_watcher);
        }
    }
    return(true);
}
//
//bool OssManager::AddIoErrorEvent(int iFd)
//{
//    LOG4_TRACE("%s(fd %d)", __FUNCTION__, iFd);
//    ev_io* io_watcher = NULL;
//    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
//    if (iter != m_mapFdAttr.end())
//    {
//        if (NULL == iter->second->pIoWatcher)
//        {
//            io_watcher = new ev_io();
//            if (io_watcher == NULL)
//            {
//                LOG4_ERROR("new io_watcher error!");
//                return(false);
//            }
//            tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
//            if (pData == NULL)
//            {
//                LOG4_ERROR("new tagIoWatcherData error!");
//                delete io_watcher;
//                return(false);
//            }
//            pData->iFd = iFd;
//            pData->ullSeq = iter->second->ullSeq;
//            pData->pManager = this;
//            ev_io_init (io_watcher, IoCallback, iFd, EV_ERROR);
//            iter->second->pIoWatcher = io_watcher;
//            io_watcher->data = (void*)pData;
//            ev_io_start (m_loop, io_watcher);
//        }
//        else
//        {
//            io_watcher = iter->second->pIoWatcher;
//            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_ERROR);
//        }
//    }
//    return(true);
//}

bool OssManager::RemoveIoWriteEvent(int iFd)
{
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, iFd);
    ev_io* io_watcher = NULL;
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
    if (iter != m_mapFdAttr.end())
    {
        if (NULL != iter->second->pIoWatcher)
        {
            if (iter->second->pIoWatcher->events & EV_WRITE)
            {
                io_watcher = iter->second->pIoWatcher;
                ev_io_stop(m_loop, io_watcher);
                ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & (~EV_WRITE));
                ev_io_start (m_loop, iter->second->pIoWatcher);
            }
        }
    }
    return(true);
}

bool OssManager::DelEvents(ev_io** io_watcher_addr)
{
    if (io_watcher_addr == NULL)
    {
        return(false);
    }
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, (*io_watcher_addr)->fd);
    ev_io_stop (m_loop, *io_watcher_addr);
    tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)((*io_watcher_addr)->data);
    if (pData != NULL)
    {
        delete pData;
    }
    (*io_watcher_addr)->data = NULL;
    delete (*io_watcher_addr);
    (*io_watcher_addr) = NULL;
    io_watcher_addr = NULL;
    return(true);
}

bool OssManager::DelEvents(ev_io* &io_watcher)
{
    if (io_watcher == NULL) 
    {
        return false;
    }
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, io_watcher->fd);
    ev_io_stop (m_loop, io_watcher);

    tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)(io_watcher->data);
    if (pData != NULL)
    {
        delete pData;
    }
    io_watcher->data = NULL;

    delete io_watcher;
    io_watcher = NULL;
    return(true);
}

bool OssManager::AddIoTimeout(int iFd, uint32 ulSeq, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagIoWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, IoTimeoutCallback,
                   dTimeout + ev_time() - ev_now(m_loop), 0.);
    pData->iFd = iFd;
    pData->ulSeq = ulSeq;
    pData->pManager = this;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool OssManager::AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagClientConnWatcherData* pData = new tagClientConnWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagClientConnWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, ClientConnFrequencyTimeoutCallback, 
                   dTimeout + ev_time() - ev_now(m_loop), 0.);
    pData->pManager = this;
    pData->iAddr = iAddr;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

tagConnectionAttr* OssManager::CreateFdAttr(int iFd, uint32 ulSeq)
{
    LOG4_DEBUG("%s(iFd %d, seq %llu)", __FUNCTION__, iFd, ulSeq);
    std::map<int, tagConnectionAttr*>::iterator fd_attr_iter;
    fd_attr_iter = m_mapFdAttr.find(iFd);
    if (fd_attr_iter == m_mapFdAttr.end())
    {
        tagConnectionAttr* pConnAttr = new tagConnectionAttr();
        if (pConnAttr == NULL)
        {
            LOG4_ERROR("new pConnAttr for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->iFd = iFd;

        pConnAttr->pRecvBuff = new loss::CBuffer();
        if (pConnAttr->pRecvBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pRecvBuff for fd%d error!", iFd);
            return(NULL);
        }
        pConnAttr->pSendBuff = new loss::CBuffer();
        if (pConnAttr->pSendBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pSendBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pWaitForSendBuff = new loss::CBuffer();
        if (pConnAttr->pWaitForSendBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pWaitForSendBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->dActiveTime = ev_now(m_loop);
        pConnAttr->ulSeq = ulSeq;
        m_mapFdAttr.insert(std::pair<int, tagConnectionAttr*>(iFd, pConnAttr));
        return(pConnAttr);
    }
    else
    {
        LOG4_ERROR("fd %d is exist!", iFd);
        return(NULL);
    }
}

bool OssManager::DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter)
{
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    LOG4_DEBUG("%s() iter->second->pIoWatcher = %p, fd %d, data %ld", __FUNCTION__,
                    iter->second->pIoWatcher, iter->second->pIoWatcher->fd, iter->second->pIoWatcher->data);
    std::map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(iter->second->strIdentify);
    if (center_iter != m_mapCenterMsgShell.end())
    {
        center_iter->second.iFd = 0;
        center_iter->second.ulSeq = 0;
    }

    DelEvents(iter->second->pIoWatcher);
    close(iter->first);
    delete iter->second;
    iter->second = NULL;
    m_mapFdAttr.erase(iter);
    return(true);
}

std::pair<int, int> OssManager::GetMinLoadWorkerDataFd()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iMinLoadWorkerFd = 0;
    int iMinLoad = -1;
    std::pair<int, int> worker_pid_Datafd;
    std::unordered_map<int, tagWorkerAttr>::iterator iter;
    for (iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
    {
       if (iter == m_mapWorker.begin())
       {
           iMinLoadWorkerFd = iter->second.iDataFd;
           iMinLoad = iter->second.iLoad;
           worker_pid_Datafd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
       }
       else if (iter->second.iLoad < iMinLoad)
       {
           iMinLoadWorkerFd = iter->second.iDataFd;
           iMinLoad = iter->second.iLoad;
           worker_pid_Datafd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
       }
    }
    LOG4_TRACE("get min load pid: %u", worker_pid_Datafd.first);
    return(worker_pid_Datafd);
}

void OssManager::SetWorkerLoad(int iPid, loss::CJsonObject& oJsonLoad)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagWorkerAttr>::iterator iter;
    iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        oJsonLoad.Get("load", iter->second.iLoad);
        oJsonLoad.Get("connect", iter->second.iConnect);
        oJsonLoad.Get("recv_num", iter->second.iRecvNum);
        oJsonLoad.Get("recv_byte", iter->second.iRecvByte);
        oJsonLoad.Get("send_num", iter->second.iSendNum);
        oJsonLoad.Get("send_byte", iter->second.iSendByte);
        oJsonLoad.Get("client", iter->second.iClientNum);
        iter->second.tBeatTime = ev_now(m_loop);
        LOG4_TRACE("recv worker pid: [%u] update load", iPid);
    }
}

void OssManager::AddWorkerLoad(int iPid, int iLoad)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagWorkerAttr>::iterator iter;
    iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        iter->second.iLoad += iLoad;
    }
}

const std::unordered_map<int, tagWorkerAttr>& OssManager::GetWorkerAttr() const
{
	return(m_mapWorker);
}

bool OssManager::CheckWorker()
{
    static int tmBeat = 5*(NODE_BEAT);
    tmBeat = 5*(NODE_BEAT);
    
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagWorkerAttr>::iterator itWorker;
    
    for (itWorker = m_mapWorker.begin(); itWorker != m_mapWorker.end(); ++itWorker) {
        if (m_workersNormalRestart.IsWorkerRestarting(itWorker->first))
        {
            LOG4_TRACE("worker pid: %u is restarting, need not check dead", 
                       itWorker->first);
            itWorker->second.tBeatTime  = ev_now(m_loop);
            continue;
        }

        if (ev_now(m_loop) - itWorker->second.tBeatTime > tmBeat) {
            LOG4_ERROR("worker pid: [%u], worker id: [%u] not active during tm: [%u]s", 
                       itWorker->first, itWorker->second.iWorkerIndex, tmBeat);
            {
                loss::CJsonObject jsAlarm;
                jsAlarm.Add("node_type", m_strNodeType);
                jsAlarm.Add("ip", m_strHostForServer);
                jsAlarm.Add("worker_id",itWorker->second.iWorkerIndex);
                jsAlarm.Add("worker_pid", itWorker->first);
                jsAlarm.Add("detail","worker not active, to check it");
                 
                SendAlarmToMonitWorker(jsAlarm);
            }
        }
    }
    return(true);
}

void OssManager::RefreshServer()
{
    LOG4_TRACE("%s(m_iRefreshInterval %d, m_iLastRefreshCalc %d)", __FUNCTION__, m_iRefreshInterval, m_iLastRefreshCalc);
    int iErrno = 0;
    ++m_iLastRefreshCalc;
    if (m_iLastRefreshCalc < m_iRefreshInterval)
    {
      return;
    }

    m_iLastRefreshCalc = 0;
    ReflushSrvCnf();
}

// > 0: incr, =0: not change, -1: err
int32_t OssManager::CheckSrvNameVerIncr() {
  static int64_t iLLocalVerNo = 0;
  int64_t iLVerNo = 0;
  if (false == m_srvNameShmFd.GetValue(SRV_NAME_VER_KEY, iLVerNo)) {
    LOG4_ERROR("get ver no failed,ver key: %s, err msg: %s", 
               SRV_NAME_VER_KEY, m_srvNameShmFd.GetErrMsg().c_str());
    return  -1;
  }

  if (iLLocalVerNo != iLVerNo) {
    LOG4_TRACE("srv name ver No. change, orig ver: %lu,new ver: %lu, shmkey: %s",
               iLLocalVerNo, iLVerNo, SRV_NAME_VER_KEY);
    iLLocalVerNo = iLVerNo; 
    return  1;
  }

  LOG4_TRACE("local svr name ver same to shm ver, no: %lu", iLVerNo);
  return 0;
}

int32_t OssManager::CheckNodeTypeCmdVerIncr() {
  static int64_t iLLocalVNo = 0;
  int64_t iLVerNo = 0;
  if (false == m_srvNameShmFd.GetValue(NODETYPE_CMD_VER_KEY, iLVerNo)) {
    LOG4_ERROR("get ver no failed,ver key: %s, err msg: %s", 
               SRV_NAME_VER_KEY, m_srvNameShmFd.GetErrMsg().c_str());
    return  -1;
  }

  if (iLLocalVNo != iLVerNo) {
    LOG4_TRACE("node type cmd ver No. change, orig ver: %lu,new ver: %lu, shmkey: %s",
               iLLocalVNo, iLVerNo, NODETYPE_CMD_VER_KEY);
    iLLocalVNo = iLVerNo; 
    return  1;
  }

  LOG4_TRACE("local node type cmd ver same to shm ver, No: %lu", iLVerNo);
  return 0;
}

int32_t OssManager::ShmCheckVer(const std::string& sKey) {
    int32_t  iRet = m_shmCheck.CheckShmVer(sKey);
    if ( iRet < 0) {
        LOG4_ERROR("get shm ver no failed,shm key: %s, err msg: %s", 
                   sKey.c_str(),m_shmCheck.GetErrMsg().c_str());
        return  iRet;
    } else if (iRet == 0) {
        LOG4_TRACE("shm ver not change, shm No: %lu, shm key: %s", 
                  m_shmCheck.GetVerVal(sKey), sKey.c_str());
        return iRet;
    } else {
        LOG4_TRACE("shm ver has change, new ver: %llu, shm key: %s",
                   m_shmCheck.GetVerVal(sKey), sKey.c_str());
    }
    return iRet;
}

void OssManager::ReflushSrvCnf() {
    //first check shm version, if modify, then load srv name conf.
    int iRet = 0;
    do {
        iRet = ShmCheckVer(SRV_NAME_VER_KEY);
        if (iRet < 0) {
            LOG4_ERROR("get srv name ver in shm failed");
            break;
        } else if (iRet == 0) {
            LOG4_TRACE("srv name ver no in shm not incr");
            break;
        }

        if (!GetSrvNameConf()) {
            LOG4_ERROR("GetSrvNameConf() failed, check srv name conf");
            break;
        }
        if (m_oLastSrvNameCnf["down_stream"].ToString() != m_oCurrentSrvNameCnf["down_stream"].ToString()) {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_body(m_oCurrentSrvNameCnf["down_stream"].ToString());
            oMsgHead.set_cmd(CMD_REQ_RELOAD_DOWN_STREAM);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
            SendToWorker(oMsgHead, oMsgBody);
            LOG4_INFO("send new downstream cnf to worker");
        }
    } while (0);

    do {
        iRet = ShmCheckVer(NODETYPE_CMD_VER_KEY);
        if (iRet == -1) {
            LOG4_ERROR("node type cmd ver in shm failed");
            break;
        } else if (iRet == 0) {
            LOG4_TRACE("node type cmd ver not change");
            break;
        }

        if (!GetNodeTypeCmdConf()) {
            LOG4_ERROR("GetNodeTypeCmdConf() fail, check node type cmd cnf");
            break;
        }

        if (m_oCurrentNodeTypeCmdCnf.ToString() != m_oLastNodeTypeCmdCnf.ToString()) {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_body(m_oCurrentNodeTypeCmdCnf.ToString());
            oMsgHead.set_cmd(CMD_REQ_RELOAD_NODETYP_CMD);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
            SendToWorker(oMsgHead, oMsgBody);
            LOG4_INFO("send new node type cmd cnf to worker");
        }
    } while(0);

    do {
        int iRet = ShmCheckVer(WHITE_LIST_VER_KEY);
        if (iRet < 0) {
            LOG4_ERROR("get white list ver in shm fail");
            break;
        } else if (iRet == 0) {
            LOG4_TRACE("white list cmd ver not change");
            break;
        }
        if (!GetWhiteListConf()) {
            LOG4_ERROR("GetWhiteListConf() fail, check white list cnf");
            break;
        }
        if (m_oLastWhiteListCnf.ToString() != m_oCurrentWhiteListCnf.ToString()) {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_body(m_oCurrentWhiteListCnf.ToString());
            oMsgHead.set_cmd(CMD_REQ_RELOAD_WHITELIST);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
            SendToWorker(oMsgHead, oMsgBody);
            LOG4_INFO("send new white list cnf to worker");
        }
    } while(0);
}

void OssManager::ReflushHostCnf() {
    if (GetConf())
    {
        do {
            if (m_oLastConf("log_level") != m_oCurrentConf("log_level"))
            {
                m_oLogger.setLogLevel(m_iLogLevel);
                MsgHead oMsgHead;
                MsgBody oMsgBody;
                LogLevel oLogLevel;
                oLogLevel.set_log_level(m_iLogLevel);
                oMsgBody.set_body(oLogLevel.SerializeAsString());
                oMsgHead.set_cmd(CMD_REQ_SET_LOG_LEVEL);
                oMsgHead.set_seq(GetSequence());
                oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
                SendToWorker(oMsgHead, oMsgBody);
            }

            // 更新动态库配置或重新加载动态库
            if (m_oLastConf["so"].ToString() != m_oCurrentConf["so"].ToString())
            {
                MsgHead oMsgHead;
                MsgBody oMsgBody;
                oMsgBody.set_body(m_oCurrentConf["so"].ToString());
                oMsgHead.set_cmd(CMD_REQ_RELOAD_SO);
                oMsgHead.set_seq(GetSequence());
                oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
                SendToWorker(oMsgHead, oMsgBody);
            }
            if (m_oLastConf["module"].ToString() != m_oCurrentConf["module"].ToString())
            {
                MsgHead oMsgHead;
                MsgBody oMsgBody;
                oMsgBody.set_body(m_oCurrentConf["module"].ToString());
                oMsgHead.set_cmd(CMD_REQ_RELOAD_MODULE);
                oMsgHead.set_seq(GetSequence());
                oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
                SendToWorker(oMsgHead, oMsgBody);
            }

            //
        } while(0);

DEFAULT_LOAD:
        if (m_iRestartConf != 0 &&
            m_oLastConf("restart_worker") != m_oCurrentConf("restart_worker") )  
        {
            if (m_workersNormalRestart.AllWorkersRestart() == true)
            {
                LOG4_TRACE("all workers is restarting .....");
                return ;
            }

            RestartAllWorkers();
            LOG4_INFO("now restart all workers >>>>> ");
            m_iRestartConf = 0;
        }
    }
    else
    {
        LOG4_INFO("GetConf() error, please check the config file.", "");
    }
}

void OssManager::RestartAllWorkers() 
{
    LOG4_TRACE("%s()", __FUNCTION__);
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    oMsgHead.set_cmd(CMD_REQ_RESTART_CHILDPROC);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(0);
    SendToWorker(oMsgHead, oMsgBody);
    
    std::unordered_map<int, tagWorkerAttr>::iterator itWorker;
    for (itWorker = m_mapWorker.begin(); itWorker != m_mapWorker.end(); ++itWorker)
    {
        m_workersNormalRestart.AddRestartWorker(itWorker->first);
        LOG4_TRACE("worker pid: %u to set restart flag", itWorker->first);
    }
}

/**
 * @brief 上报节点状态信息
 * @return 上报是否成功
 * @note 节点状态信息结构如：
 * {
 *     "node_type":"ACCESS",
 *     "node_ip":"192.168.11.12",
 *     "node_port":9988,
 *     "access_ip":"120.234.2.106",
 *     "access_port":10001,
 *     "worker_num":10,
 *     "active_time":16879561651.06,
 *     "node":{
 *         "load":1885792, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":495870
 *     },
 *     "worker":
 *     [
 *          {"load":655666, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":655235, "connect":485872, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":585696, "connect":415379, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}}
 *     ]
 * }
 */
bool OssManager::ReportToCenter()
{
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    loss::CJsonObject oReportData;
    loss::CJsonObject oMember;
    oReportData.Add("node_type", m_strNodeType);
    oReportData.Add("node_id", m_uiNodeId);
    oReportData.Add("node_ip", m_strHostForServer);
    oReportData.Add("node_port", m_iPortForServer);
    oReportData.Add("access_ip", m_strHostForClient);
    oReportData.Add("access_port", m_iPortForClient);
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", loss::CJsonObject("{}"));
    oReportData.Add("worker", loss::CJsonObject("[]"));
    std::unordered_map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        iLoad += worker_iter->second.iLoad;
        iConnect += worker_iter->second.iConnect;
        iRecvNum += worker_iter->second.iRecvNum;
        iRecvByte += worker_iter->second.iRecvByte;
        iSendNum += worker_iter->second.iSendNum;
        iSendByte += worker_iter->second.iSendByte;
        iClientNum += worker_iter->second.iClientNum;
        oMember.Clear();
        oMember.Add("load", worker_iter->second.iLoad);
        oMember.Add("connect", worker_iter->second.iConnect);
        oMember.Add("recv_num", worker_iter->second.iRecvNum);
        oMember.Add("recv_byte", worker_iter->second.iRecvByte);
        oMember.Add("send_num", worker_iter->second.iSendNum);
        oMember.Add("send_byte", worker_iter->second.iSendByte);
        oMember.Add("client", worker_iter->second.iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
    oMsgBody.set_body(oReportData.ToString());
    oMsgHead.set_cmd(CMD_REQ_NODE_STATUS_REPORT);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    LOG4_TRACE("%s()：  %s", __FUNCTION__, oReportData.ToString().c_str());

    std::map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.begin();
    for (; center_iter != m_mapCenterMsgShell.end(); ++center_iter)
    {
        if (center_iter->second.iFd == 0)
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            AutoSend(center_iter->first, oMsgHead, oMsgBody);
        }
        else
        {
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            SendTo(center_iter->second, oMsgHead, oMsgBody);
        }
    }
    return(true);
}

bool OssManager::SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    int iErrno = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = 0;
    std::map<int, tagConnectionAttr*>::iterator worker_conn_iter;
    std::unordered_map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        worker_conn_iter = m_mapFdAttr.find(worker_iter->second.iControlFd);
        if (worker_conn_iter != m_mapFdAttr.end())
        {
            worker_conn_iter->second->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
            if (oMsgHead.msgbody_len() != 0) 
            {
                worker_conn_iter->second->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            }
            iNeedWriteLen = worker_conn_iter->second->pSendBuff->ReadableBytes();
            LOG4_DEBUG("send cmd %d seq %llu to worker %d", oMsgHead.cmd(), oMsgHead.seq(), worker_iter->second.iWorkerIndex);
            iWriteLen = worker_conn_iter->second->pSendBuff->WriteFD(worker_conn_iter->first, iErrno);
            if (iWriteLen < 0)
            {
                if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                {
                    LOG4_ERROR("send to worker by control fd %d error %d: %s",
                               worker_conn_iter->first, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                }
                else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                {
                    worker_conn_iter->second->dActiveTime = ev_now(m_loop);
                    AddIoWriteEvent(worker_conn_iter);
                }
            }
            else if (iWriteLen > 0)
            {
                worker_conn_iter->second->dActiveTime = ev_now(m_loop);
                if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                {
                    LOG4_DEBUG("send to worker %d success, data len %d, cmd %d seq %llu",
                                    worker_iter->second.iWorkerIndex, iWriteLen, oMsgHead.cmd(), oMsgHead.seq());
                    RemoveIoWriteEvent(worker_conn_iter->first);
                }
                else    // 内容未写完，添加或保持监听fd写事件
                {
                    AddIoWriteEvent(worker_conn_iter);
                }
            }
            worker_conn_iter->second->pSendBuff->Compact(32784);
        }
    }
    return(true);
}

bool OssManager::DisposeDataFromWorker(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, 
                                       const MsgBody& oInMsgBody, loss::CBuffer* pSendBuff)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    if (CMD_REQ_UPDATE_WORKER_LOAD == oInMsgHead.cmd())    // 新请求
    {
        std::map<int, int>::iterator iter = m_mapWorkerFdPid.find(stMsgShell.iFd);
        if (iter != m_mapWorkerFdPid.end())
        {
            loss::CJsonObject oJsonLoad;
            oJsonLoad.Parse(oInMsgBody.body());
            SetWorkerLoad(iter->second, oJsonLoad);
        }
    }
    else if (CMD_REQ_ALARM_TOMANAGER == oInMsgHead.cmd()) 
    {
        LOG4_TRACE("recv worker alarm info");
        loss::CJsonObject oJsonLoad;
        oJsonLoad.Parse(oInMsgBody.body());
        SendAlarmToMonitWorker(oJsonLoad);
    }
    else
    {
        LOG4_WARN("unknow cmd %d from worker!", oInMsgHead.cmd());
    }
    return(true);
}

bool OssManager::DisposeDataAndTransferFd(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, loss::CBuffer* pSendBuff)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    int iErrno = 0;
    ConnectWorker oConnWorker;
    MsgHead oOutMsgHead; oOutMsgHead.Clear();
    MsgBody oOutMsgBody; oOutMsgBody.Clear();
    OrdinaryResponse oRes;
    LOG4_TRACE("oInMsgHead.cmd() = %u, seq() = %u", oInMsgHead.cmd(), oInMsgHead.seq());
    if (oInMsgHead.cmd() == CMD_REQ_CONNECT_TO_WORKER)
    {
        if (oConnWorker.ParseFromString(oInMsgBody.body()))
        {
            int iToConWorkIndex = oConnWorker.worker_index();
            LOG4_DEBUG("conn worker data: %s", oConnWorker.DebugString().c_str());

            int retNo = ERR_OK;
            std::string retMsg;
            do { 
                std::map<int, int>::iterator itWIdPid = m_mapWorkerIdPid.find(iToConWorkIndex);
                if (itWIdPid == m_mapWorkerIdPid.end()) 
                {
                    LOG4_ERROR("not find worker index: %d for worker", iToConWorkIndex);
                    retNo = ERR_SERVERINFO;
                    retMsg = "not find worker index";
                    break;
                }

                int iWorkerPid = itWIdPid->second;
                std::unordered_map<int, tagWorkerAttr>::iterator itWorkAttr = m_mapWorker.find(iWorkerPid);
                if (itWorkAttr == m_mapWorker.end()) 
                {
                    LOG4_ERROR("not find worker pid: %u", iWorkerPid);
                    retMsg = "not find worker pid";
                    retNo = ERR_SERVERINFO;
                    break;
                }

                int iManagerDataFd = itWorkAttr->second.iDataFd;
                char szIp[16] = {0};
                strncpy(szIp, "0.0.0.0", 16);   // 内网其他Server的IP不重要
                int iTryNums = 3;
                while(iTryNums -- > 0) 
                {
                    iErrno = send_fd_with_attr(iManagerDataFd, stMsgShell.iFd, szIp, 16, loss::CODEC_PROTOBUF);

                    if (iErrno != 0)
                    {
                        if (iErrno != EAGAIN && iErrno != EINTR)
                        {
                            LOG4_ERROR("transfer errno: %d, errmsg: %s, manager data fd: %d, transfered fd: %u, to worker id: %u, try nums: %u",
                                       iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen),
                                       iManagerDataFd, stMsgShell.iFd, iToConWorkIndex ,iTryNums);
                            retMsg = "transfer client connected fd to worker failed";
                            retNo = ERR_SERVERINFO;
                        }
                    }
                    else
                    {
                        retNo = ERR_OK;
                        retMsg = "Ok";
                        break;
                    }
                }

                if (iErrno != 0) 
                {
                    LOG4_ERROR("transfer errno: %d, errmsg: %s, manager data fd: %d, transfered fd: %u, to worker id: %u, total try nums: %u",
                               iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen),
                               iManagerDataFd, stMsgShell.iFd, iToConWorkIndex ,3);
                }
                //
            } while(0);
    
            oRes.set_err_no(retNo);
            oRes.set_err_msg(retMsg);
            oOutMsgBody.set_body(oRes.SerializeAsString());
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            oOutMsgHead.set_seq(oInMsgHead.seq());
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            pSendBuff->Write(oOutMsgHead.SerializeAsString().c_str(), oOutMsgHead.ByteSize());
            pSendBuff->Write(oOutMsgBody.SerializeAsString().c_str(), oOutMsgBody.ByteSize());
            pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
        }
    }
    else
    {
        oRes.set_err_no(ERR_UNKNOWN_CMD);
        oRes.set_err_msg("unknow cmd!");
        LOG4_DEBUG("unknow cmd %d!", oInMsgHead.cmd());
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        pSendBuff->Write(oOutMsgHead.SerializeAsString().c_str(), oOutMsgHead.ByteSize());
        pSendBuff->Write(oOutMsgBody.SerializeAsString().c_str(), oOutMsgBody.ByteSize());
        pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
        return(false);
    }
    return(false);
}

bool OssManager::GetMinLoadWorkIndex(unsigned int& workerIndex)
{
    return true;
}

bool OssManager::DisposeDataFromCenter(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody,
                loss::CBuffer* pSendBuff,
                loss::CBuffer* pWaitForSendBuff)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    int iErrno = 0;
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求，直接转发给Worker，并回复Center已收到请求
    {
        if (CMD_REQ_BEAT == oInMsgHead.cmd())   // center发过来的心跳包
        {
            MsgHead oOutMsgHead = oInMsgHead;
            MsgBody oOutMsgBody = oInMsgBody;
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
            return(true);
        }
        SendToWorker(oInMsgHead, oInMsgBody);
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(0);
        oRes.set_err_msg("OK");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
    }
    else    // 回调
    {
        if (CMD_RSP_NODE_REGISTER == oInMsgHead.cmd()) //Manager这层只有向center注册会收到回调，上报状态不收回调或者收到回调不必处理
        {
            loss::CJsonObject oNode(oInMsgBody.body());
            int iErrno = 0;
            oNode.Get("errcode", iErrno);
            if (0 == iErrno)
            {
                oNode.Get("node_id", m_uiNodeId);
                MsgHead oMsgHead;
                oMsgHead.set_cmd(CMD_REQ_REFRESH_NODE_ID);
                oMsgHead.set_seq(oInMsgHead.seq());
                oMsgHead.set_msgbody_len(oInMsgHead.msgbody_len());
                SendToWorker(oMsgHead, oInMsgBody);
                RemoveIoWriteEvent(stMsgShell.iFd);
            }
            else
            {
                LOG4_WARN("register to center error, errcode %d!", iErrno);
            }
        }
        else if (CMD_RSP_CONNECT_TO_WORKER == oInMsgHead.cmd()) // 连接center时的回调
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            TargetWorker oTargetWorker;
            oTargetWorker.set_err_no(0);
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d", m_strHostForServer.c_str(), m_iPortForServer);
            oTargetWorker.set_worker_identify(szWorkerIdentify);
            oTargetWorker.set_node_type(GetNodeType());
            oTargetWorker.set_err_msg("OK");
            oOutMsgBody.set_body(oTargetWorker.SerializeAsString());
            oOutMsgHead.set_cmd(CMD_REQ_TELL_WORKER);
            oOutMsgHead.set_seq(GetSequence());
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            pSendBuff->Write(oOutMsgHead.SerializeAsString().c_str(), oOutMsgHead.ByteSize());
            pSendBuff->Write(oOutMsgBody.SerializeAsString().c_str(), oOutMsgBody.ByteSize());
            pSendBuff->Write(pWaitForSendBuff, pWaitForSendBuff->ReadableBytes());
            int iWriteLen = 0;
            int iNeedWriteLen = pSendBuff->ReadableBytes();
            iWriteLen = pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
            if (iWriteLen < 0)
            {
                if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                {
                    LOG4_ERROR("send to fd %d error %d: %s",
                                    stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                    return(false);
                }
                else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                {
                    AddIoWriteEvent(stMsgShell.iFd);
                }
            }
            else if (iWriteLen > 0)
            {
                if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                {
                    RemoveIoWriteEvent(stMsgShell.iFd);
                }
                else    // 内容未写完，添加或保持监听fd写事件
                {
                    AddIoWriteEvent(stMsgShell.iFd);
                }
            }
        }
    }
    return(true);
}

bool OssManager::QueryNodeTypeByCmd(std::string& sNodeType,const int iCmd) {
  std::map<int, std::string>::iterator it = m_mpCmdNodeType.find(iCmd);
  if (it == m_mpCmdNodeType.end()) {
    LOG4_ERROR("not find any node type for cmd: %u", iCmd);
    return false;
  }
  sNodeType =  it->second;
  return true;
}

bool OssManager::SetSocketFdKeepAlive(int iFd) {
    int iKeepAlive = 1;
    loss::SettigSocketOpt sockOptSet;
    loss::SocketOpt sockOpt;

    sockOpt.Clear();
    sockOpt.SetOptName(SO_KEEPALIVE);
    sockOpt.SetOptFd(iFd);
    sockOpt.SetOptLevel(SOL_SOCKET);
    sockOpt.SetOptVal((void*)&iKeepAlive);
    sockOpt.SetOptLen(sizeof(iKeepAlive));
    sockOptSet.AddOpt(SO_KEEPALIVE, sockOpt);
    if (sockOptSet.SetOpt() == false) {
        LOG4_ERROR("set keepalive fail, %s", sockOptSet.GetErrMsg().c_str());
        return false;
    }
    return true;
}

bool OssManager::SetSocketFdOpt(int iFd, int& iOptVal, int iOptName, int iLevel) {
    loss::SettigSocketOpt sockOptSet;
    loss::SocketOpt sockOpt;

    sockOpt.Clear();
    sockOpt.SetOptName(iOptName);
    sockOpt.SetOptFd(iFd);
    sockOpt.SetOptLevel(iLevel);
    sockOpt.SetOptVal((void*)&iOptVal);
    sockOpt.SetOptLen(sizeof(iOptVal));
    sockOptSet.AddOpt(iOptName, sockOpt);
    if (sockOptSet.SetOpt() == false) {
        LOG4_ERROR("set socketsocket  opt: %u fail, %s", iOptName, sockOptSet.GetErrMsg().c_str());
        return false;
    }
    return true;
}

bool OssManager::AddIoReadEvent(std::map<int, tagConnectionAttr*>::iterator iter) {
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    if (NULL == iter->second->pIoWatcher)
    {
        io_watcher = new ev_io();
        if (io_watcher == NULL)
        {
            LOG4_ERROR("new io_watcher error!");
            return(false);
        }
        tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
        if (pData == NULL)
        {
            LOG4_ERROR("new tagIoWatcherData error!");
            delete io_watcher;
            return(false);
        }
        pData->iFd = iter->first;
        pData->ulSeq = iter->second->ulSeq;
        pData->pManager = this;
        ev_io_init (io_watcher, IoCallback, iter->first, EV_READ);
        iter->second->pIoWatcher = io_watcher;
        io_watcher->data = (void*)pData;
        ev_io_start (m_loop, io_watcher);
    }
    else
    {
        io_watcher = iter->second->pIoWatcher;
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
        ev_io_start (m_loop, io_watcher);
    }
    return true;
}

bool OssManager::AddIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter) {
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    if (NULL == iter->second->pIoWatcher)
    {
        io_watcher = new ev_io();
        if (io_watcher == NULL)
        {
            LOG4_ERROR("new io_watcher error!");
            return(false);
        }
        tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
        if (pData == NULL)
        {
            LOG4_ERROR("new tagIoWatcherData error!");
            delete io_watcher;
            return(false);
        }
        pData->iFd = iter->first;
        pData->ulSeq = iter->second->ulSeq;
        pData->pManager = this;
        ev_io_init (io_watcher, IoCallback, iter->first, EV_WRITE);
        io_watcher->data = (void*)pData;
        iter->second->pIoWatcher = io_watcher;
        ev_io_start (m_loop, io_watcher);
    }
    else
    {
        io_watcher = iter->second->pIoWatcher;
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
        ev_io_start (m_loop, io_watcher);
    }
    return true;
}

//------------------------------------//
NormalRestartWorker::NormalRestartWorker()
    :m_bIsRestarting(false)
{
    m_mpWorkers.clear();
}

NormalRestartWorker::~NormalRestartWorker()
{
    m_bIsRestarting = false;
    m_mpWorkers.clear();
}

bool NormalRestartWorker::AddRestartWorker(int32_t iPid) 
{
    m_mpWorkers[iPid] = true;
    m_bIsRestarting = true;
}

bool NormalRestartWorker::AllWorkersRestart() 
{
   return m_bIsRestarting; 
}

bool NormalRestartWorker::UpdateWorkerRestartSatus(int32_t iPid, int iStatus)
{
    bool bRet = false;
    std::map<int, bool>::iterator it;
    it = m_mpWorkers.find(iPid);
    if (it != m_mpWorkers.end() && it->second == true) 
    {
        m_mpWorkers.erase(it);
        bRet = true;

        if (m_mpWorkers.empty()) 
        {
            SetNoRestart();
        }
    }

    return bRet;
}

bool NormalRestartWorker::IsWorkerRestarting(int32_t iPid) 
{
    if (m_bIsRestarting == false)
    {
        return false;
    }
    std::map<int, bool>::iterator it;
    it = m_mpWorkers.find(iPid);
    if (it == m_mpWorkers.end() || it->second == false)
    {
        return false;
    }
    return true;
}

bool NormalRestartWorker::IsNoRestart()
{
    return !m_bIsRestarting;
}

void NormalRestartWorker::SetNoRestart() 
{
    m_bIsRestarting = false;
    m_mpWorkers.clear();
}
///
} /* namespace oss */
