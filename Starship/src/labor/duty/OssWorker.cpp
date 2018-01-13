/*******************************************************************************
 * Project:  AsyncServer
 * @file     OssWorker.cpp
 * @brief 
 * @author   
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"
#include "unix_util/process_helper.h"
#include "unix_util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "OssWorker.hpp"
#include "codec/ProtoCodec.hpp"
#include "codec/ClientMsgCodec.hpp"
#include "codec/HttpCodec.hpp"
#include "step/Step.hpp"
#include "step/RedisStep.hpp"
#include "step/HttpStep.hpp"
#include "session/Session.hpp"
#include "ctimer/CTimer.hpp"
#include "cmd/Cmd.hpp"
#include "cmd/Module.hpp"
#include "cmd/sys_cmd/CmdConnectWorker.hpp"
#include "cmd/sys_cmd/CmdToldWorker.hpp"
#include "cmd/sys_cmd/CmdUpdateNodeId.hpp"
#include "cmd/sys_cmd/CmdNodeNotice.hpp"
#include "cmd/sys_cmd/CmdBeat.hpp"
#include "step/sys_step/StepIoTimeout.hpp"
#include "step/sys_step/StepSendAlarm.hpp"
#include "util/StringTools.hpp"
#include "json/json.h"
#include <iostream>

namespace oss
{

tagSo::tagSo() : pSoHandle(NULL), pCmd(NULL), iVersion(0)
{
}

tagSo::~tagSo()
{
    if (pCmd != NULL)
    {
        delete pCmd;
        pCmd = NULL;
    }
    if (pSoHandle != NULL)
    {
        dlclose(pSoHandle);
        pSoHandle = NULL;
    }
}

tagModule::tagModule() : pSoHandle(NULL), pModule(NULL), iVersion(0)
{
}

tagModule::~tagModule()
{
    if (pModule != NULL)
    {
        delete pModule;
        pModule = NULL;
    }
    if (pSoHandle != NULL)
    {
        dlclose(pSoHandle);
        pSoHandle = NULL;
    }
}


void OssWorker::TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)watcher->data;
        pWorker->Terminated(watcher);  // timeout，worker进程无响应或与Manager通信通道异常，被manager进程终止时返回
    }
}

void OssWorker::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)watcher->data;
        pWorker->CheckParent();
    }
}

void OssWorker::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
        OssWorker* pWorker = (OssWorker*)pData->pWorker;
        if (revents & EV_READ)
        {
            pWorker->IoRead(pData, watcher);
        }
        if (revents & EV_WRITE)
        {
            pWorker->IoWrite(pData, watcher);
        }
        if (revents & EV_ERROR)
        {
            pWorker->IoError(pData, watcher);
        }
    }
}

void OssWorker::IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
        OssWorker* pWorker = pData->pWorker;
        pWorker->IoTimeout(watcher);
    }
}

void OssWorker::PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)(watcher->data);
#ifndef NODE_TYPE_CENTER
        //pWorker->CheckParent();
#endif
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT, 0);
    ev_timer_start (loop, watcher);
}

void OssWorker::StepTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Step* pStep = (Step*)watcher->data;
        ((OssWorker*)(pStep->GetLabor()))->StepTimeout(pStep, watcher);
    }
}

void OssWorker::SessionTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        ((OssWorker*)pSession->GetLabor())->SessionTimeout(pSession, watcher);
    }
}
void OssWorker::TimerTmOutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents) {
    if (watcher->data != NULL)
    {
      CTimer* pTimer = (CTimer*)watcher->data;
      ((OssWorker*)pTimer->GetLabor())->TimerTimeOut(pTimer, watcher);
    }
}

void OssWorker::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)c->data;
        pWorker->RedisConnect(c, status);
    }
}

void OssWorker::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)c->data;
        pWorker->RedisDisconnect(c, status);
    }
}

void OssWorker::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)c->data;
        pWorker->RedisCmdResult(c, reply, privdata);
    }
}

OssWorker::OssWorker(const std::string& strWorkPath, int iControlFd, int iDataFd, 
                     int iWorkerIndex, loss::CJsonObject& oJsonConf, loss::CJsonObject& oJsonConfSrvName, bool isMonitor )
    : m_pErrBuff(NULL), m_ulSequence(0), m_bInitLogger(false), m_dIoTimeout(480.0), m_strWorkPath(strWorkPath), m_uiNodeId(0),
      m_iManagerControlFd(iControlFd), m_iManagerDataFd(iDataFd), m_iWorkerIndex(iWorkerIndex), m_iWorkerPid(0),
      m_dMsgStatInterval(60.0), m_iMsgPermitNum(60),
      m_iRecvNum(0), m_iRecvByte(0), m_iSendNum(0), m_iSendByte(0),
      m_loop(NULL), m_pCmdConnect(NULL), m_isMonitor(isMonitor)
{
    m_mpCmdNodeType.clear();
    m_mpNodeTypeCmd.clear();
    //
    m_iWorkerPid = getpid();
    m_pErrBuff = new char[gc_iErrBuffLen];
    if (!Init(oJsonConf))
    {
        exit(-1);
    }
    if (!InitSrvNmConf(oJsonConfSrvName)) {
        exit(-1);
    }
    if (!CreateEvents())
    {
        exit(-2);
    }
    PreloadCmd();
    LoadSo(oJsonConf["so"]);
    LoadModule(oJsonConf["module"]);
}

OssWorker::~OssWorker()
{
    Destroy();
}

void OssWorker::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

void OssWorker::Terminated(struct ev_signal* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iSignum = watcher->signum;
    delete watcher;
    //Destroy();
    LOG4_FATAL("terminated by signal %d!", iSignum);
    exit(iSignum);
}

bool OssWorker::CheckParent()
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager process exist, worker %d exit.", m_iWorkerIndex);
        //Destroy();
        exit(0);
    }
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    loss::CJsonObject oJsonLoad;
    oJsonLoad.Add("load", int32(m_mapFdAttr.size() + m_mapCallbackStep.size()));
    oJsonLoad.Add("connect", int32(m_mapFdAttr.size()));
    oJsonLoad.Add("recv_num", m_iRecvNum);
    oJsonLoad.Add("recv_byte", m_iRecvByte);
    oJsonLoad.Add("send_num", m_iSendNum);
    oJsonLoad.Add("send_byte", m_iSendByte);
    oJsonLoad.Add("client", int32(m_mapFdAttr.size() - m_mapInnerFd.size()));
    LOG4_TRACE("%s", oJsonLoad.ToString().c_str());
    oMsgBody.set_body(oJsonLoad.ToString());
    oMsgHead.set_cmd(CMD_REQ_UPDATE_WORKER_LOAD);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(m_iManagerControlFd);
    if (iter != m_mapFdAttr.end())
    {
    	int iErrno = 0;
    	iter->second->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
    	iter->second->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
    	iter->second->pSendBuff->WriteFD(m_iManagerControlFd, iErrno);
    	iter->second->pSendBuff->Compact(8192);
    }
    m_iRecvNum = 0;
    m_iRecvByte = 0;
    m_iSendNum = 0;
    m_iSendByte = 0;
//    uint32 uiLoad = m_mapFdAttr.size() + m_mapCallbackStep.size();
//    write(m_iManagerControlFd, &uiLoad, sizeof(uiLoad));    // 向父进程上报当前进程负载
    return(true);
}

bool OssWorker::IoRead(tagIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (watcher->fd == m_iManagerDataFd)
    {
        return(FdTransfer());
    }
    else
    {
        return(RecvDataAndDispose(pData, watcher));
    }
}

bool OssWorker::RecvDataAndDispose(tagIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
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
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, conn_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pWorker = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        conn_iter->second->pRecvBuff->Compact(8192);
        iReadLen = conn_iter->second->pRecvBuff->ReadFD(pData->iFd, iErrno);
        LOG4_TRACE("recv from fd %d data len %d. "
                        "and conn_iter->second->pRecvBuff->ReadableBytes() = %d, read_idx=%d, codec = %d", pData->iFd, iReadLen,
                        conn_iter->second->pRecvBuff->ReadableBytes(), conn_iter->second->pRecvBuff->GetReadIndex(),
                        conn_iter->second->eCodecType);
        if (iReadLen > 0)
        {
            m_iRecvByte += iReadLen;
            conn_iter->second->ulMsgNumUnitTime++;      // TODO 这里要做发送消息频率限制，有待补充
            conn_iter->second->ulMsgNum++;
            MsgHead oInMsgHead, oOutMsgHead;
            MsgBody oInMsgBody, oOutMsgBody;
            StarshipCodec* pCodec = NULL;
            std::map<loss::E_CODEC_TYPE, StarshipCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                {
                    LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                    DestroyConnect(conn_iter);
                }
                return(false);
            }
            while (conn_iter->second->pRecvBuff->ReadableBytes() >= gc_uiClientMsgHeadSize)
            {
                oInMsgHead.Clear();
                oInMsgBody.Clear();
                E_CODEC_STATUS eCodecStatus = codec_iter->second->Decode(conn_iter->second->pRecvBuff, oInMsgHead, oInMsgBody);
//                LOG4_TRACE("eCodecStatus=%d, "
//                                "conn_iter->second->pRecvBuff->ReadableBytes() = %d, read_idx=%d", eCodecStatus,
//                                conn_iter->second->pRecvBuff->ReadableBytes(), conn_iter->second->pRecvBuff->GetReadIndex());
                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    LOG4_TRACE("recv connect status %u", conn_iter->second->ucConnectStatus);
                    if (conn_iter->second->ucConnectStatus != CMD_RSP_TELL_WORKER
                                    && oInMsgHead.has_cmd())   // 连接尚未完成
                    {
                        conn_iter->second->ucConnectStatus = (unsigned char)oInMsgHead.cmd();
                    }
                    ++m_iRecvNum;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    bool bDisposeResult = false;
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = conn_iter->second->ulSeq;
                    if (oInMsgHead.has_cmd() && oInMsgHead.has_seq() && oInMsgHead.cmd() != 0)  // 基于TCP的自定义协议请求或带cmd、seq自定义头域的http请求
                    {
#ifdef NODE_TYPE_ACCESS
                        if (conn_iter->second->pClientData != NULL)
                        {
                            if (conn_iter->second->pClientData->ReadableBytes() > 0)
                            {
                                oInMsgBody.set_additional(conn_iter->second->pClientData->GetRawReadBuffer(),
                                                conn_iter->second->pClientData->ReadableBytes());
                                oInMsgHead.set_msgbody_len(oInMsgBody.ByteSize());
                            }
                            else //if (loss::CODEC_HTTP != conn_iter->second->eCodecType) // 如果是http短连接，则不需要验证，满足上面的(oInMsgHead.cmd() != 0)条件的话不需要再做此检查
                            {
                                std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stMsgShell.iFd);
                                if (inner_iter == m_mapInnerFd.end() && conn_iter->second->ulMsgNum > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                                {
                                    LOG4_DEBUG("invalid request, please login first!");
                                    DestroyConnect(conn_iter);
                                    return(false);
                                }
                            }
                        }
                        else //if (loss::CODEC_HTTP != conn_iter->second->eCodecType) // 如果是http短连接，则不需要验证，满足上面的(oInMsgHead.cmd() != 0)条件的话不需要再做此检查
                        {
                            std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stMsgShell.iFd);
                            if (inner_iter == m_mapInnerFd.end() && conn_iter->second->ulMsgNum > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                            {
                                LOG4_DEBUG("invalid request, please login first!");
                                DestroyConnect(conn_iter);
                                return(false);
                            }
                        }
#endif

                        if ((gc_uiCmdReq & oInMsgHead.cmd()) && oInMsgHead.has_seq())
                        {
                            conn_iter->second->ulForeignSeq = oInMsgHead.seq();
                        }
                        bDisposeResult = Dispose(stMsgShell, oInMsgHead, oInMsgBody, oOutMsgHead, oOutMsgBody); // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
                        std::map<int, tagConnectionAttr*>::iterator dispose_conn_iter = m_mapFdAttr.find(pData->iFd);
                        if (dispose_conn_iter == m_mapFdAttr.end())     // 连接已断开，资源已回收
                        {
                            return(true);
                        }
                        else
                        {
                            if (pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
                            {
                                return(true);
                            }
                        }
                        if (oOutMsgHead.ByteSize() > 0)
                        {
                            eCodecStatus = codec_iter->second->Encode(oOutMsgHead, oOutMsgBody, conn_iter->second->pSendBuff);
                            if (CODEC_STATUS_OK == eCodecStatus)
                            {
                                conn_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
                                conn_iter->second->pSendBuff->Compact(8192);
                                if (iErrno != 0)
                                {
                                    LOG4_ERROR("error %d %s!",
                                                    iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                                }
                            }
                        }
                    }
                    else            // url方式的http请求
                    {
                        HttpMsg oInHttpMsg;
                        HttpMsg oOutHttpMsg;
                        if (oInHttpMsg.ParseFromString(oInMsgBody.body()))
                        {
                            conn_iter->second->dKeepAlive = 10;   // 未带KeepAlive参数，默认10秒钟关闭
                            for (int i = 0; i < oInHttpMsg.headers_size(); ++i)
                            {
                                if (std::string("Keep-Alive") == oInHttpMsg.headers(i).header_name())
                                {
                                    conn_iter->second->dKeepAlive = strtoul(oInHttpMsg.headers(i).header_value().c_str(), NULL, 10);
                                    AddIoTimeout(conn_iter->first, conn_iter->second->ulSeq, conn_iter->second, conn_iter->second->dKeepAlive);
                                    break;
                                }
                                else if (std::string("Connection") == oInHttpMsg.headers(i).header_name())
                                {
                                    if (std::string("keep-alive") == oInHttpMsg.headers(i).header_value())
                                    {
                                        conn_iter->second->dKeepAlive = 60.0;
                                        AddIoTimeout(conn_iter->first, conn_iter->second->ulSeq, conn_iter->second, 60.0);
                                        break;
                                    }
                                    else if (std::string("close") == oInHttpMsg.headers(i).header_value())
                                    {   // 作为客户端请求远端http服务器，对方response后要求客户端关闭连接
                                        conn_iter->second->dKeepAlive = -1;
                                        LOG4_DEBUG("std::string(\"close\") == oInHttpMsg.headers(i).header_value()");
                                        DestroyConnect(conn_iter);
                                        break;
                                    }
                                }
                            }
                            bDisposeResult = Dispose(stMsgShell, oInHttpMsg, oOutHttpMsg);  // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
                            std::map<int, tagConnectionAttr*>::iterator dispose_conn_iter = m_mapFdAttr.find(pData->iFd);
                            if (dispose_conn_iter == m_mapFdAttr.end())     // 连接已断开，资源已回收
                            {
                                return(true);
                            }
                            else
                            {
                                if (pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
                                {
                                    return(true);
                                }
                            }
                            if (conn_iter->second->dKeepAlive < 0)
                            {
                                if (HTTP_RESPONSE == oInHttpMsg.type())
                                {
                                    LOG4_DEBUG("if (HTTP_RESPONSE == oInHttpMsg.type())");
                                    DestroyConnect(conn_iter);
                                }
                                else
                                {
                                    ((HttpCodec*)codec_iter->second)->AddHttpHeader("Connection", "close");
                                }
                            }
                            if (oOutHttpMsg.ByteSize() > 0)
                            {
                                eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oOutHttpMsg, conn_iter->second->pSendBuff);
                                if (CODEC_STATUS_OK == eCodecStatus)
                                {
                                    conn_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
                                    conn_iter->second->pSendBuff->Compact(8192);
                                    if (iErrno != 0)
                                    {
                                        LOG4_ERROR("error %d %s!",
                                                        iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                                    }
                                }
                            }
                        }
                        else
                        {
                            LOG4_ERROR("oInHttpMsg.ParseFromString() error!");
                        }
                    }
                    if (!bDisposeResult)
                    {
                        break;
                    }
                }
                else if (CODEC_STATUS_ERR == eCodecStatus)
                {
                    if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                    {
                        LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                        DestroyConnect(conn_iter);
                    }
                    return(false);
                }
                else
                {
                    break;  // 数据尚未接收完整
                }
            }
            return(true);
        }
        else if (iReadLen == 0)
        {
            LOG4_DEBUG("fd %d closed by peer, error %d %s!",
                            pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
            {
                //del 连接资源.
                const std::string& sIdentify  = conn_iter->second->strIdentify; //todo:
                LOG4_TRACE("fd: %u, peer worker: %s, conn seq: %lu", 
                           pData->iFd, sIdentify.c_str(), conn_iter->second->ulSeq);
                std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(sIdentify);
                if (shell_iter != m_mapMsgShell.end()) {
                  m_mapMsgShell.erase(shell_iter);
                }
#if 0
                std::map<int, tagConnectionAttr*>::iterator iterFdAttr = m_mapFdAttr.find(pData->iFd);
                if (iterFdAttr != m_mapFdAttr.end()) {
                  tagMsgShell conShell;
                  conShell.iFd = pData->iFd;
                  conShell.ulSeq = iterFdAttr->second->ulSeq;
                  std::map<std::string, tagMsgShell>::iterator iterShell = m_mapMsgShell.begin();
                  for ( ; iterShell != m_mapMsgShell.end(); ) {
                    if (iterShell->second.iFd  ==  conShell.iFd &&  iterShell->second.ulSeq == conShell.ulSeq) {
                      LOG4_TRACE("del identify from map of identity-shell,fd: %u, create seq: %lu, peer node info: %s",
                                 conShell.iFd, conShell.ulSeq, iterShell->first.c_str());
                      m_mapMsgShell.erase(iterShell++);
                    } else {
                      iterShell++;
                    }
                  }
                }
#endif

                LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                DestroyConnect(conn_iter);
            }
        }
        else
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_ERROR("recv from fd %d error %d: %s",
                                pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                {
                    LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                    oss::tagMsgShell failMsgShell;
                    failMsgShell.iFd = pData->iFd;
                    failMsgShell.ulSeq = conn_iter->second->ulSeq;
                    UpdateFailReqStat(failMsgShell);
                    DestroyConnect(conn_iter);
                }
            }
        }
    }
    return(false);
}

bool OssWorker::FdTransfer()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    int iCodec = 0;
    // int iAcceptFd = recv_fd(m_iManagerDataFd);
    int iAcceptFd = recv_fd_with_attr(m_iManagerDataFd, szIpAddr, 16, &iCodec);
    if (iAcceptFd <= 0)
    {
        if (iAcceptFd == 0)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d len %d", m_iManagerDataFd, iAcceptFd);
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
        else if (errno != EAGAIN)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d error %d", m_iManagerDataFd, errno);
            //Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        uint32 ulSeq = GetSequence();
        tagConnectionAttr* pConnAttr = CreateFdAttr(iAcceptFd, ulSeq, loss::E_CODEC_TYPE(iCodec));
        x_sock_set_block(iAcceptFd, 0);
        if (pConnAttr)
        {
            strncpy(pConnAttr->pRemoteAddr, szIpAddr, 16);
            LOG4_DEBUG("pConnAttr->pClientAddr = %s, iCodec = %d", pConnAttr->pRemoteAddr, iCodec);
            std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iAcceptFd);
            if(AddIoTimeout(iAcceptFd, ulSeq, iter->second, 1.5))     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在第一次超时检查（或正常发送第一个http数据包）之后才采用正常配置的网络IO超时检查
            {
                if (!AddIoReadEvent(iter))
                {
                    LOG4_DEBUG("if (!AddIoReadEvent(iter))");
                    DestroyConnect(iter);
                    return(false);
                }
//                if (!AddIoErrorEvent(iAcceptFd))
//                {
//                    DestroyConnect(iter);
//                    return(false);
//                }
                return(true);
            }
            else
            {
                LOG4_DEBUG("if(AddIoTimeout(iAcceptFd, ulSeq, iter->second, 1.5)) else");
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

bool OssWorker::IoWrite(tagIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator attr_iter =  m_mapFdAttr.find(pData->iFd);
    if (attr_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (pData->ulSeq != attr_iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, attr_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pWorker = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        int iErrno = 0;
        int iNeedWriteLen = (int)attr_iter->second->pSendBuff->ReadableBytes();
        int iWriteLen = 0;
        iWriteLen = attr_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
        attr_iter->second->pSendBuff->Compact(8192);
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_TRACE("if (EAGAIN != iErrno && EINTR != iErrno)");
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
            m_iSendByte += iWriteLen;
            attr_iter->second->dActiveTime = ev_now(m_loop);
            if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
            {
                RemoveIoWriteEvent(attr_iter);
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
                if (index_iter != m_mapSeq2WorkerIndex.end())   // 系统内部Server间通信需由OssManager转发
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    AddInnerFd(stMsgShell); //内部服务s-s和m-w间连接
                    if (loss::CODEC_PROTOBUF == attr_iter->second->eCodecType)  // 系统内部Server间通信
                    {
                        LOG4_TRACE("fd: %u, seq: %lu, peer work index: %u", 
                                   stMsgShell.iFd, stMsgShell.ulSeq, index_iter->second);
                        m_pCmdConnect->Start(stMsgShell, index_iter->second);
                    }
                    else        // 与系统外部Server通信，连接成功后直接将数据发送
                    {
                        SendTo(stMsgShell);
                    }
                    m_mapSeq2WorkerIndex.erase(index_iter);
                    LOG4_TRACE("RemoveIoWriteEvent(%d)", pData->iFd);
                    RemoveIoWriteEvent(attr_iter);    // 在m_pCmdConnect的两个回调之后再把等待发送的数据发送出去
                }
                else // 与系统外部Server通信，连接成功后直接将数据发送
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    SendTo(stMsgShell);
                }
            }
        }
        return(true);
    }
}

bool OssWorker::IoError(tagIoWatcherData* pData, struct ev_io* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        LOG4_TRACE("if (iter == m_mapFdAttr.end()) else");
        if (pData->ulSeq != iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pWorker = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        DestroyConnect(iter);
        return(true);
    }
}

bool OssWorker::IoTimeout(struct ev_timer* watcher, bool bCheckBeat)
{
    bool bRes = false;
    tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
    if (pData == NULL)
    {
        LOG4_ERROR("pData is null in %s()", __FUNCTION__);
        ev_timer_stop(m_loop, watcher);
        pData->pWorker = NULL;
        delete pData;
        watcher->data = NULL;
        delete watcher;
        watcher = NULL;
        return(false);
    }
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
            if (bCheckBeat && iter->second->dKeepAlive == 0)     // 需要发送心跳检查 或 完成心跳检查并未超时
            {
                ev_tstamp after = iter->second->dActiveTime - ev_now(m_loop) + m_dIoTimeout;
                if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
                {
                    ev_timer_stop (m_loop, watcher);
                    ev_timer_set (watcher, after, 0);
                    ev_timer_start (m_loop, watcher);
                    return(true);
                }
                else    // IO已超时，发送心跳检查
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = iter->second->ulSeq;
                    StepIoTimeout* pStepIoTimeout = new StepIoTimeout(stMsgShell, watcher);
                    if (pStepIoTimeout != NULL)
                    {
                        if (RegisterCallback(pStepIoTimeout))
                        {
                            oss::E_CMD_STATUS eStatus = pStepIoTimeout->Emit(ERR_OK);
                            if (STATUS_CMD_RUNNING != eStatus)
                            {
                                // pStepIoTimeout->Start()会发送心跳包，若干返回非running状态，则表明发包时已出错，
                                // 销毁连接过程在SentTo里已经完成，这里不需要再销毁连接
                                DeleteCallback(pStepIoTimeout);
                            }
                            else
                            {
                                return(true);
                            }
                        }
                        else
                        {
                            LOG4_TRACE("if (RegisterCallback(pStepIoTimeout)) else");
                            delete pStepIoTimeout;
                            pStepIoTimeout = NULL;
                            DestroyConnect(iter);
                        }
                    }
                }
            }
            else        // 心跳检查过后的超时已是实际超时，关闭文件描述符并清理相关资源
            {
                LOG4_TRACE("if (bCheckBeat && iter->second->dKeepAlive == 0)");
                DestroyConnect(iter);
            }
            bRes = true;
        }
    }

    ev_timer_stop(m_loop, watcher);
    pData->pWorker = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

bool OssWorker::StepTimeout(Step* pStep, struct ev_timer* watcher)
{
    ev_tstamp after = pStep->GetActiveTime() - ev_now(m_loop) + pStep->GetTimeout();
    if (after > 0)    // 在定时时间内被重新刷新过，重新设置定时器
    {
//        LOG4_TRACE("%s(pStep 0x%X, seq %lu): active_time %lf, now_time %lf, timeout %lf, reset timeout to %lf",
//                        __FUNCTION__, &pStep, pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout(), after);
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after, 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("%s(seq %lu): active_time %lf, now_time %lf, timeout %lf",
                        __FUNCTION__, pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout());
        E_CMD_STATUS eResult = pStep->Timeout();
        if (STATUS_CMD_RUNNING == eResult)      // 超时Timeout()里重新执行Start()，重新设置定时器
        {
            ev_tstamp after = pStep->GetTimeout();
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, after, 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            //TODO: add fail req conn stat
            tagMsgShell toDelConnShell;
            if (true == GetStepofConnFd(pStep, toDelConnShell)) {
              UpdateFailReqStat(toDelConnShell);
            }
            
            DelStepFdRelation(pStep);
            DeleteCallback(pStep);
            return(true);
        }
    }
}

bool OssWorker::TimerTimeOut(CTimer* pTimer, struct ev_timer* watcher) {
  if (pTimer == NULL) {
    return false;
  }

  LOG4_TRACE("timer: %s has timed", pTimer->GetTimerId().c_str());
  if (STATUS_CMD_RUNNING == pTimer->TimerTmOut()) {

    ev_timer_stop (m_loop, watcher);
    ev_timer_set (watcher, pTimer->GetTimerTm(), 0);
    ev_timer_start (m_loop, watcher);
    return(true);
  } else {
    DeleteCallback(pTimer);
    return  true;
  }
}

bool OssWorker::SessionTimeout(Session* pSession, struct ev_timer* watcher)
{
    ev_tstamp after = pSession->GetActiveTime() - ev_now(m_loop) + pSession->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after, 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("%s(session_name: %s,  session_id: %s)",
                        __FUNCTION__, pSession->GetSessionClass().c_str(), pSession->GetSessionId().c_str());
        if (STATUS_CMD_RUNNING == pSession->Timeout())   // 定时器时间到，执行定时操作，session时间刷新
        {
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, pSession->GetTimeout(), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            DeleteCallback(pSession);
            return(true);
        }
    }
}

bool OssWorker::RedisConnect(const redisAsyncContext *c, int status)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        if (status == REDIS_OK)
        {
            attr_iter->second->bIsReady = true;
            int iCmdStatus;
            for (std::list<RedisStep*>::iterator step_iter = attr_iter->second->listWaitData.begin();
                            step_iter != attr_iter->second->listWaitData.end(); )
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
                const char* argv[args_size];
                size_t arglen[args_size];
                argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
                arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
                std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
                for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
                {
                    argv[i] = c_iter->first.c_str();
                    arglen[i] = c_iter->first.size();
                }
                iCmdStatus = redisAsyncCommandArgv((redisAsyncContext*)c, RedisCmdCallback, NULL, args_size, argv, arglen);
                if (iCmdStatus == REDIS_OK)
                {
                    LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                    attr_iter->second->listData.push_back(pRedisStep);
                    attr_iter->second->listWaitData.erase(step_iter++);
                }
                else    // 命令执行失败，不再继续执行，等待下一次回调
                {
                    break;
                }
            }
        }
        else
        {
            for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                            step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                pRedisStep->Callback(c, status, NULL);
                delete pRedisStep;
            }
            attr_iter->second->listWaitData.clear();
            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
    }
    return(true);
}

bool OssWorker::RedisDisconnect(const redisAsyncContext *c, int status)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listData.begin();
                        step_iter != attr_iter->second->listData.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listData.clear();

        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                        step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listWaitData.clear();

        delete attr_iter->second;
        attr_iter->second = NULL;
        DelRedisContextAddr(c);
        m_mapRedisAttr.erase(attr_iter);
    }
    return(true);
}

bool OssWorker::RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        std::list<RedisStep*>::iterator step_iter = attr_iter->second->listData.begin();
        if (NULL == reply)
        {
            std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(c);
            if (identify_iter != m_mapContextIdentify.end())
            {
                LOG4_ERROR("redis %s error %d: %s", identify_iter->second.c_str(), c->err, c->errstr);
            }
            for ( ; step_iter != attr_iter->second->listData.end(); ++step_iter)
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
                (*step_iter)->Callback(c, c->err, (redisReply*)reply);
                delete (*step_iter);
                (*step_iter) = NULL;
            }
            attr_iter->second->listData.clear();

            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
        else
        {
            if (step_iter != attr_iter->second->listData.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->GetRedisCmd()->ToString().c_str());
                /** @note 注意，若Callback返回STATUS_CMD_RUNNING，框架不回收并且不再管理该RedisStep，该RedisStep后续必须重新RegisterCallback或由开发者自己回收 */
                if (STATUS_CMD_RUNNING != (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply))
                {
                    delete (*step_iter);
                    (*step_iter) = NULL;
                }
                attr_iter->second->listData.erase(step_iter);
                //freeReplyObject(reply);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }
    }
    return(true);
}

bool OssWorker::Pretreat(Cmd* pCmd)
{
    LOG4_TRACE("%s(Cmd*)", __FUNCTION__);
    if (pCmd == NULL)
    {
        return(false);
    }
    pCmd->SetLabor(this);
    pCmd->SetLogger(&m_oLogger);
    return(true);
}

bool OssWorker::Pretreat(Step* pStep)
{
    LOG4_TRACE("%s(Step*)", __FUNCTION__);
    if (pStep == NULL)
    {
        return(false);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    return(true);
}

bool OssWorker::Pretreat(Session* pSession)
{
    LOG4_TRACE("%s(Session*)", __FUNCTION__);
    if (pSession == NULL)
    {
        return(false);
    }
    pSession->SetLabor(this);
    pSession->SetLogger(&m_oLogger);
    return(true);
}

bool OssWorker::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(Step* %p, timeout %lf)", __FUNCTION__, pStep, dTimeout);
    if (pStep == NULL)
    {
        return(false);
    }
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));

    std::pair<std::map<uint32, Step*>::iterator, bool> ret
        = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
    if (ret.second)
    {
        ev_timer* pTimeoutWatcher = (ev_timer*)malloc(sizeof(ev_timer));
        if (pTimeoutWatcher == NULL)
        {
            return(false);
        }
        if (0.0 == dTimeout)
        {
            pStep->SetTimeout(m_dStepTimeout);
        }
        else
        {
            pStep->SetTimeout(dTimeout);
        }
        ev_timer_init (pTimeoutWatcher, StepTimeoutCallback, pStep->GetTimeout(), 0.0);
        pStep->m_pTimeoutWatcher = pTimeoutWatcher;
        pTimeoutWatcher->data = (void*)pStep;
        ev_timer_start (m_loop, pTimeoutWatcher);
        LOG4_TRACE("Step(seq %u, timeout %lf) register successful.", pStep->GetSequence(), pStep->GetTimeout());
    }
    return(ret.second);
}

void OssWorker::DeleteCallback(Step* pStep)
{
    LOG4_TRACE("%s(Step* %p)", __FUNCTION__, pStep);
    if (pStep == NULL)
    {
        return;
    }
    if (pStep->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pStep->m_pTimeoutWatcher);
    }
    std::map<uint32, Step*>::iterator iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("delete step(seq %u)", pStep->GetSequence());
//        delete iter->second;
//        iter->second = NULL;
        delete pStep;
        pStep = NULL;
        m_mapCallbackStep.erase(iter);
    }
}

bool OssWorker::UnRegisterCallback(Step* pStep)
{
    LOG4_TRACE("%s(Step* 0x%X)", __FUNCTION__, &pStep);
    if (pStep == NULL)
    {
        return(false);
    }
    if (pStep->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pStep->m_pTimeoutWatcher);
    }
    std::map<uint32, Step*>::iterator iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("unRigester step(seq %u)", pStep->GetSequence());
        pStep->UnsetRegistered();
        m_mapCallbackStep.erase(iter);
    }
    return(true);
}

bool OssWorker::RegisterCallback(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X, timeout %lf)", __FUNCTION__, &pSession, pSession->GetTimeout());
    if (pSession == NULL)
    {
        return(false);
    }
    if (pSession->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pSession->SetLabor(this);
    pSession->SetLogger(&m_oLogger);
    pSession->SetRegistered();
    pSession->SetActiveTime(ev_now(m_loop));

    std::pair<std::map<std::string, Session*>::iterator, bool> ret;
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter == m_mapCallbackSession.end())
    {
        std::map<std::string, Session*> mapSession;
        ret = mapSession.insert(std::pair<std::string, Session*>(pSession->GetSessionId(), pSession));
        m_mapCallbackSession.insert(std::pair<std::string, std::map<std::string, Session*> >(pSession->GetSessionClass(), mapSession));
    }
    else
    {
        ret = name_iter->second.insert(std::pair<std::string, Session*>(pSession->GetSessionId(), pSession));
    }
    if (ret.second)
    {
        if (pSession->GetTimeout() != 0)
        {
            ev_timer* pTimeoutWatcher = (ev_timer*)malloc(sizeof(ev_timer));
            if (pTimeoutWatcher == NULL)
            {
                return(false);
            }
            ev_timer_init (pTimeoutWatcher, SessionTimeoutCallback, pSession->GetTimeout(), 0.0);
            pSession->m_pTimeoutWatcher = pTimeoutWatcher;
            pTimeoutWatcher->data = (void*)pSession;
            ev_timer_start (m_loop, pTimeoutWatcher);
        }
        LOG4_TRACE("Session(session_id %s) register successful.", pSession->GetSessionId().c_str());
    }
    return(ret.second);
}

bool OssWorker::DeleteCallback(CTimer* pTimer) {
  LOG4_TRACE("%s(timer)", __FUNCTION__);
  if (pTimer == NULL) { 
    return true;
  }

  if (pTimer->GetWatcher() != NULL) {
    ev_timer_stop (m_loop,pTimer->GetWatcher());
  }
  std::string sId = pTimer->GetTimerId();

  std::map<std::string, CTimer*>::iterator it = m_mpTimers.find(sId);
  if (it != m_mpTimers.end()) {
    LOG4_TRACE("now delete timer: %s", sId.c_str());
    delete it->second;
    it->second = NULL;
    m_mpTimers.erase(it);
  }
  return true;
}

void OssWorker::DeleteCallback(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X)", __FUNCTION__, &pSession);
    if (pSession == NULL)
    {
        return;
    }
    if (pSession->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pSession->m_pTimeoutWatcher);
    }
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter != m_mapCallbackSession.end())
    {
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(pSession->GetSessionId());
        if (id_iter != name_iter->second.end())
        {
            LOG4_TRACE("delete session(session_id %s)", pSession->GetSessionId().c_str());
            delete id_iter->second;
            id_iter->second = NULL;
            name_iter->second.erase(id_iter);
        }
    }
}

bool OssWorker::RegisterCallback(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pRedisStep == NULL)
    {
        return(false);
    }
    if (pRedisStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pRedisStep->SetLabor(this);
    pRedisStep->SetLogger(&m_oLogger);
    pRedisStep->SetRegistered();
    /* redis回调暂不作超时处理
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        return(false);
    }
    tagIoWatcherData* pData = new tagIoWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagIoWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, IoTimeoutCallback, 0.5, 0.0);
    pData->ullSeq = pStep->GetSequence();
    pData->pWorker = this;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    */

    std::map<redisAsyncContext*, tagRedisAttr*>::iterator iter = m_mapRedisAttr.find((redisAsyncContext*)pRedisContext);
    if (iter == m_mapRedisAttr.end())
    {
        LOG4_ERROR("redis attr not exist!");
        return(false);
    }
    else
    {
        LOG4_TRACE("iter->second->bIsReady = %d", iter->second->bIsReady);
        if (iter->second->bIsReady)
        {
            int status;
            size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
            const char* argv[args_size];
            size_t arglen[args_size];
            argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
            arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
            std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
            for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
            {
                argv[i] = c_iter->first.c_str();
                arglen[i] = c_iter->first.size();
            }
            status = redisAsyncCommandArgv((redisAsyncContext*)pRedisContext, RedisCmdCallback, NULL, args_size, argv, arglen);
            if (status == REDIS_OK)
            {
                LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                iter->second->listData.push_back(pRedisStep);
                return(true);
            }
            else
            {
                LOG4_ERROR("redis status %d!", status);
                return(false);
            }
        }
        else
        {
            LOG4_TRACE("listWaitData.push_back()");
            iter->second->listWaitData.push_back(pRedisStep);
            return(true);
        }
    }
}

Session* OssWorker::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        char szSession[16] = {0};
        snprintf(szSession, sizeof(szSession), "%u", uiSessionId);
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(szSession);
        if (id_iter == name_iter->second.end())
        {
            return(NULL);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

Session* OssWorker::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(strSessionId);
        if (id_iter == name_iter->second.end())
        {
            return(NULL);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

//bool OssWorker::RegisterCallback(Session* pSession)
//{
//    if (pSession == NULL)
//    {
//        return(false);
//    }
//    pSession->SetWorker(this);
//    pSession->SetLogger(m_oLogger);
//    pSession->SetRegistered();
//    ev_timer* timeout_watcher = new ev_timer();
//    if (timeout_watcher == NULL)
//    {
//        return(false);
//    }
//    uint32* pUlSeq = new uint32;
//    if (pUlSeq == NULL)
//    {
//        delete timeout_watcher;
//        return(false);
//    }
//    ev_timer_init (timeout_watcher, StepTimeoutCallback, 60, 0.);
//    *pUllSeq = pSession->GetSequence();
//    timeout_watcher->data = (void*)pUllSeq;
//    ev_timer_start (m_loop, timeout_watcher);
//
//    std::pair<std::map<uint32, Session*>::iterator, bool> ret
//        = m_mapCallbackSession.insert(std::pair<uint32, Session*>(pSession->GetSequence(), pSession));
//    return(ret.second);
//}
//
//void OssWorker::DeleteCallback(Session* pSession)
//{
//    if (pSession == NULL)
//    {
//        return;
//    }
//    std::map<uint32, Session*>::iterator iter = m_mapCallbackSession.find(pSession->GetSequence());
//    if (iter != m_mapCallbackSession.end())
//    {
//        delete iter->second;
//        iter->second = NULL;
//        m_mapCallbackSession.erase(iter);
//    }
//}

bool OssWorker::Disconnect(const tagMsgShell& stMsgShell, bool bMsgShellNotice)
{
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter != m_mapFdAttr.end())
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            LOG4_TRACE("if (iter->second->ulSeq == stMsgShell.ulSeq)");
            return(DestroyConnect(iter, bMsgShellNotice));
        }
    }
    return(false);
}

bool OssWorker::Disconnect(const std::string& strIdentify, bool bMsgShellNotice)
{
    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        return(true);
    }
    else
    {
        return(Disconnect(shell_iter->second, bMsgShellNotice));
    }
}

bool OssWorker::SetProcessName(const loss::CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    ngx_setproctitle(szProcessName);
    return(true);
}
//
//// conf format is:{"down_stream":[{"node_type":"logic","child_proc_num":1,"ip":"127.0.0.1","port":3360}]}
////
bool OssWorker::ParseDownStreamNodeInfo(loss::CJsonObject& jsonDownStream)
{
    LOG4_DEBUG("run func: %s",__FUNCTION__);
    if( jsonDownStream.IsEmpty() )
    {
        LOG4_INFO("downstream conf not config");
        return true;
    }
    for(unsigned int iIndex = 0; iIndex < jsonDownStream.GetArraySize(); iIndex ++)
    {
        loss::CJsonObject jsonItem = jsonDownStream[iIndex];
        std::string node_type_val = jsonItem("node_type");

        int child_proc_num_val = 0;
        if (jsonItem("child_proc_num").empty()) {
          child_proc_num_val = PROC_NUM_WORKER_DEF;
        } else {
          child_proc_num_val  = atoi(jsonItem("child_proc_num").c_str());
        }
        std::string ip_val = jsonItem("ip");
        std::string port_val = jsonItem("port");
        
        std::map<std::string,std::set<SerNodeInfo*> >::iterator it;
        it = m_NodeType_NodeInfo.find(node_type_val);
        if( it == m_NodeType_NodeInfo.end() )
        {
            SerNodeInfo *SetNodes = new SerNodeInfo;
            SetNodes->SetIp(ip_val);
            SetNodes->SetPort(atoi(port_val.c_str()));
            SetNodes->SetPeerWorkNums(child_proc_num_val);
            std::set<SerNodeInfo*> SetSerInfo;
            SetSerInfo.insert(SetNodes);
            m_NodeType_NodeInfo.insert(std::pair<std::string,std::set<SerNodeInfo*> >(node_type_val,SetSerInfo) );
        }
        else 
        {
            std::set<SerNodeInfo*>::iterator itNode ;
            for(itNode = it->second.begin(); itNode != it->second.end();itNode++)
            {
                if( (*itNode)->strIp == ip_val && (*itNode)->uiPort == atoi(port_val.c_str()) && \
                    (*itNode)->uiWorkNums == child_proc_num_val)
                    break;
            }
            if( itNode == it->second.end() )
            {
                SerNodeInfo *SetNodes = new SerNodeInfo;
                SetNodes->SetIp(ip_val);
                SetNodes->SetPort(atoi(port_val.c_str()));
                SetNodes->SetPeerWorkNums(child_proc_num_val);
                it = m_NodeType_NodeInfo.find(node_type_val);
                it->second.insert(SetNodes);
            }
        }
    }
    return true;
}

//json format is: {"****": [1,2,3,4,5,6], "**":[1,2,3,4,6]}
//parse json and fill node type cmd map 
bool OssWorker::ParseNodeTypeCmdConf(const loss::CJsonObject& jsNodeTypeCmd) {
  if (jsNodeTypeCmd.IsEmpty()) {
    LOG4_ERROR("node type cmd conf json is empty");
    return false;
  }

  std::string strJsonCnf = jsNodeTypeCmd.ToString();
  Json::Value jsRoot;
  Json::Reader reader;
  if (false == reader.parse(strJsonCnf.c_str(), jsRoot)) {
    LOG4_ERROR("parse node type cmd  json file fail,content: %s",
               strJsonCnf.c_str());
    return false;
  }

  Json::Value::Members vKeyList = jsRoot.getMemberNames();
  if (vKeyList.empty()) {
    LOG4_ERROR("json key list is empty");
    return false;
  }

  std::vector<std::string>::iterator  it; //iterator of nodetype vector 
  for (it = vKeyList.begin(); it != vKeyList.end(); ++it) {
    Json::Value vCmdList =  jsRoot[*it];
    if (vCmdList.isArray() == false) {
      LOG4_TRACE("cmd list i empty,nodetype: %s",(*it).c_str());
      continue;
    }

    Json::Value::ArrayIndex indexCmd =  0; 
    for (; indexCmd < vCmdList.size(); indexCmd++) {
      if (vCmdList[indexCmd].isInt() == false) {
        LOG4_ERROR("cmd not int format, js key: %s",(*it).c_str());
        continue;
      }

      //insert <cmd, nodetype> into m_mpCmdNodeType;
      int iCmd = vCmdList[indexCmd].asInt();
      std::pair<ITerCmdNodeType,bool> bInsert = m_mpCmdNodeType.insert(std::pair<int, std::string>(iCmd,(*it)));
      if (bInsert.second == false) {
        LOG4_TRACE("insert cmd exist, cmd: %d, node type: %s", iCmd, (*it).c_str());
        continue;
      }
      LOG4_TRACE("insert into cmd nodetye node into map");
      
      //insert <nodetype, int> into m_mpNodeTypeCmd;
      ITerNodeTypeCmd itNodeTypeCmd = m_mpNodeTypeCmd.find(*it);
      if (itNodeTypeCmd == m_mpNodeTypeCmd.end()) {
        std::set<int> setCmdNodeType;
        setCmdNodeType.insert(bInsert.first->first);
        m_mpNodeTypeCmd.insert(std::pair<std::string, std::set<int> >(*it, setCmdNodeType));
        LOG4_TRACE("insert new cmd list into map of node type cmd, nodetype: %s", (*it).c_str());

      } else {
        std::set<int>& setIter = itNodeTypeCmd->second;
        if (setIter.insert(bInsert.first->first).second == false) {
          LOG4_ERROR("insert cmd_nodetyp iterator into node type cmd map failed, as exist");
        } else {
          LOG4_TRACE("insert nodetype cmd into map");
        }
      }
    }
  }  
  return true; 
}

bool OssWorker::Init(loss::CJsonObject& oJsonConf)
{

    char szProcessName[64] = {0};
    if (m_isMonitor == true) {
      snprintf(szProcessName, sizeof(szProcessName), "%s_Monitor_%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    } else {
      snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    }
    ngx_setproctitle(szProcessName);
    oJsonConf.Get("io_timeout", m_dIoTimeout);
    if (!oJsonConf.Get("step_timeout", m_dStepTimeout))
    {
        std::cerr << "parse step_timeout failed, use def val: " << 0.5 << std::endl;
        m_dStepTimeout = 0.5;
    }
    oJsonConf.Get("node_type", m_strNodeType);
    oJsonConf.Get("inner_host", m_strHostForServer);
    oJsonConf.Get("inner_port", m_iPortForServer);
#ifdef NODE_TYPE_ACCESS
    oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_dMsgStatInterval);
    oJsonConf["permission"]["uin_permit"].Get("permit_num", m_iMsgPermitNum);
#endif
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }
    m_oCustomConf = oJsonConf["custom"];
    LOG4_DEBUG("host conf content: %s",oJsonConf.ToString().c_str());
    //ParseDownStreamNodeInfo(oJsonConf["down_stream"]);
    
    StarshipCodec* pCodec = new ProtoCodec(loss::CODEC_PROTOBUF);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<loss::E_CODEC_TYPE, StarshipCodec*>(loss::CODEC_PROTOBUF, pCodec));
    pCodec = new HttpCodec(loss::CODEC_HTTP);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<loss::E_CODEC_TYPE, StarshipCodec*>(loss::CODEC_HTTP, pCodec));
    pCodec = new ClientMsgCodec(loss::CODEC_PRIVATE);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<loss::E_CODEC_TYPE, StarshipCodec*>(loss::CODEC_PRIVATE, pCodec));
    m_pCmdConnect = new CmdConnectWorker();
    if (m_pCmdConnect == NULL)
    {
        return(false);
    }
    m_pCmdConnect->SetLogger(&m_oLogger);
    m_pCmdConnect->SetLabor(this);
    return(true);
}


bool OssWorker::InitSrvNmConf(loss::CJsonObject& oJsonConf) {
  LOG4_DEBUG("srv name content: %s",oJsonConf.ToString().c_str());
  if (false == ParseDownStreamNodeInfo(oJsonConf["down_stream"])) {
    LOG4_ERROR("parse down stream srv name json failed");
    return true;
  }
  return true;
}

bool OssWorker::InitLogger(const loss::CJsonObject& oJsonConf)
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
        std::string strLogname = m_strWorkPath + std::string("/") + oJsonConf("log_path")
                        + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        if (oJsonConf.Get("log_level", iLogLevel))
        {
            switch (iLogLevel)
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
                    iLogLevel = log4cplus::INFO_LOG_LEVEL;
            }
        }
        else
        {
            iLogLevel = log4cplus::INFO_LOG_LEVEL;
        }
        log4cplus::initialize();
        log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        append->setName(strLogname);
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        append->setLayout(layout);
        m_oLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(strLogname));
        m_oLogger.addAppender(append);
        m_oLogger.setLogLevel(iLogLevel);
        LOG4_INFO("%s begin...", getproctitle());
        m_bInitLogger = true;
        return(true);
    }
}

bool OssWorker::CreateEvents()
{
    m_loop = ev_loop_new(EVFLAG_AUTO);
    if (m_loop == NULL)
    {
        return(false);
    }

    signal(SIGPIPE, SIG_IGN);
    // 注册信号事件
    ev_signal* signal_watcher = new ev_signal();
    ev_signal_init (signal_watcher, TerminatedCallback, SIGINT);
    signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, signal_watcher);

    AddPeriodicTaskEvent();
    // 注册闲时处理事件         注册idle事件在Server空闲时会导致CPU占用过高，暂时弃用之，改用定时器实现
//    ev_idle* idle_watcher = new ev_idle();
//    ev_idle_init (idle_watcher, IdleCallback);
//    idle_watcher->data = (void*)this;
//    ev_idle_start (m_loop, idle_watcher);

    // 注册网络IO事件
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(m_iManagerControlFd, ulSeq))
    {
        tagMsgShell stMsgShell;
        stMsgShell.iFd = m_iManagerControlFd;
        stMsgShell.ulSeq = ulSeq;
        std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(m_iManagerControlFd);
        if (!AddIoReadEvent(iter))
        {
            LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
            DestroyConnect(iter);
            return(false);
        }
        AddInnerFd(stMsgShell);
//        if (!AddIoErrorEvent(m_iManagerControlFd))
//        {
//            DestroyConnect(iter);
//            return(false);
//        }
    }
    else
    {
        return(false);
    }

    // 注册网络IO事件
    ulSeq = GetSequence();
    if (CreateFdAttr(m_iManagerDataFd, ulSeq))
    {
        tagMsgShell stMsgShell;
        stMsgShell.iFd = m_iManagerDataFd;
        stMsgShell.ulSeq = ulSeq;
        std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(m_iManagerDataFd);
        if (!AddIoReadEvent(iter))
        {
            LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
            DestroyConnect(iter);
            return(false);
        }
        AddInnerFd(stMsgShell);
//        if (!AddIoErrorEvent(m_iManagerDataFd))
//        {
//            DestroyConnect(iter);
//            return(false);
//        }
    }
    else
    {
        return(false);
    }
    return(true);
}

void OssWorker::PreloadCmd()
{
    Cmd* pCmdToldWorker = new CmdToldWorker();
    pCmdToldWorker->SetCmd(CMD_REQ_TELL_WORKER);
    pCmdToldWorker->SetLogger(&m_oLogger);
    pCmdToldWorker->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdToldWorker->GetCmd(), pCmdToldWorker));

    Cmd* pCmdUpdateNodeId = new CmdUpdateNodeId();
    pCmdUpdateNodeId->SetCmd(CMD_REQ_REFRESH_NODE_ID);
    pCmdUpdateNodeId->SetLogger(&m_oLogger);
    pCmdUpdateNodeId->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdUpdateNodeId->GetCmd(), pCmdUpdateNodeId));

    Cmd* pCmdNodeNotice = new CmdNodeNotice();
    pCmdNodeNotice->SetCmd(CMD_REQ_NODE_REG_NOTICE);
    pCmdNodeNotice->SetLogger(&m_oLogger);
    pCmdNodeNotice->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdNodeNotice->GetCmd(), pCmdNodeNotice));

    Cmd* pCmdBeat = new CmdBeat();
    pCmdBeat->SetCmd(CMD_REQ_BEAT);
    pCmdBeat->SetLogger(&m_oLogger);
    pCmdBeat->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdBeat->GetCmd(), pCmdBeat));
}

void OssWorker::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);

    m_mapHttpAttr.clear();

    for (std::map<int32, Cmd*>::iterator cmd_iter = m_mapCmd.begin();
                    cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
    }
    m_mapCmd.clear();

    for (std::map<int, tagSo*>::iterator so_iter = m_mapSo.begin();
                    so_iter != m_mapSo.end(); ++so_iter)
    {
        delete so_iter->second;
        so_iter->second = NULL;
    }
    m_mapSo.clear();

    for (std::map<std::string, tagModule*>::iterator module_iter = m_mapModule.begin();
                    module_iter != m_mapModule.end(); ++module_iter)
    {
        delete module_iter->second;
        module_iter->second = NULL;
    }
    m_mapModule.clear();

    for (std::map<int, tagConnectionAttr*>::iterator attr_iter = m_mapFdAttr.begin();
                    attr_iter != m_mapFdAttr.end(); ++attr_iter)
    {
        LOG4_TRACE("for (std::map<int, tagConnectionAttr*>::iterator attr_iter = m_mapFdAttr.begin();");
        DestroyConnect(attr_iter);
    }

    for (std::map<loss::E_CODEC_TYPE, StarshipCodec*>::iterator codec_iter = m_mapCodec.begin();
                    codec_iter != m_mapCodec.end(); ++codec_iter)
    {
        delete codec_iter->second;
        codec_iter->second = NULL;
    }
    m_mapCodec.clear();

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

    for (std::map<std::string, CTimer*>::iterator it = m_mpTimers.begin(); it != m_mpTimers.end(); ++it) {
      if (it->second) { delete it->second; it->second = NULL; }
    }
    m_mpTimers.clear();

}

void OssWorker::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool OssWorker::AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s(%s, fd %d, seq %u)", __FUNCTION__, strIdentify.c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        m_mapMsgShell.insert(std::pair<std::string, tagMsgShell>(strIdentify, stMsgShell));
    }
    else
    {
        if ((stMsgShell.iFd != shell_iter->second.iFd || stMsgShell.ulSeq != shell_iter->second.ulSeq))
        {
            LOG4_DEBUG("connect to %s was exist, replace old fd %d with new fd %d",
                            strIdentify.c_str(), shell_iter->second.iFd, stMsgShell.iFd);
            std::map<int, tagConnectionAttr*>::iterator fd_iter = m_mapFdAttr.find(shell_iter->second.iFd);
            if (GetWorkerIdentify() != strIdentify)
            {
                LOG4_TRACE("GetWorkerIdentify() != strIdentify");
                DestroyConnect(fd_iter);
            }
            shell_iter->second = stMsgShell;
        }
    }
    SetConnectIdentify(stMsgShell, strIdentify);
    return(true);
}

void OssWorker::DelMsgShell(const std::string& strIdentify)
{
    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        ;
    }
    else
    {
        m_mapMsgShell.erase(shell_iter);
    }

    // 连接虽然断开，但不应清除节点标识符，这样可以保证下次有数据发送时可以重新建立连接
//    std::map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
//    if (identify_iter != m_mapIdentifyNodeType.end())
//    {
//        std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
//        node_type_iter = m_mapNodeIdentify.find(identify_iter->second);
//        if (node_type_iter != m_mapNodeIdentify.end())
//        {
//            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
//            if (id_iter != node_type_iter->second.second.end())
//            {
//                node_type_iter->second.second.erase(id_iter);
//                node_type_iter->second.first = node_type_iter->second.second.begin();
//            }
//        }
//        m_mapIdentifyNodeType.erase(identify_iter);
//    }
}

void OssWorker::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::map<std::string, std::string>::iterator iter = m_mapIdentifyNodeType.find(strIdentify);
    if (iter == m_mapIdentifyNodeType.end())
    {
        m_mapIdentifyNodeType.insert(iter,
                std::pair<std::string, std::string>(strIdentify, strNodeType));

        T_MAP_NODE_TYPE_IDENTIFY::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(strNodeType);
        if (node_type_iter == m_mapNodeIdentify.end())
        {
            std::set<std::string> setIdentify;
            setIdentify.insert(strIdentify);
            std::pair<T_MAP_NODE_TYPE_IDENTIFY::iterator, bool> insert_node_result;
            insert_node_result = m_mapNodeIdentify.insert(std::pair< std::string,
                            std::pair<std::set<std::string>::iterator, std::set<std::string> > >(
                                            strNodeType, std::make_pair(setIdentify.begin(), setIdentify)));    //这里的setIdentify是临时变量，setIdentify.begin()将会成非法地址
            if (insert_node_result.second == false)
            {
                return;
            }
            insert_node_result.first->second.first = insert_node_result.first->second.second.begin();
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter == node_type_iter->second.second.end())
            {
                node_type_iter->second.second.insert(strIdentify);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
    }
}

bool OssWorker::UpDateNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo)
{
    unsigned int uiPort;
    unsigned int uiWorkNums;

    std::map<std::string,std::set<SerNodeInfo*> >::iterator mNodeIter = m_NodeType_NodeInfo.find(strNodeType);
    if( mNodeIter != m_NodeType_NodeInfo.end() )
    {
      return true;
    }
    //
    std::vector<std::string> vsHostInfo;
    loss::StringTools::Split(strNodeInfo, ":", vsHostInfo);
    
    if (vsHostInfo.size() < 3) {
      LOG4_ERROR("parse node info failed, origin nodeinfo: %s",strNodeInfo.c_str());
      return false;
    }

    std::string strIp =  vsHostInfo.at(0);
    std::string strPort = vsHostInfo.at(1);
    std::string strWorkerIndex = vsHostInfo.at(2);
    
    uiPort = atoi(strPort.c_str());
    uiWorkNums = atoi(strWorkerIndex.c_str());

    SerNodeInfo *SetNodes = new SerNodeInfo;
    SetNodes->SetIp(strIp);
    SetNodes->SetPort(uiPort);

    std::set<SerNodeInfo*> SetSerInfo;
    SetSerInfo.insert(SetNodes);
    std::string sNodeType = strNodeType;
    m_NodeType_NodeInfo.insert(std::pair<std::string, std::set<SerNodeInfo*> >(sNodeType,SetSerInfo));
    return true;
}


void OssWorker::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
    if (identify_iter != m_mapIdentifyNodeType.end())
    {
        std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(identify_iter->second);
        if (node_type_iter != m_mapNodeIdentify.end())
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter != node_type_iter->second.second.end())
            {
                node_type_iter->second.second.erase(id_iter);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
        m_mapIdentifyNodeType.erase(identify_iter);
    }
}

bool OssWorker::RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(strIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_DEBUG("redis context %s", strIdentify.c_str());
        return(RegisterCallback(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_DEBUG("GetLabor()->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool OssWorker::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    char szIdentify[32] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_TRACE("redis context %s", szIdentify);
        return(RegisterCallback(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_TRACE("GetLabor()->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

/*
bool OssWorker::RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep)
{
    std::map<std::string, std::set<std::string> >::iterator conf_iter = m_mapRedisNodeConf.find(strRedisNodeType);
    if (conf_iter == m_mapRedisNodeConf.end())
    {
        LOG4_ERROR("error: no redis node type \"%s\"!", strRedisNodeType.c_str());
        return(false);
    }
    else
    {
        // TODO 从配置中通过hash算法查找到对应redis的标识（hash算法暂未实现）
        std::set<std::string>::iterator identify_iter = conf_iter->second.begin();

        std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(*identify_iter);
        if (ctx_iter != m_mapRedisContext.end())
        {
            LOG4_TRACE("redis context %s", (*identify_iter).c_str());
            return(RegisterCallback(ctx_iter->second, pRedisStep));
        }
        else
        {
            int iPosIpPortSeparator = (*identify_iter).find(':');
            std::string strHost = (*identify_iter).substr(0, iPosIpPortSeparator);
            int iPort = atoi((*identify_iter).substr(iPosIpPortSeparator + 1, std::string::npos).c_str());
            LOG4_TRACE("GetLabor()->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
            return(AutoRedisCmd(strHost, iPort, pRedisStep));
        }
    }
}

void OssWorker::AddRedisNodeConf(const std::string strNodeType, const std::string strHost, int iPort)
{
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, std::set<std::string> >::iterator node_iter = m_mapRedisNodeConf.find(strNodeType);
    if (node_iter == m_mapRedisNodeConf.end())
    {
        std::set<std::string> setIdentify;
        setIdentify.insert(szIdentify);
        m_mapRedisNodeConf.insert(std::pair<std::string, std::set<std::string> >(strNodeType, setIdentify));
    }
    else
    {
        node_iter->second.insert(szIdentify);
    }
}

void OssWorker::DelRedisNodeConf(const std::string strNodeType, const std::string strHost, int iPort)
{
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, std::set<std::string> >::iterator node_iter = m_mapRedisNodeConf.find(strNodeType);
    if (node_iter != m_mapRedisNodeConf.end())
    {
        node_iter->second.erase(node_iter->second.find(szIdentify));
    }
}
*/

bool OssWorker::AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
{
    LOG4_TRACE("%s(%s, %d, 0x%X)", __FUNCTION__, strHost.c_str(), iPort, ctx);
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter == m_mapRedisContext.end())
    {
        m_mapRedisContext.insert(std::pair<std::string, const redisAsyncContext*>(szIdentify, ctx));
        std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
        if (identify_iter == m_mapContextIdentify.end())
        {
            m_mapContextIdentify.insert(std::pair<const redisAsyncContext*, std::string>(ctx, szIdentify));
        }
        else
        {
            identify_iter->second = szIdentify;
        }
        return(true);
    }
    else
    {
        return(false);
    }
}

void OssWorker::DelRedisContextAddr(const redisAsyncContext* ctx)
{
    std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
    if (identify_iter != m_mapContextIdentify.end())
    {
        std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(identify_iter->second);
        if (ctx_iter != m_mapRedisContext.end())
        {
            m_mapRedisContext.erase(ctx_iter);
        }
        m_mapContextIdentify.erase(identify_iter);
    }
}

bool OssWorker::SendTo(const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s(fd %d, seq %lu) pWaitForSendBuff", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
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
                iter->second->pSendBuff->Compact(8192);
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
                    m_iSendByte += iWriteLen;
                    iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(iter);
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

bool OssWorker::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(fd %d, fd_seq %lu, cmd %u, msg_seq %u)",
                    __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq, oMsgHead.cmd(), oMsgHead.seq());
    std::map<int, tagConnectionAttr*>::iterator conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->ulSeq == stMsgShell.ulSeq)
        {
            std::map<loss::E_CODEC_TYPE, StarshipCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            LOG4_TRACE("connect status %u", conn_iter->second->ucConnectStatus);
            E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
            if (conn_iter->second->ucConnectStatus < CMD_RSP_TELL_WORKER)   // 连接尚未完成
            {
                if (oMsgHead.cmd() <= CMD_RSP_TELL_WORKER)   // 创建连接的过程
                {
                    E_CODEC_STATUS eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pSendBuff);
                    conn_iter->second->ucConnectStatus = (unsigned char)oMsgHead.cmd();
                }
                else    // 创建连接过程中的其他数据发送请求
                {
                    //LOG4_TRACE("send to with shell, wait to send msg body: %s", oMsgBody.DebugString().c_str());
                    E_CODEC_STATUS eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pWaitForSendBuff);
                    if (CODEC_STATUS_OK == eCodecStatus)
                    {
                        return(true);
                    }
                    return(false);
                }
            }
            else
            {
                //LOG4_TRACE("send to after conn, send msg body: %s", oMsgBody.DebugString().c_str());
                eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pSendBuff);
            }
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_iSendNum;
                int iErrno = 0;
                int iNeedWriteLen = (int)conn_iter->second->pSendBuff->ReadableBytes();
                LOG4_TRACE("try send cmd[%d] seq[%lu] len %d",
                                oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen);
                int iWriteLen = conn_iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                conn_iter->second->pSendBuff->Compact(8192);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(conn_iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                    else
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    m_iSendByte += iWriteLen;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        LOG4_TRACE("cmd[%d] seq[%lu] need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, iWriteLen);
                        RemoveIoWriteEvent(conn_iter);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("cmd[%d] seq[%lu] need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, iWriteLen);
                        AddIoWriteEvent(conn_iter);
                    }
                }
                return(true);
            }
            else
            {
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->ulSeq);
            return(false);
        }
    }
}

//< this interface method not to been called, it's abolished. or strIndentify is ip:port which is from conf file.
bool OssWorker::SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_TRACE("no MsgShell match %s", strIdentify.c_str());
        return(AutoSend(strIdentify, oMsgHead, oMsgBody));
    }
    else
    {
        return(SendTo(shell_iter->second, oMsgHead, oMsgBody));
    }
}

bool OssWorker::GetOptimalNextWorkIndex(const std::string& strNodeType, unsigned int& uiDstWorkIndex,std::string& strWokerIndentity)
{
    //strWorkIndentity: ip + port + workindex.
    std::map<std::string,std::set<SerNodeInfo*> >::iterator iterNode;
    iterNode = m_NodeType_NodeInfo.find(strNodeType);
    if ( iterNode == m_NodeType_NodeInfo.end() )
    {
        LOG4_DEBUG("not find any node info for node_type: %s",strNodeType.c_str());
        return false;
    }

    std::set<SerNodeInfo*>& SetItem = iterNode->second;
    std::set<SerNodeInfo*>::iterator minIter = iterNode->second.begin();
    if (minIter == iterNode->second.end()) {
      LOG4_ERROR("has not any usable node for nodetye: %s", strNodeType.c_str());
      return false;
    }

    int32_t iLastMinIndex = -1;

    static std::vector<int> iAccnt(SetItem.size(), 0);
    int iIndex = 0;
    for(std::set<SerNodeInfo*>::iterator it = SetItem.begin(); it != SetItem.end(); it ++ )
    {
      if ((*it)->uiWorkNums <=0 ) {
        continue;
      }
      //reuse long ago fail connect svr. reuse about before 300s connnect.
      if ((*it)->GetFailReqNums() != 0
          && (time(NULL) - (*it)->tmFailReqTimeStamp > SerNodeInfo::TM_REUSE_FAIL_CONN)) {
        LOG4_TRACE("reset before [%d]s fail for conn srv,ip: %s, port: %u, fail nums: %u, total load: %u",
                   SerNodeInfo::TM_REUSE_FAIL_CONN,(*it)->GetIp().c_str(), (*it)->GetPort(),
                   (*it)->GetFailReqNums(), (*it)->GetFreqence());
        (*it)->ResetFailReqNum();
        //reset load as minimum load in node type list nodes.
        GetNodeMinimumFreqence(*it, iterNode->second);
      }

      if ((*it)->GetFailReqNums() <= (*minIter)->GetFailReqNums()) {
        iIndex ++;
        if ((*it)->GetFailReqNums() < (*minIter)->GetFailReqNums()) {
          minIter = it;
          iLastMinIndex = iIndex;
          continue;
        }

        if ((*it)->GetFreqence() < (*minIter)->GetFreqence()) {
          minIter = it;
          iLastMinIndex = iIndex;
        }
      }
    }

    iIndex = 0;
    if (-1 != iLastMinIndex) {
      iIndex  =  iLastMinIndex;
    }

    //take care boundary condition.
    ProcessReqNumToMaxUint(*minIter, iterNode->second);

    uiDstWorkIndex = (iAccnt[iIndex]++)% (*minIter)->GetWorkNums();
    char chTmpBuf[1024];
    snprintf(chTmpBuf,sizeof(chTmpBuf),"%s:%u:%u",(*minIter)->GetIp().c_str(),(*minIter)->GetPort(),uiDstWorkIndex);
    strWokerIndentity.assign(chTmpBuf,strlen(chTmpBuf));
    (*minIter)->IncrFreqence();

    LOG4_INFO("optic node ip: %s, port: %d, total load: %lu", 
              (*minIter)->GetIp().c_str(), (*minIter)->GetPort(), (*minIter)->GetFreqence());
    return true;
}

void OssWorker::GetNodeMinimumFreqence(SerNodeInfo* pSerNode, std::set<SerNodeInfo*>& sSerNode) {
  if (pSerNode == NULL || sSerNode.empty()) {
    return ;
  }
  pSerNode->ResetFrequence();

  std::set<SerNodeInfo*>::iterator it = sSerNode.begin();
  unsigned int uiMinFreq = 0; 
  for (; it != sSerNode.end(); ++it) {
    if (*it == pSerNode) {
      continue;
    }

    if ((*it)->GetFailReqNums() != 0) {
      continue;
    }

    if (uiMinFreq == 0) {
      uiMinFreq = (*it)->GetFreqence(); 
      continue;
    }

    if (uiMinFreq > (*it)->GetFreqence()) {
      uiMinFreq = (*it)->GetFreqence(); 
    }
  }

  if (pSerNode->GetFreqence() > uiMinFreq) {
    pSerNode->uiFreqence = uiMinFreq;
  }
  LOG4_TRACE("get load: %u, ip: %s, port %u", pSerNode->GetFreqence(), 
             pSerNode->GetIp().c_str(), pSerNode->GetPort());
}

void OssWorker::ProcessReqNumToMaxUint(SerNodeInfo* pSerNode,std::set<SerNodeInfo*>& sSerNode) {
  if (pSerNode == NULL || sSerNode.empty()) {
    return ;
  }

  if (pSerNode->GetFreqence() < UINT_MAX) {
    return ;
  }
  LOG4_INFO("ip: %s, port: %u process req reach: %lu, you are succ on life",
            pSerNode->GetIp().c_str(), pSerNode->GetPort(), UINT_MAX);
  
  std::set<SerNodeInfo*>::iterator it;
  for (it = sSerNode.begin(); it != sSerNode.end(); ++it) {
    if ((*it)->GetFailReqNums() != 0) {
      LOG4_INFO("has fail req, omit reset req load, ip: %s, port %u",
                (*it)->GetIp().c_str(),(*it)->GetPort() );
      continue;
    }
    if (pSerNode == (*it)) {
      continue;
    }
    (*it)->ResetFrequence();
    LOG4_INFO("reset load to 0,ip: %s, port: %u", (*it)->GetIp().c_str(),(*it)->GetPort() );
  }
}

void OssWorker::AutoDelFailNode(const std::string& strNodeType) {
    std::map<std::string,std::set<SerNodeInfo*> >::iterator iterNode;
    iterNode = m_NodeType_NodeInfo.find(strNodeType);
    if ( iterNode == m_NodeType_NodeInfo.end() )
    {
        LOG4_DEBUG("not find any node info for node_type: %s",strNodeType.c_str());
        return ;
    }

    if (iterNode->second.empty()) {
      LOG4_DEBUG("node set is empty for node_type: %s",strNodeType.c_str());
      m_NodeType_NodeInfo.erase(iterNode);
      return ;
    }

    std::set<SerNodeInfo*>::iterator it;
    for (it = iterNode->second.begin(); it != iterNode->second.end();) {
      if ((*it)->GetFailReqNums() >= SerNodeInfo::NUM_FAIL_MAXNUM_PERSEC) {
        LOG4_ERROR("auto del: one serious fail node, type: %s, ip: %s, port: %d, fail num: %d",
                  strNodeType.c_str(), (*it)->GetIp().c_str(), (*it)->GetPort(), (*it)->GetFailReqNums());
        
        std::map<std::string, std::set<SerNodeInfo*> >::iterator itDel = \
                                m_AutoDel_NodeTypeNodeInfo.find(strNodeType);

        if (itDel ==  m_AutoDel_NodeTypeNodeInfo.end()) {
          std::set<SerNodeInfo*> setNode;
          setNode.insert(*it);
          m_AutoDel_NodeTypeNodeInfo[strNodeType] = setNode;
        } else {
          std::set<SerNodeInfo*>& serNode = itDel->second;
          serNode.insert(*it);
        }
        iterNode->second.erase(it++);
        //
        continue;
      }
      //
      ++it;
    }
}

void OssWorker::UpdateFailReqStat(const oss::tagMsgShell& msgShell) {

  if (msgShell.iFd <= 0 || msgShell.ulSeq <= 0) {
    LOG4_ERROR("input shell param invalid");
    return ;
  }

  std::map<int, tagConnectionAttr*>::iterator conn_iter;
  conn_iter = m_mapFdAttr.find(msgShell.iFd);
  if (conn_iter == m_mapFdAttr.end()) {
    LOG4_ERROR("not find exist conn for fd: %d", msgShell.iFd);
    return ;
  }

  if (msgShell.ulSeq != conn_iter->second->ulSeq) {
    LOG4_ERROR("conn seq: %u != %u shell seq", conn_iter->second->ulSeq, msgShell.ulSeq);
    return ;
  }

  const std::string& strIdentify = conn_iter->second->strIdentify;
  if (strIdentify.empty()) {
    LOG4_ERROR("fd: %d, seq: %u, peer client identify is empty", msgShell.iFd, msgShell.ulSeq);
    return ;
  }

  std::vector<std::string> vsHostInfo;
  loss::StringTools::Split(strIdentify, ":", vsHostInfo);

  if (vsHostInfo.size() < 3) {
    LOG4_ERROR("parse node info failed, nodeinfo: %s",strIdentify.c_str());
    return ;
  }

  std::string strIp =  vsHostInfo.at(0);
  std::string strPort = vsHostInfo.at(1);
  int32_t uiPort = atoi(strPort.c_str());
  
  std::map<std::string,std::set<SerNodeInfo*> >::iterator it;
  for (it = m_NodeType_NodeInfo.begin(); it != m_NodeType_NodeInfo.end(); ++it) {
    const std::string& sNodeType = it->first;

    std::set<SerNodeInfo*>::iterator itNode;
    for (itNode =it->second.begin(); itNode != it->second.end(); ++itNode) {
      SerNodeInfo* node = (*itNode);
      if (node == NULL) {
        continue;
      }
      if (node->GetIp() == strIp && node->GetPort() == uiPort) {
        node->IncrFailReqNum();
        LOG4_ERROR("fail conn on ip: %s, port: %u, node type: %s,fail num: %d",
                  strIp.c_str(), uiPort, sNodeType.c_str(), node->GetFailReqNums());
        /////////////
        try {
          AutoDelFailNode(sNodeType);

        } catch (const std::exception& ex) {
          LOG4_ERROR("auto del fail node crash,err: %s", ex.what());
        }
        //////////
      }
    }
  }
  //
  return ;
}

bool OssWorker::UpdateSendToFailStat(const std::string& sNodeType, const std::string& sIpPortWkIndex) {
  try {
    AutoDelFailNode(sNodeType);
  } catch(const std::exception& ex) {
    LOG4_ERROR("auto del fail node crash, err: %s", ex.what());
  }

  if (sIpPortWkIndex.empty()) {
    return false;
  }

  std::vector<std::string> vInfo;
  loss::StringTools::Split(sIpPortWkIndex, ":", vInfo);
  if (vInfo.size() < 3) {
    return false;
  }

  std::string strHost = vInfo.at(0);
  std::string strPort = vInfo.at(1);
  //std::string strWorkerIndex = vInfo.at(2);
  int iPort = atoi(strPort.c_str());
  if (iPort <=0 ) {
    return false;
  }

  std::map<std::string,std::set<SerNodeInfo*> >::iterator iterNode;
  iterNode = m_NodeType_NodeInfo.find(sNodeType);
  if( iterNode == m_NodeType_NodeInfo.end() )
  {
    LOG4_DEBUG("not find any node info for node_type: %s",sNodeType.c_str());
    return false;
  }

  std::set<SerNodeInfo*>& setSerNode = iterNode->second;
  if (setSerNode.empty()) {
    LOG4_ERROR("sernode info empty, node type: %s", sNodeType.c_str());
    return false;
  }

  for (std::set<SerNodeInfo*>::iterator it = setSerNode.begin(); it != setSerNode.end(); ++it) {
    if ((*it)->GetIp() == strHost && (*it)->GetPort() == iPort ) {
      (*it)->IncrFailReqNum();
      LOG4_ERROR("increase req fail num on ip: %s, port: %d, node type: %s, fail num: %d",
                strHost.c_str(), iPort, sNodeType.c_str(), (*it)->GetFailReqNums());
    }
  }
  return true;
}

bool OssWorker::DelStepFdRelation(Step* pStep) {
  if (pStep == NULL) {
    LOG4_ERROR("invaild step handler");
    return false;
  }

  std::map<Step*, tagMsgShell>::iterator it = m_mpSendingStepConnFd.find(pStep); 
  if (it  == m_mpSendingStepConnFd.end()) {
    return true;
  }
  m_mpSendingStepConnFd.erase(it);
  LOG4_TRACE("erase step: %p, from step conn fd relation map", pStep);
  return true;
}

bool OssWorker::GetStepofConnFd(Step* pStep, tagMsgShell& connShell) {
  if (NULL == pStep) {
    LOG4_ERROR("input  step empty");
    return false;
  }
  std::map<Step*, tagMsgShell>::iterator it = m_mpSendingStepConnFd.find(pStep);
  if (it == m_mpSendingStepConnFd.end()) {
    LOG4_ERROR("not find step in step conn fd relation");
    return false;
  }

  connShell = it->second;
  LOG4_TRACE("step: %p, shell fd: %u, shell seq: %u",
             pStep, connShell.iFd, connShell.ulSeq);
  return true;
}

//< this interface method not to been called. it's abolished. strNodeType: ip+port;  workindex allocated by location,not conf by center.
//< [NodeType:ip+port]
bool OssWorker::SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody, Step* pDoStep)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    
    //< allocated donwstream workIndex now:
    unsigned int uiOptimalWkIndex = 0; //it's conf for this server.
    std::string strWorkNode;
    if (false == GetOptimalNextWorkIndex(strNodeType,uiOptimalWkIndex,strWorkNode)) {
      LOG4_ERROR("get optimal worker index failed for worker type: %s", strNodeType.c_str());
      return false;
    }

    LOG4_INFO("get optimal downstream worker: %s, get peer worker id: %u",strWorkNode.c_str(), uiOptimalWkIndex);
    
    bool bSendToRet = true ;
    try {
      bSendToRet = SendTo(strWorkNode,oMsgHead,oMsgBody);
    } catch (std::exception& ex) {
      LOG4_ERROR("catch err: %s", ex.what());
      bSendToRet = false;
    }
    //LOG4_TRACE("sendtonext msg body: %s", oMsgBody.DebugString().c_str());

    if (bSendToRet == false) {
      UpdateSendToFailStat(strNodeType, strWorkNode);
      LOG4_ERROR("add one fail sendnext req ");
    } else {
      std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strWorkNode);
      if (shell_iter == m_mapMsgShell.end()) {
       LOG4_ERROR("not find conn shell for peer node: %s",  strWorkNode.c_str());
       return false;
      }

      if (pDoStep !=  NULL) {
        m_mpSendingStepConnFd[pDoStep] = shell_iter->second;
        LOG4_TRACE("SendToNext, reg step: %p", pDoStep);
      }
    }
    return bSendToRet; 
}

bool OssWorker::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(nody_type: %s, mod_factor: %d)", __FUNCTION__, strNodeType.c_str(), uiModFactor);
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no MsgShell match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.second.size() == 0)
        {
            LOG4_ERROR("no MsgShell match %s!", strNodeType.c_str());
            return(false);
        }
        else
        {
            std::set<std::string>::iterator id_iter;
            int target_identify = uiModFactor % node_type_iter->second.second.size();
            int i = 0;
            for (i = 0, id_iter = node_type_iter->second.second.begin();
                            i < node_type_iter->second.second.size();
                            ++i, ++id_iter)
            {
                if (i == target_identify && id_iter != node_type_iter->second.second.end())
                {
                    return(SendTo(*id_iter, oMsgHead, oMsgBody));
                }
            }
            return(false);
        }
    }
}

bool OssWorker::SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    LOG4_TRACE("%s(fd %d, seq %lu)", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<int, tagConnectionAttr*>::iterator conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("con_seq: %lu, fd %d not  found in m_mapFdAttr", stMsgShell.ulSeq, stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->ulSeq == stMsgShell.ulSeq)
        {
            std::map<loss::E_CODEC_TYPE, StarshipCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            if (loss::CODEC_HTTP != conn_iter->second->eCodecType)
            {
                LOG4_ERROR("the codec for fd %d is not http codec!", stMsgShell.iFd);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus;
            if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
            {
                eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pWaitForSendBuff);
            }
            else
            {
                eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pSendBuff);
            }
            if (CODEC_STATUS_OK == eCodecStatus && conn_iter->second->pSendBuff->ReadableBytes() > 0)
            {
                ++m_iSendNum;
                if ((conn_iter->second->pIoWatcher != NULL) && (conn_iter->second->pIoWatcher->events & EV_WRITE))
                {   // 正在监听fd的写事件，说明发送缓冲区满，此时直接返回，等待EV_WRITE事件再执行WriteFD
                    return(true);
                }
                int iErrno = 0;
                int iNeedWriteLen = (int)conn_iter->second->pSendBuff->ReadableBytes();
                int iWriteLen = conn_iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                conn_iter->second->pSendBuff->Compact(8192);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(conn_iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                    else
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    if (pHttpStep != NULL)
                    {
                        std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
                        if (http_step_iter == m_mapHttpAttr.end())
                        {
                            std::list<uint32> listHttpStepSeq;
                            listHttpStepSeq.push_back(pHttpStep->GetSequence());
                            m_mapHttpAttr.insert(std::pair<int32, std::list<uint32> >(stMsgShell.iFd, listHttpStepSeq));
                        }
                        else
                        {
                            http_step_iter->second.push_back(pHttpStep->GetSequence());
                        }
                    }
                    m_iSendByte += iWriteLen;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(conn_iter);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(conn_iter);
                    }
                }
                return(true);
            }
            else
            {
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->ulSeq);
            return(false);
        }
    }
}

bool OssWorker::SentTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, szIdentify);
    return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, pHttpStep));
    // 向外部发起http请求不复用连接
//    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(szIdentify);
//    if (shell_iter == m_mapMsgShell.end())
//    {
//        LOG4_TRACE("no MsgShell match %s.", szIdentify);
//        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, pHttpStep));
//    }
//    else
//    {
//        return(SendTo(shell_iter->second, oHttpMsg, pHttpStep));
//    }
}

bool OssWorker::SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify)
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
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, iter->second->ulSeq);
            return(false);
        }
    }
}

bool OssWorker::AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
     
    std::vector<std::string> vInfo;
    loss::StringTools::Split(strIdentify, ":", vInfo);
    
    if (vInfo.size() < 3) {
      return false;
    }
    std::string strHost = vInfo.at(0);
    std::string strPort = vInfo.at(1);
    std::string strWorkerIndex = vInfo.at(2);
    int iPort = atoi(strPort.c_str());
    if (iPort <=0 ) {
      return false;
    }

    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    if (iWorkerIndex > 200)
    {
        return(false);
    }
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(iFd);
        if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5))
        {
            conn_iter->second->ucConnectStatus = 0;
            if (!AddIoReadEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
//            if (!AddIoErrorEvent(iFd))
//            {
//                DestroyConnect(iter);
//                return(false);
//            }
            if (!AddIoWriteEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            std::map<loss::E_CODEC_TYPE, StarshipCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pWaitForSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_iSendNum;
            }
            else
            {
                return(false);
            }
           
#if 0
            MsgHead toMsgHead;
            MsgBody toMsgBody;
            E_CODEC_STATUS  deCodeRetTest = codec_iter->second->Decode(conn_iter->second->pWaitForSendBuff, toMsgHead, toMsgBody);
            
            if (deCodeRetTest != CODEC_STATUS_OK) {
              LOG4_ERROR("decode test failed");
            } else {
              LOG4_TRACE("decode head: %s, decode body: %s", toMsgHead.SerializeAsString().c_str() , toMsgBody.SerializeAsString().c_str());
              eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pWaitForSendBuff);
              if (CODEC_STATUS_OK == eCodecStatus)
              {
                ++m_iSendNum;
              }
              else
              {
                return(false);
              }
            }
#endif

//            LOG4_TRACE("fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            iFd, iter->second->pWaitForSendBuff->ReadableBytes());
//
            m_mapSeq2WorkerIndex[ulSeq] = iWorkerIndex;
            //m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
            tagMsgShell stMsgShell;
            stMsgShell.iFd = iFd;
            stMsgShell.ulSeq = ulSeq;
            LOG4_TRACE(" to connnect peer work, fd: %u, seq: %lu, work index: %d",
                       stMsgShell.iFd, stMsgShell.ulSeq, iWorkerIndex);
            AddMsgShell(strIdentify, stMsgShell);
            connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            return(true);
        }
        else
        {
            LOG4_TRACE("if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5)) else");
            DestroyConnect(conn_iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool OssWorker::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    LOG4_TRACE("%s(%s, %d, %s)", __FUNCTION__, strHost.c_str(), iPort, strUrlPath.c_str());
    struct sockaddr_in stAddr;
    tagMsgShell stMsgShell;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    if (stAddr.sin_addr.s_addr == 4294967295 || stAddr.sin_addr.s_addr == 0)
    {
        struct hostent *he;
        he = gethostbyname(strHost.c_str());
        if (he != NULL)
        {
            stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
        }
        else
        {
            LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
            return(false);
        }
    }
    bzero(&(stAddr.sin_zero), 8);
    stMsgShell.iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (stMsgShell.iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(stMsgShell.iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(stMsgShell.iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    stMsgShell.ulSeq = GetSequence();
    tagConnectionAttr* pConnAttr = CreateFdAttr(stMsgShell.iFd, stMsgShell.ulSeq);
    if (pConnAttr)
    {
        pConnAttr->eCodecType = loss::CODEC_HTTP;
        std::map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(stMsgShell.iFd);
        if(AddIoTimeout(stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second, 2.5))
        {
            conn_iter->second->dKeepAlive = 10;
            if (!AddIoReadEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            if (!AddIoWriteEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            std::map<loss::E_CODEC_TYPE, StarshipCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pWaitForSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_iSendNum;
            }
            else
            {
                return(false);
            }
//            LOG4_TRACE("fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            iFd, iter->second->pWaitForSendBuff->ReadableBytes());
            connect(stMsgShell.iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
            if (http_step_iter == m_mapHttpAttr.end())
            {
                std::list<uint32> listHttpStepSeq;
                listHttpStepSeq.push_back(pHttpStep->GetSequence());
                m_mapHttpAttr.insert(std::pair<int32, std::list<uint32> >(stMsgShell.iFd, listHttpStepSeq));
            }
            else
            {
                http_step_iter->second.push_back(pHttpStep->GetSequence());
            }
            return(true);
            // 向外部发起http请求不复用连接
//            char szIdentify[256] = {0};
//            snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
//            return(AddMsgShell(szIdentify, stMsgShell));
        }
        else
        {
            LOG4_TRACE("if(AddIoTimeout(stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second, 2.5)) else");
            DestroyConnect(conn_iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(stMsgShell.iFd);
        return(false);
    }
}

bool OssWorker::AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s() redisAsyncConnect(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        /* Let *c leak for now... */
        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }
    c->data = this;
    tagRedisAttr* pRedisAttr = new tagRedisAttr();
    pRedisAttr->ulSeq = GetSequence();
    pRedisAttr->listWaitData.push_back(pRedisStep);
    pRedisStep->SetLogger(&m_oLogger);
    pRedisStep->SetLabor(this);
    pRedisStep->SetRegistered();
    m_mapRedisAttr.insert(std::pair<redisAsyncContext*, tagRedisAttr*>(c, pRedisAttr));
//    LOG4_TRACE("redisLibevAttach(0x%X, 0x%X)", m_loop, c);
    redisLibevAttach(m_loop, c);
//    LOG4_TRACE("redisAsyncSetConnectCallback(0x%X, 0x%X)", c, RedisConnectCallback);
    redisAsyncSetConnectCallback(c, RedisConnectCallback);
//    LOG4_TRACE("redisAsyncSetDisconnectCallback(0x%X, 0x%X)", c, RedisDisconnectCallback);
    redisAsyncSetDisconnectCallback(c, RedisDisconnectCallback);
//    LOG4_TRACE("RedisStep::AddRedisContextAddr(%s, %d, 0x%X)", strHost.c_str(), iPort, c);
    AddRedisContextAddr(strHost, iPort, c);
    return(true);
}

bool OssWorker::AutoConnect(const std::string& strIdentify)
{
    LOG4_DEBUG("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    if (iWorkerIndex > 200)
    {
        return(false);
    }
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(iFd);
        if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5))
        {
            conn_iter->second->ucConnectStatus = 0;
            if (!AddIoReadEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
//            if (!AddIoErrorEvent(iFd))
//            {
//                DestroyConnect(iter);
//                return(false);
//            }
            if (!AddIoWriteEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
            tagMsgShell stMsgShell;
            stMsgShell.iFd = iFd;
            stMsgShell.ulSeq = ulSeq;
            AddMsgShell(strIdentify, stMsgShell);
            connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            return(true);
        }
        else
        {
            LOG4_TRACE("if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5)) else");
            DestroyConnect(conn_iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

void OssWorker::AddInnerFd(const tagMsgShell& stMsgShell)
{
    std::map<int, uint32>::iterator iter = m_mapInnerFd.find(stMsgShell.iFd);
    if (iter == m_mapInnerFd.end())
    {
        m_mapInnerFd.insert(std::pair<int, uint32>(stMsgShell.iFd, stMsgShell.ulSeq));
    }
    else
    {
        iter->second = stMsgShell.ulSeq;
    }
    LOG4_TRACE("%s() now m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
}

bool OssWorker::GetMsgShell(const std::string& strIdentify, tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_DEBUG("no MsgShell match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        stMsgShell = shell_iter->second;
        return(true);
    }
}

bool OssWorker::SetClientData(const tagMsgShell& stMsgShell, loss::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            iter->second->pClientData->Write(pBuff, pBuff->ReadableBytes());
            return(true);
        }
        else
        {
            return(false);
        }
    }
}

bool OssWorker::HadClientData(const tagMsgShell& stMsgShell)
{
    std::map<int, tagConnectionAttr*>::iterator conn_iter;
    conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (stMsgShell.ulSeq != conn_iter->second->ulSeq)
        {
            return(false);
        }
        if (conn_iter->second->pClientData != NULL)
        {
            if (conn_iter->second->pClientData->ReadableBytes() > 0)
            {
                return(true);
            }
            else
            {
                return(false);
            }
        }
        else
        {
            return(false);
        }
    }
}

std::string OssWorker::GetClientAddr(const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        return("");
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            if (iter->second->pRemoteAddr == NULL)
            {
                return("");
            }
            else
            {
                return(iter->second->pRemoteAddr);
            }
        }
        else
        {
            return("");
        }
    }
}

bool OssWorker::AbandonConnect(const std::string& strIdentify)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_DEBUG("no MsgShell match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(shell_iter->second.iFd);
        if (iter == m_mapFdAttr.end())
        {
            return(false);
        }
        else
        {
            if (iter->second->ulSeq == shell_iter->second.ulSeq)
            {
                iter->second->strIdentify = "";
                iter->second->pClientData->Clear();
                m_mapMsgShell.erase(shell_iter);
                return(true);
            }
            else
            {
                return(false);
            }
        }
        return(true);
    }
}

void OssWorker::LoadSo(loss::CJsonObject& oSoConf)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iCmd = 0;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::map<int, tagSo*>::iterator cmd_iter;
    tagSo* pSo = NULL;
    for (int i = 0; i < oSoConf.GetArraySize(); ++i)
    {
        oSoConf[i].Get("load", bIsload);
        if (bIsload)
        {
            if (oSoConf[i].Get("cmd", iCmd) && oSoConf[i].Get("version", iVersion))
            {
                cmd_iter = m_mapSo.find(iCmd);
                if (cmd_iter == m_mapSo.end())
                {
                    strSoPath = m_strWorkPath + std::string("/") + oSoConf[i]("so_path");
                    pSo = LoadSoAndGetCmd(iCmd, strSoPath, oSoConf[i]("entrance_symbol"), iVersion);
                    if (pSo != NULL)
                    {
                        LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                        m_mapSo.insert(std::pair<int, tagSo*>(iCmd, pSo));
                    }
                }
                else
                {
                    if (iVersion != cmd_iter->second->iVersion)
                    {
                        strSoPath = m_strWorkPath + std::string("/") + oSoConf[i]("so_path");
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
                            continue;
                        }

                        delete cmd_iter->second; cmd_iter->second = NULL;
                        m_mapSo.erase(cmd_iter);
                        
                        pSo = LoadSoAndGetCmd(iCmd, strSoPath, oSoConf[i]("entrance_symbol"), iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
                        if (pSo != NULL)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            m_mapSo.insert(std::pair<int, tagSo*>(iCmd, pSo));
                        }
                    }
                }
            }
        }
        else        // 卸载动态库
        {
            if (oSoConf[i].Get("cmd", iCmd))
            {
                strSoPath = m_strWorkPath + std::string("/") + oSoConf[i]("so_path");
                UnloadSoAndDeleteCmd(iCmd);
                LOG4_INFO("unload %s", strSoPath.c_str());
            }
        }
    }
}

tagSo* OssWorker::LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagSo* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        if (pHandle != NULL)
        {
            dlclose(pHandle);
        }
        return(pSo);
    }
    CreateCmd* pCreateCmd = (CreateCmd*)dlsym(pHandle, strSymbol.c_str());
    dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("dlsym error %s!" , dlsym_error);
        dlclose(pHandle);
        return(pSo);
    }
    Cmd* pCmd = pCreateCmd();
    if (pCmd != NULL)
    {
        pSo = new tagSo();
        if (pSo != NULL)
        {
            pSo->pSoHandle = pHandle;
            pSo->pCmd = pCmd;
            pSo->iVersion = iVersion;
            pSo->pCmd->SetLogger(&m_oLogger);
            pSo->pCmd->SetLabor(this);
            pSo->pCmd->SetConfigPath(m_strWorkPath);
            pSo->pCmd->SetCmd(iCmd);
            if (!pSo->pCmd->Init())
            {
                LOG4_FATAL("Cmd %d %s init error",
                                iCmd, strSoPath.c_str());
                delete pSo;
                pSo = NULL;
            }
        }
        else
        {
            LOG4_FATAL("new tagSo() error!");
            delete pCmd;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void OssWorker::UnloadSoAndDeleteCmd(int iCmd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapSo.find(iCmd);
    if (cmd_iter != m_mapSo.end())
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
        m_mapSo.erase(cmd_iter);
    }
}

//ret: 2: ip,port node exit
//     1: snodetype node exit
//     other: not exist
int32_t OssWorker::CheckNodeInAutoDelSet(const std::string& sNodeType,
                                         const std::string& ip,
                                         int32_t iport) {
  int32_t iRet = 0;
  if (sNodeType.empty()) {
    return iRet;
  }

  std::map<std::string, std::set<SerNodeInfo*> >::iterator it;
  it = m_AutoDel_NodeTypeNodeInfo.find(sNodeType);
  if (it == m_AutoDel_NodeTypeNodeInfo.end()) {
    return iRet; 
  }
  iRet = 1;
  
  if (ip.empty() || iport <= 0) {
    return iRet;
  }

  //
  std::set<SerNodeInfo*>::iterator serIt ;
  for (serIt = it->second.begin(); serIt != it->second.end(); ++serIt) {
    if ((*serIt)->GetIp() == ip && (*serIt)->GetPort() == iport) {
      iRet = 2;
      return iRet;
    }
  }

  return iRet;
}

//ret: 2: ip,port node exit
//     1: snodetype node exit
//     other: not exist
int32_t OssWorker::CheckInToDelNodeList(const std::string& sNodeType,
                                        const std::string& ip,
                                        int32_t port) {
  int32_t iRet = 0;
  if (sNodeType.empty() || ip.empty() || port <= 0) {
    return iRet;
  }

  std::map<std::string, std::set<SerNodeInfo*> >::iterator it;
  it = m_ToDel_NodeTypeNodeInfo.find(sNodeType);
  if (it == m_ToDel_NodeTypeNodeInfo.end()) {
    return iRet;
  }

  std::set<SerNodeInfo*>::iterator serIt ;
  for (serIt = it->second.begin(); serIt != it->second.end(); ++serIt) {
    if ((*serIt)->GetIp() == ip && (*serIt)->GetPort() == port) {
      iRet = 2;
      return iRet;
    }
  }
  return iRet;
}

bool OssWorker::AppendToAddNodeInList(const std::string& sNodeType,
                                      const SerNodeInfo *tempNodeInfo) {
  bool bRet = false;
  if (sNodeType.empty() || tempNodeInfo == NULL) {
    return bRet;
  }

  SerNodeInfo *SetNodes  = NULL;
  std::map<std::string, std::set<SerNodeInfo*> >::iterator it;
  it = m_AppendNodeTypeNodeInfo.find(sNodeType);
  if (it == m_AppendNodeTypeNodeInfo.end()) {
    SetNodes = new SerNodeInfo;
    SetNodes->SetIp(tempNodeInfo->GetIp());
    SetNodes->SetPort(tempNodeInfo->GetPort());
    SetNodes->SetPeerWorkNums(tempNodeInfo->GetWorkNums());
    std::set<SerNodeInfo*> SetSerInfo;
    SetSerInfo.insert(SetNodes);
    m_AppendNodeTypeNodeInfo.insert(std::pair<std::string,
                                    std::set<SerNodeInfo*> >(
                                        sNodeType,SetSerInfo));

    LOG4_TRACE("add first new node into append list, nodetype: %s",
               "ip: %s,port: %u", sNodeType.c_str(), 
               SetNodes->GetIp().c_str(), SetNodes->GetPort());  
    return true;
  }

  bool bNodeExist = false;
  std::set<SerNodeInfo*>& SetSerInfo = it->second;
  std::set<SerNodeInfo*>::iterator itNode = SetSerInfo.begin();
  for (; itNode != SetSerInfo.end(); ++itNode) {
    if (!((*itNode)->GetIp() == tempNodeInfo->GetIp() && 
          (*itNode)->GetPort() == tempNodeInfo->GetPort())) {
      continue;
    }
    bNodeExist  = true; break;
  }

  if (bNodeExist == false) {
    SetNodes = new SerNodeInfo;
    SetNodes->SetIp(tempNodeInfo->GetIp());
    SetNodes->SetPort(tempNodeInfo->GetPort());
    SetNodes->SetPeerWorkNums(tempNodeInfo->GetWorkNums());
    SetSerInfo.insert(SetNodes);
    LOG4_TRACE("append new node into append list, nodetype: %s",
               "ip: %s,port: %u", sNodeType.c_str(), 
               SetNodes->GetIp().c_str(), SetNodes->GetPort());  
  }
  return true;
}

bool OssWorker::UpdateNodeRouteTable(const std::string& sNodeType,
                                     const SerNodeInfo *tempNodeInfo) {
  bool bRet = false;
  if (tempNodeInfo == NULL || sNodeType.empty()) {
    return bRet;
  }

  int iChildProcNum = tempNodeInfo->GetWorkNums();
  std::string sIp = tempNodeInfo->GetIp();
  int iPort = tempNodeInfo->GetPort();

  std::map<std::string,std::set<SerNodeInfo*> >::iterator it;

  it = m_NodeType_NodeInfo.find(sNodeType);
  if (it == m_NodeType_NodeInfo.end()) {
    if (CheckNodeInAutoDelSet(sNodeType, sIp, iPort) == 2) {
      LOG4_INFO("node type: %s , ip port exist in auto del node list", sNodeType.c_str());
      return false;
    }

    SerNodeInfo *SetNodes = new SerNodeInfo;
    SetNodes->SetIp(sIp);
    SetNodes->SetPort(iPort);
    SetNodes->SetPeerWorkNums(iChildProcNum);

    std::set<SerNodeInfo*> SetSerInfo;
    SetSerInfo.insert(SetNodes);
    m_NodeType_NodeInfo.insert(std::pair<std::string,std::set<SerNodeInfo*> >(sNodeType,SetSerInfo) );
    
    LOG4_INFO("update new node type: %s to node_type_set", sNodeType.c_str());
    return true;
  }

  bool bNodeExist = false;
  std::set<SerNodeInfo*>& setNodeInfo = it->second; //history node set
  std::set<SerNodeInfo*>::iterator itNodeInfo = setNodeInfo.begin();
  for ( ; itNodeInfo != setNodeInfo.end(); itNodeInfo++) {
    if (!((*itNodeInfo)->strIp == sIp && (*itNodeInfo)->uiPort == iPort)) {
      continue;
    } else if ((*itNodeInfo)->uiWorkNums <= 0) {
      LOG4_INFO("this is produced in  CmdToldWorker, not active sendtonext ");
      continue;
    } 
    bNodeExist = true;
    break;
  }

  if (bNodeExist == false) { //new node not exist history node set.
    //check new node exist in auto del node set, if exist, not to add
    //history
    if (CheckNodeInAutoDelSet(sNodeType, sIp, iPort) == 2) {
      LOG4_INFO("node type: %s, ip: %s, port: %d exist auto del node set",
                sNodeType.c_str(), sIp.c_str(), iPort);
      return false;;
    }

    SerNodeInfo* newNodeInfo = new SerNodeInfo;
    newNodeInfo->SetIp(sIp);
    newNodeInfo->SetPort(iPort);
    newNodeInfo->SetPeerWorkNums(iChildProcNum);
    setNodeInfo.insert(newNodeInfo); 

    LOG4_INFO("update new node type: %s, ip: %s, port: %u  to node set, ",
              sNodeType.c_str(), sIp.c_str(), iPort);
    return true;
  }

  LOG4_INFO("has exist node,ip: %s,port: %u, orig work num:%d, new conf work num: %d",
            sIp.c_str(),iPort,(*itNodeInfo)->uiWorkNums, iChildProcNum); 
  return false;

}

void OssWorker::FiltNodeExistANotExistB(std::map<std::string, std::set<SerNodeInfo*> >& nodeSetA,
                                        const std::map<std::string, std::set<SerNodeInfo*> >& nodeSetB) {

  std::map<std::string,std::set<SerNodeInfo*> >::iterator preIt;
  std::map<std::string,std::set<SerNodeInfo*> >::const_iterator itFind;
  
  for (preIt = nodeSetA.begin(); preIt != nodeSetA.end(); ) {
    const std::string& sPreNodeType = preIt->first;
    itFind = nodeSetB.find(sPreNodeType); // 在新的节点配置中查找 历史节点

    if (itFind == nodeSetB.end()) {
      m_ToDel_NodeTypeNodeInfo[sPreNodeType] = preIt->second; //没找到，在历史配置中删除该节点。
      nodeSetA.erase(preIt++);
      continue;
    }
    
    std::set<SerNodeInfo*>::iterator itPreNodeInfo;
    for (itPreNodeInfo = preIt->second.begin(); itPreNodeInfo != preIt->second.end(); ) {
      if ((*itPreNodeInfo)->uiWorkNums == 0) {
        LOG4_INFO("this work node vaild, work num is 0");
        ++itPreNodeInfo;
        continue;
      }

      bool bEq = false;
      std::set<SerNodeInfo*>::const_iterator itPre2NewCmp =itFind->second.begin();
      for ( ; itPre2NewCmp != itFind->second.end(); ++itPre2NewCmp) {
        if ((*itPre2NewCmp)->strIp == (*itPreNodeInfo)->strIp 
            && (*itPre2NewCmp)->uiPort == (*itPreNodeInfo)->uiPort) {
          bEq = true;
          break;
        }
      }

      if (false == bEq) {
        LOG4_INFO("pre node not exist in new node cnf, ip: %s, port: %u",
                  (*itPreNodeInfo)->strIp.c_str(), (*itPreNodeInfo)->uiPort);
        m_ToDel_NodeTypeNodeInfo[sPreNodeType].insert(*itPreNodeInfo); 
        preIt->second.erase(itPreNodeInfo++);
        continue;
      }
      ++itPreNodeInfo;
    }
    ++preIt;
  }
}

void OssWorker::LoadDownStream(loss::CJsonObject& oDownStreamCnf) {
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string sNodeType;
    int iChildProcNum;
    std::string sIp;
    int iPort;
    std::map<std::string,std::set<SerNodeInfo*> > mNewCnfNodeTypeInfo;
    std::map<std::string,std::set<SerNodeInfo*> >::iterator itFind;
    std::map<std::string,std::set<SerNodeInfo*> >::iterator it, preIt;

    for (int i = 0; i < oDownStreamCnf.GetArraySize(); ++i) {
      oDownStreamCnf[i].Get("node_type", sNodeType);
      
      if (oDownStreamCnf[i]("child_proc_num").empty()) {
        iChildProcNum = PROC_NUM_WORKER_DEF;
        LOG4_INFO("not set child proc num in  conf,use def num %d",
                  PROC_NUM_WORKER_DEF);
      } else {
        iChildProcNum = atoi(oDownStreamCnf[i]("child_proc_num").c_str());
        LOG4_TRACE("child proc num: %s from cnf",oDownStreamCnf[i]("child_proc_num").c_str());
      }
      oDownStreamCnf[i].Get("ip", sIp);
      oDownStreamCnf[i].Get("port", iPort);

      SerNodeInfo *tempNodeInfo = new SerNodeInfo;
      tempNodeInfo->SetIp(sIp);
      tempNodeInfo->SetPort(iPort);
      tempNodeInfo->SetPeerWorkNums(iChildProcNum);
      mNewCnfNodeTypeInfo[sNodeType].insert(tempNodeInfo);
      
      //check it in to del table;
      if (CheckInToDelNodeList(sNodeType, sIp, iPort) == 2) {
        LOG4_INFO("node: %s, ip: %s, port: %u exist in to del list",
                  sNodeType.c_str(), sIp.c_str(), iPort);
        AppendToAddNodeInList(sNodeType, tempNodeInfo);
        continue;
      }
      
      if (false == UpdateNodeRouteTable(sNodeType, tempNodeInfo)) {
        LOG4_TRACE("update node to route table failed,node type: %s,"
                   "ip: %s, port: %u", sNodeType.c_str(),
                   tempNodeInfo->GetIp().c_str(),
                   tempNodeInfo->GetPort());
        continue;
      }
#if 0
      //历史节点中没有该"节点类型",没有就增加。
      it = m_NodeType_NodeInfo.find(sNodeType);
      if (it == m_NodeType_NodeInfo.end()) {
        //check node type in auto del set; if exist, not to add 
        if (CheckNodeInAutoDelSet(sNodeType, sIp, iPort) == 2) {
          LOG4_INFO("node type: %s , ip port exist in auto del node list", sNodeType.c_str());
          continue;
        } 

        SerNodeInfo *SetNodes = new SerNodeInfo;
        SetNodes->SetIp(sIp);
        SetNodes->SetPort(iPort);
        SetNodes->SetPeerWorkNums(iChildProcNum);

        std::set<SerNodeInfo*> SetSerInfo;
        SetSerInfo.insert(SetNodes);
        m_NodeType_NodeInfo.insert(std::pair<std::string,std::set<SerNodeInfo*> >(sNodeType,SetSerInfo) );
        LOG4_INFO("insert new node type: %s to node_type_set", sNodeType.c_str());
        continue;
      }

      //确定新的配置节点是否存在历史节点类型的节点列表中，存在不做任何处理，不
      //在则添加到到内存类型节点列表里。
      bool bNodeExist = false;
      std::set<SerNodeInfo*>& setNodeInfo = it->second; //history node set
      std::set<SerNodeInfo*>::iterator itNodeInfo = setNodeInfo.begin();
      for ( ; itNodeInfo != setNodeInfo.end(); itNodeInfo++) {
        if (!((*itNodeInfo)->strIp == sIp && (*itNodeInfo)->uiPort == iPort)) {
            continue;
        } else if ((*itNodeInfo)->uiWorkNums <= 0) {
            LOG4_INFO("this is produced in  CmdToldWorker, not active sendtonext ");
            continue;
        } 
        bNodeExist = true;
        break;
      }
      
      //在节点类型存在下，该Ip:port:* 不存在于历史类型的节点列表中
      if (bNodeExist == false) { //new node not exist history node set.
         //check new node exist in auto del node set, if exist, not to add
         //history
         if (CheckNodeInAutoDelSet(sNodeType, sIp, iPort) == 2) {
           LOG4_INFO("node type: %s, ip: %s, port: %d exist auto del node set",
                     sNodeType.c_str(), sIp.c_str(), iPort);
           continue;
         }

         SerNodeInfo* newNodeInfo = new SerNodeInfo;
         newNodeInfo->SetIp(sIp);
         newNodeInfo->SetPort(iPort);
         newNodeInfo->SetPeerWorkNums(iChildProcNum);
         setNodeInfo.insert(newNodeInfo); 
      } else {
         LOG4_INFO("exist node,ip: %s,port: %u, orig work num:%d, new conf work num: %d",
                   sIp.c_str(),iPort,(*itNodeInfo)->uiWorkNums, iChildProcNum); 
      }
#endif

    }
    //上面完成 把新配置节点向历史节点列表中插入
    // ||
    // ||
    // ||
    //查找节点历史配置中有没有出现在节点新配置中的节点。如果历史节点不出现在本次更
    //新的节点中，那么需要把该历史节点从内存配置删除。比如配置的服务下架的场景。
    FiltNodeExistANotExistB(m_NodeType_NodeInfo, mNewCnfNodeTypeInfo);
    FiltNodeExistANotExistB(m_AutoDel_NodeTypeNodeInfo, mNewCnfNodeTypeInfo); //希望手动去触发删除自动被删除的节点列表
    // 上面完成 过滤历史节点（这些节点存在历史列表中，但不存在新的节点配置中），
    // 最后就是删除这些节点。
    
    for (itFind = mNewCnfNodeTypeInfo.begin(); itFind != mNewCnfNodeTypeInfo.end(); ++itFind) {
      std::set<SerNodeInfo*>& setServNodeInfo = itFind->second;
      for (std::set<SerNodeInfo*>::iterator itservNode = setServNodeInfo.begin();
           itservNode != setServNodeInfo.end(); ++itservNode) {
        if (*itservNode) {
          delete *itservNode;
        }
      }
    }
    mNewCnfNodeTypeInfo.clear();
    ///< 设置一个定时器并触发处理删除掉的节点连接资源,目前设置配置文件定时加载时
    //间的一半。
    //m_ToDel_NodeTypeNodeInfo
    if (m_ToDel_NodeTypeNodeInfo.empty() == false) {
      #define  delayDelNodeTm   (10)
      RegisterDelInValidPeerNodeProc(delayDelNodeTm);
    }
}

void OssWorker::RegisterDelInValidPeerNodeProc(int iDelayTm) {
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* tmout_watcher = new ev_timer();
    if (NULL == tmout_watcher) {
        LOG4_ERROR("new tmout watcher err");
        return ;
    }

    ev_timer_init(tmout_watcher, DelInvalidNodeCB, iDelayTm, 0.);
    tmout_watcher->data = (void*)this;
    ev_timer_start(m_loop, tmout_watcher);
    
    LOG4_INFO("after %ds, then del deleted peer work node", iDelayTm);
}

void OssWorker::DelInvalidNodeCB(struct ev_loop* loop, struct ev_timer* watcher, int revents) {
    if (watcher->data != NULL) {
      OssWorker* pWorker = (OssWorker*)(watcher->data);
      pWorker->DelInvalidNode();
    }

    //LOG4_INFO("timer for del invalid node tmout,now do del proc");
    ev_timer_stop(loop, watcher); 
    delete watcher;
}

void OssWorker::DelInvalidNode() {
  std::map<std::string,std::set<SerNodeInfo*> >::iterator it;

  for (it = m_ToDel_NodeTypeNodeInfo.begin(); it != m_ToDel_NodeTypeNodeInfo.end(); ++it) {
    const std::string& sNodeType = it->first;
    std::set<SerNodeInfo*>& setNodeInfo = it->second;

    for (std::set<SerNodeInfo*>::iterator itNode = setNodeInfo.begin(); itNode != setNodeInfo.end();++itNode) {
      SerNodeInfo* pNodeInfo = *itNode;

      if (pNodeInfo == NULL) {
        continue;
      }
      if (pNodeInfo->GetWorkNums() <=0 ) {
        delete pNodeInfo;
        continue;
      }

      std::string sIp = pNodeInfo->GetIp();
      int iPort = pNodeInfo->GetPort();

      for (int iWorkIndex = 0; iWorkIndex < pNodeInfo->GetWorkNums(); ++iWorkIndex) {
        std::stringstream os;
        os << sIp << ":" << iPort << ":" << iWorkIndex;
        std::string sIdentity = os.str();

        std::map<std::string, tagMsgShell>::iterator itIdentityFd = m_mapMsgShell.find(sIdentity);
        if (itIdentityFd == m_mapMsgShell.end()) {
          continue;
        }

        tagMsgShell& fdShell = itIdentityFd->second;
        std::map<int, tagConnectionAttr*>::iterator itFdConn = m_mapFdAttr.find(fdShell.iFd);
        if (itFdConn != m_mapFdAttr.end()) {
          LOG4_INFO("del node, [type: %s, identity: %s], close connected fd: %d, conn seq: %lu",
                    sNodeType.c_str(), sIdentity.c_str(), fdShell.iFd, fdShell.ulSeq);
          DestroyConnect(itFdConn); //del conattr in func.
        }
      }
      delete pNodeInfo;
    }
    setNodeInfo.clear();
  }
  //一并删除集合。
  m_ToDel_NodeTypeNodeInfo.clear();

  //add append ipport to route table;
  std::map<std::string, std::set<SerNodeInfo*> >::iterator itAppend = m_AppendNodeTypeNodeInfo.begin();
  for( ; itAppend != m_AppendNodeTypeNodeInfo.end(); ) {
    std::set<SerNodeInfo*>& serNodeSet = itAppend->second;
    const std::string& sTmpNodeType = itAppend->first;

    std::set<SerNodeInfo*>::iterator itNode = serNodeSet.begin(); 
    for( ; itNode != serNodeSet.end(); ) {
      if (true == UpdateNodeRouteTable(sTmpNodeType, *itNode)) {
        SerNodeInfo* tmpNode = *itNode;
        LOG4_INFO("after timer del node, append  need add node succ. node type: %s, ip: %s, port: %d", 
                  sTmpNodeType.c_str(), tmpNode->GetIp().c_str(), tmpNode->GetPort());

        serNodeSet.erase(itNode++);
        delete tmpNode;
      } else {
        ++itNode;
      }
    }
    if (serNodeSet.empty()) {
      m_AppendNodeTypeNodeInfo.erase(itAppend++);
    } else {
      itAppend++;
    }
  }
}

void OssWorker::LoadModule(loss::CJsonObject& oModuleConf)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string strModulePath;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::map<std::string, tagModule*>::iterator module_iter;
    tagModule* pModule = NULL;
    LOG4_TRACE("oModuleConf.GetArraySize() = %d", oModuleConf.GetArraySize());
    for (int i = 0; i < oModuleConf.GetArraySize(); ++i)
    {
        oModuleConf[i].Get("load", bIsload);
        if (bIsload)
        {
            if (oModuleConf[i].Get("url_path", strModulePath) && oModuleConf[i].Get("version", iVersion))
            {
                LOG4_TRACE("url_path = %s", strModulePath.c_str());
                module_iter = m_mapModule.find(strModulePath);
                if (module_iter == m_mapModule.end())
                {
                    strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                    if (0 != access(strSoPath.c_str(), F_OK))
                    {
                        LOG4_WARN("%s not exist!", strSoPath.c_str());
                        continue;
                    }
                    pModule = LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                    if (pModule != NULL)
                    {
                        LOG4_INFO("succeed in loading %s with module path \"%s\".",
                                        strSoPath.c_str(), strModulePath.c_str());
                        m_mapModule.insert(std::pair<std::string, tagModule*>(strModulePath, pModule));
                    }
                }
                else
                {
                    if (iVersion != module_iter->second->iVersion)
                    {
                        strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
                            continue;
                        }
                        LOG4_TRACE("to del pModule addr: %p, to del pSoHandle addr: %p",
                                   module_iter->second->pModule, module_iter->second->pSoHandle); 
                        //
                        delete module_iter->second->pModule;module_iter->second->pModule = NULL;
                        dlclose(module_iter->second->pSoHandle); module_iter->second->pSoHandle = NULL;
                        
                        delete module_iter->second;
                        module_iter->second = NULL;
                        m_mapModule.erase(module_iter);
                        
                        pModule = LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetModule", __FILE__, __LINE__);
                        if (pModule != NULL)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            m_mapModule.insert(std::pair<std::string, tagModule*>(strModulePath, pModule));
                            LOG4_TRACE("new version, pModule addr: %p, pSoHandle addr: %p",
                                       pModule->pModule, pModule->pSoHandle);
                        }
                    }
                }
            }
        }
        else        // 卸载动态库
        {
            if (oModuleConf[i].Get("url_path", strModulePath))
            {
                strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                UnloadSoAndDeleteModule(strModulePath);
                LOG4_INFO("unload %s", strSoPath.c_str());
            }
        }
    }
}

tagModule* OssWorker::LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagModule* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        return(pSo);
    }
    CreateCmd* pCreateModule = (CreateCmd*)dlsym(pHandle, strSymbol.c_str());
    dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("dlsym error %s!" , dlsym_error);
        dlclose(pHandle);
        return(pSo);
    }
    Module* pModule = (Module*)pCreateModule();
    if (pModule != NULL)
    {
        pSo = new tagModule();
        if (pSo != NULL)
        {
            pSo->pSoHandle = pHandle;
            pSo->pModule = pModule;
            pSo->iVersion = iVersion;
            pSo->pModule->SetLogger(&m_oLogger);
            pSo->pModule->SetLabor(this);
            pSo->pModule->SetConfigPath(m_strWorkPath);
            pSo->pModule->SetModulePath(strModulePath);
            if (!pSo->pModule->Init())
            {
                LOG4_FATAL("Module %s %s init error", strModulePath.c_str(), strSoPath.c_str());
                delete pSo;
                pSo = NULL;
            }
        }
        else
        {
            LOG4_FATAL("new tagSo() error!");
            delete pModule;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void OssWorker::UnloadSoAndDeleteModule(const std::string& strModulePath)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<std::string, tagModule*>::iterator module_iter;
    module_iter = m_mapModule.find(strModulePath);
    if (module_iter != m_mapModule.end())
    {
        delete module_iter->second;
        module_iter->second = NULL;
        m_mapModule.erase(module_iter);
    }
}

bool OssWorker::AddPeriodicTaskEvent()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    ev_timer_init (timeout_watcher, PeriodicTaskCallback, NODE_BEAT, 0.);
    timeout_watcher->data = (void*)this;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool OssWorker::AddIoReadEvent(int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
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
            tagIoWatcherData* pData = new tagIoWatcherData();
            if (pData == NULL)
            {
                LOG4_ERROR("new tagIoWatcherData error!");
                delete io_watcher;
                return(false);
            }
            pData->iFd = iFd;
            pData->ulSeq = iter->second->ulSeq;
            pData->pWorker = this;
            ev_io_init (io_watcher, IoCallback, iFd, EV_READ);
            io_watcher->data = (void*)pData;
            iter->second->pIoWatcher = io_watcher;
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

bool OssWorker::AddIoWriteEvent(int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
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
            tagIoWatcherData* pData = new tagIoWatcherData();
            if (pData == NULL)
            {
                LOG4_ERROR("new tagIoWatcherData error!");
                delete io_watcher;
                return(false);
            }
            pData->iFd = iFd;
            pData->ulSeq = iter->second->ulSeq;
            pData->pWorker = this;
            ev_io_init (io_watcher, IoCallback, iFd, EV_WRITE);
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
    }
    return(true);
}

//bool OssWorker::AddIoErrorEvent(int iFd)
//{
//    LOG4_TRACE("%s()", __FUNCTION__);
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
//            tagIoWatcherData* pData = new tagIoWatcherData();
//            if (pData == NULL)
//            {
//                LOG4_ERROR("new tagIoWatcherData error!");
//                delete io_watcher;
//                return(false);
//            }
//            pData->iFd = iFd;
//            pData->ullSeq = iter->second->ullSeq;
//            pData->pWorker = this;
//            ev_io_init (io_watcher, IoCallback, iFd, EV_ERROR);
//            io_watcher->data = (void*)pData;
//            iter->second->pIoWatcher = io_watcher;
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

bool OssWorker::RemoveIoWriteEvent(int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
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
                ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & ~EV_WRITE);
                ev_io_start (m_loop, iter->second->pIoWatcher);
            }
        }
    }
    return(true);
}

bool OssWorker::AddIoReadEvent(std::map<int, tagConnectionAttr*>::iterator iter)
{
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
        tagIoWatcherData* pData = new tagIoWatcherData();
        if (pData == NULL)
        {
            LOG4_ERROR("new tagIoWatcherData error!");
            delete io_watcher;
            return(false);
        }
        pData->iFd = iter->first;
        pData->ulSeq = iter->second->ulSeq;
        pData->pWorker = this;
        ev_io_init (io_watcher, IoCallback, iter->first, EV_READ);
        io_watcher->data = (void*)pData;
        iter->second->pIoWatcher = io_watcher;
        ev_io_start (m_loop, io_watcher);
    }
    else
    {
        io_watcher = iter->second->pIoWatcher;
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool OssWorker::AddIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter)
{
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
        tagIoWatcherData* pData = new tagIoWatcherData();
        if (pData == NULL)
        {
            LOG4_ERROR("new tagIoWatcherData error!");
            delete io_watcher;
            return(false);
        }
        pData->iFd = iter->first;
        pData->ulSeq = iter->second->ulSeq;
        pData->pWorker = this;
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
    return(true);
}

bool OssWorker::RemoveIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    if (NULL != iter->second->pIoWatcher)
    {
        if (iter->second->pIoWatcher->events & EV_WRITE)
        {
            io_watcher = iter->second->pIoWatcher;
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & ~EV_WRITE);
            ev_io_start (m_loop, iter->second->pIoWatcher);
        }
    }
    return(true);
}

bool OssWorker::DelEvents(ev_io** io_watcher_addr)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (io_watcher_addr == NULL)
    {
        return(false);
    }
    if (*io_watcher_addr == NULL)
    {
        return(false);
    }
    ev_io_stop (m_loop, *io_watcher_addr);
    tagIoWatcherData* pData = (tagIoWatcherData*)(*io_watcher_addr)->data;
    delete pData;
    (*io_watcher_addr)->data = NULL;
    delete (*io_watcher_addr);
    (*io_watcher_addr) = NULL;
    io_watcher_addr = NULL;
    return(true);
}

bool OssWorker::AddIoTimeout(int iFd, uint32 ulSeq, tagConnectionAttr* pConnAttr, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pConnAttr->pTimeWatcher != NULL)
    {
        ev_timer_stop (m_loop, pConnAttr->pTimeWatcher);
        ev_timer_set (pConnAttr->pTimeWatcher, m_dIoTimeout, 0);
        ev_timer_start (m_loop, pConnAttr->pTimeWatcher);
        return(true);
    }
    else
    {
        ev_timer* timeout_watcher = new ev_timer();
        if (timeout_watcher == NULL)
        {
            LOG4_ERROR("new timeout_watcher error!");
            return(false);
        }
        tagIoWatcherData* pData = new tagIoWatcherData();
        if (pData == NULL)
        {
            LOG4_ERROR("new tagIoWatcherData error!");
            delete timeout_watcher;
            return(false);
        }
        ev_timer_init (timeout_watcher, IoTimeoutCallback, dTimeout, 0.);
        pData->iFd = iFd;
        pData->ulSeq = ulSeq;
        pData->pWorker = this;
        timeout_watcher->data = (void*)pData;
        if (pConnAttr != NULL)
        {
            pConnAttr->pTimeWatcher = timeout_watcher;
        }
        ev_timer_start (m_loop, timeout_watcher);
        return(true);
    }
}

tagConnectionAttr* OssWorker::CreateFdAttr(int iFd, uint32 ulSeq, loss::E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s(iFd %d, seq %lu, codec %d)", __FUNCTION__, iFd, ulSeq, eCodecType);
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
        pConnAttr->pRecvBuff = new loss::CBuffer();
        if (pConnAttr->pRecvBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pRecvBuff for fd %d error!", iFd);
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
        pConnAttr->pRemoteAddr = new char[16];
        if (pConnAttr->pRemoteAddr == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pRemoteAddr for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pClientData = new loss::CBuffer();
        if (pConnAttr->pClientData == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pClientData for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->dActiveTime = ev_now(m_loop);
        pConnAttr->ulSeq = ulSeq;
        pConnAttr->eCodecType = eCodecType;
        m_mapFdAttr.insert(std::pair<int, tagConnectionAttr*>(iFd, pConnAttr));
        return(pConnAttr);
    }
    else
    {
        LOG4_ERROR("fd %d is exist!", iFd);
        return(NULL);
    }
}

bool OssWorker::DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter, bool bMsgShellNotice)
{
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    LOG4_TRACE("%s(fd %d, MsgShellNotice = %d)", __FUNCTION__, iter->first, bMsgShellNotice);
    tagMsgShell stMsgShell;
    stMsgShell.iFd = iter->first;
    stMsgShell.ulSeq = iter->second->ulSeq;
    std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(iter->first);
    if (inner_iter != m_mapInnerFd.end())
    {
        LOG4_TRACE("%s() m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
        m_mapInnerFd.erase(inner_iter);
    }
    std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
    if (http_step_iter != m_mapHttpAttr.end())
    {
        m_mapHttpAttr.erase(http_step_iter);
    }
    DelMsgShell(iter->second->strIdentify);
    if (bMsgShellNotice)
    {
        MsgShellNotice(stMsgShell, iter->second->strIdentify, iter->second->pClientData);
    }
    DelEvents(&(iter->second->pIoWatcher));
    close(iter->first);
    delete iter->second;
    iter->second = NULL;
    m_mapFdAttr.erase(iter);
    return(true);
}

void OssWorker::MsgShellNotice(const tagMsgShell& stMsgShell, const std::string& strIdentify, loss::CBuffer* pClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int32, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapSo.find(CMD_REQ_DISCONNECT);
    if (cmd_iter != m_mapSo.end() && cmd_iter->second != NULL)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        oMsgBody.set_body(strIdentify);
        if (pClientData != NULL)
        {
            if (pClientData->ReadableBytes() > 0)
            {
                oMsgBody.set_additional(pClientData->GetRawReadBuffer(), pClientData->ReadableBytes());
            }
        }
        oMsgHead.set_cmd(CMD_REQ_DISCONNECT);
        oMsgHead.set_seq(GetSequence());
        oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
        cmd_iter->second->pCmd->AnyMessage(stMsgShell, oMsgHead, oMsgBody);
    }
}

bool OssWorker::Dispose(const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
                MsgHead& oOutMsgHead, MsgBody& oOutMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %lu, fd: %u, conn seq: %lu)",
                    __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq(),
                    stMsgShell.iFd, stMsgShell.ulSeq);
    OrdinaryResponse oRes;
    oOutMsgHead.Clear();
    oOutMsgBody.Clear();
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求
    {
        std::map<int32, Cmd*>::iterator cmd_iter;
        cmd_iter = m_mapCmd.find(gc_uiCmdBit & oInMsgHead.cmd()); //系统命令
        if (cmd_iter != m_mapCmd.end() && cmd_iter->second != NULL)
        {
            cmd_iter->second->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
        }
        else
        {
            std::map<int, tagSo*>::iterator cmd_so_iter;
            cmd_so_iter = m_mapSo.find(gc_uiCmdBit & oInMsgHead.cmd()); //业务命令
            if (cmd_so_iter != m_mapSo.end() && cmd_so_iter->second != NULL)
            {
                LOG4_DEBUG("begin to proc cmd: %u",oInMsgHead.cmd());
                
                cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
            }
            else        // 没有对应的cmd，是需由接入层转发的请求
            {
                if (CMD_REQ_SET_LOG_LEVEL == oInMsgHead.cmd())         //manager 发送过来的控制命令
                {
                    LogLevel oLogLevel;
                    oLogLevel.ParseFromString(oInMsgBody.body());
                    LOG4_INFO("log level set to %d", oLogLevel.log_level());
                    m_oLogger.setLogLevel(oLogLevel.log_level());
                }
                else if (CMD_REQ_RELOAD_SO == oInMsgHead.cmd())
                {
                    loss::CJsonObject oSoConfJson(oInMsgBody.body());
                    LOG4_INFO("new so set to:  %s ", oInMsgBody.body().c_str());
                    LoadSo(oSoConfJson);
                }
                else if (CMD_REQ_RELOAD_MODULE == oInMsgHead.cmd())
                {
                    loss::CJsonObject oModuleConfJson(oInMsgBody.body());
                    LOG4_INFO("new module set to: %s", oInMsgBody.body().c_str());
                    LoadModule(oModuleConfJson);
                }
                else if (CMD_REQ_RELOAD_DOWN_STREAM == oInMsgHead.cmd()) {
                    loss::CJsonObject oDownStreamCnfJs(oInMsgBody.body());
                    LOG4_INFO("new down stream set to: %s", oInMsgBody.body().c_str());
                    LoadDownStream(oDownStreamCnfJs);
                }
                else if (CMD_REQ_RELOAD_NODETYP_CMD == oInMsgHead.cmd()) {
                  loss::CJsonObject  oNodeTypCmdJs(oInMsgBody.body());
                  LOG4_INFO("new node type cmd cnf: %s", oInMsgBody.body().c_str());
                  SetNodeTypeCmdCnf(oNodeTypCmdJs);
                } else if (CMD_REQ_ALARM_SEDN_MON == oInMsgHead.cmd()) {
                  loss::CJsonObject jsAlarm;
                  std::string sBody = oInMsgBody.body();
                  if (jsAlarm.Parse(sBody)) {
                    LOG4_INFO("recv manager push alarm info: %s", sBody.c_str());
                    SendAlarmInfoFromManager(jsAlarm);
                  } else {
                    LOG4_ERROR("parse manager push alarm js info fail");
                  }
                }
                else
                {
#ifdef NODE_TYPE_ACCESS
                    std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stMsgShell.iFd);   //中转命令
                    if (inner_iter != m_mapInnerFd.end())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
                    {
                        cmd_so_iter = m_mapSo.find(CMD_REQ_TO_CLIENT); //收到内部服务发送给内部服务，目的是要中转到client.
                        if (cmd_so_iter != m_mapSo.end())
                        {
                            cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
                        }
                        else
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                            LOG4_ERROR(m_pErrBuff);
                            oRes.set_err_no(ERR_UNKNOWN_CMD);
                            oRes.set_err_msg(m_pErrBuff);
                            oOutMsgBody.set_body(oRes.SerializeAsString());
                            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                            oOutMsgHead.set_seq(oInMsgHead.seq());
                            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
                        }
                    }
                    else
                    {
                        cmd_so_iter = m_mapSo.find(CMD_REQ_FROM_CLIENT); //所有外部请求都经access中转，access 不会提供具体命令的处理。
                        if (cmd_so_iter != m_mapSo.end() && cmd_so_iter->second != NULL)
                        {
                            cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
                        }
                        else
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                            LOG4_ERROR(m_pErrBuff);
                            oRes.set_err_no(ERR_UNKNOWN_CMD);
                            oRes.set_err_msg(m_pErrBuff);
                            oOutMsgBody.set_body(oRes.SerializeAsString());
                            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                            oOutMsgHead.set_seq(oInMsgHead.seq());
                            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
                        }
                    }
#else
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                    LOG4_ERROR(m_pErrBuff);
                    oRes.set_err_no(ERR_UNKNOWN_CMD);
                    oRes.set_err_msg(m_pErrBuff);
                    oOutMsgBody.set_body(oRes.SerializeAsString());
                    oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                    oOutMsgHead.set_seq(oInMsgHead.seq());
                    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
#endif
                }
            }
        }
    }
    else    // 回调
    {
        std::map<uint32, Step*>::iterator step_iter;
        step_iter = m_mapCallbackStep.find(oInMsgHead.seq());
        if (step_iter != m_mapCallbackStep.end())   // 步骤回调
        {
            LOG4_TRACE("receive message, cmd = %d",
                            oInMsgHead.cmd());
            if (step_iter->second != NULL)
            {
//                if (oss::CMD_RSP_SYS_ERROR == oInMsgHead.cmd())   框架层不应截止系统错误，业务层会有逻辑，对于无逻辑的也要求加上对系统错误处理
//                {
//                    OrdinaryResponse oError;
//                    if (oError.ParseFromString(oInMsgBody.body()))
//                    {
//                        LOG4_ERROR("cmd[%u] seq[%u] callback error %d: %s!",
//                                        oInMsgHead.cmd(), oInMsgHead.seq(),
//                                        oError.err_no(), oError.err_msg().c_str());
//                        DeleteCallback(step_iter->second);
//                        return(false);
//                    }
//                }
                E_CMD_STATUS eResult;
                step_iter->second->SetActiveTime(ev_now(m_loop));
                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x, active_time %lf",
                                oInMsgHead.cmd(), oInMsgHead.seq(), step_iter->second->GetSequence(),
                                step_iter->second, step_iter->second->GetActiveTime());
                eResult = step_iter->second->Callback(stMsgShell, oInMsgHead, oInMsgBody);
//                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x",
//                                oInMsgHead.cmd(), oInMsgHead.seq(), step_iter->second->GetSequence(),
//                                step_iter->second);
//
//              //
                //更新s-s连接过程中逻辑失败的错误统计 
                if (eResult == STATUS_CMD_FAULT) {
                  std::map<int, tagConnectionAttr*>::iterator conn_iter;
                  conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
                  if (conn_iter != m_mapFdAttr.end()) {
                    if (conn_iter->second->ucConnectStatus  < CMD_RSP_TELL_WORKER) {
                      LOG4_ERROR("err happen s-s connecting");
                      UpdateFailReqStat(stMsgShell);
                    }
                  }
                }

                //对每个step清除连接的句柄信息。
                DelStepFdRelation(step_iter->second);
                if (eResult != STATUS_CMD_RUNNING)
                {
                    DeleteCallback(step_iter->second);
                }
            }
        }
        else
        {
            snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %lu had been timeout!", oInMsgHead.seq());
            LOG4_ERROR(m_pErrBuff);
//            oRes.set_err_no(ERR_NO_CALLBACK);
//            oRes.set_err_msg(m_pErrBuff);
//            oOutMsgBody.set_body(step_iter->secondoRes.SerializeAsString());
//            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
//            oOutMsgHead.set_seq(oInMsgHead.seq());
//            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        }
    }
    return(true);
}

bool OssWorker::Dispose(const tagMsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg, HttpMsg& oOutHttpMsg)
{
    LOG4_DEBUG("%s() oInHttpMsg.type() = %d, oInHttpMsg.path() = %s",
                    __FUNCTION__, oInHttpMsg.type(), oInHttpMsg.path().c_str());
    oOutHttpMsg.Clear();
    if (HTTP_REQUEST == oInHttpMsg.type())    // 新请求
    {
        std::map<std::string, tagModule*>::iterator module_iter;
        module_iter = m_mapModule.find(oInHttpMsg.path());
        if (module_iter == m_mapModule.end())
        {
            snprintf(m_pErrBuff, gc_iErrBuffLen, "no module to dispose %s!", oInHttpMsg.path().c_str());
            LOG4_ERROR(m_pErrBuff);
            oOutHttpMsg.set_type(HTTP_RESPONSE);
            oOutHttpMsg.set_status_code(404);
            oOutHttpMsg.set_http_major(oInHttpMsg.http_major());
            oOutHttpMsg.set_http_minor(oInHttpMsg.http_minor());
        }
        else
        {
            LOG4_TRACE("get pModule addr(CreateCmd()): %p,pSoHandle addr(dlopen): %p", 
                        module_iter->second->pModule, module_iter->second->pSoHandle);
            module_iter->second->pModule->AnyMessage(stMsgShell, oInHttpMsg);
            //--------------------------------------------------------------------------------------
        //                        HttpMsg oHttpMsg;
        //                        oHttpMsg.set_type(HTTP_RESPONSE);
        //                        oHttpMsg.set_http_major(1);
        //                        oHttpMsg.set_http_minor(1);
        //                        oHttpMsg.set_status_code(200);
        //                        HttpMsg::Header* pHeader = oHttpMsg.add_headers();
        //                        pHeader->set_header_name("Content-Type");
        //                        pHeader->set_header_value("application/json; charset=UTF-8");
        //                        oHttpMsg.set_body("{\"error\":\"10002: no handler to dispose cmd 1005!\"}");
        //                        oOutMsgBody.set_body(oHttpMsg.SerializeAsString());
            //--------------------------------------------------------------------------------------
        }
    }
    else
    {
        std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
        if (http_step_iter == m_mapHttpAttr.end())
        {
            LOG4_ERROR("no callback for http response from %s!", oInHttpMsg.url().c_str());
        }
        else
        {
            if (http_step_iter->second.begin() != http_step_iter->second.end())
            {
                std::map<uint32, Step*>::iterator step_iter;
                step_iter = m_mapCallbackStep.find(*http_step_iter->second.begin());
                if (step_iter != m_mapCallbackStep.end() && step_iter->second != NULL)   // 步骤回调
                {
                    E_CMD_STATUS eResult;
                    step_iter->second->SetActiveTime(ev_now(m_loop));
                    eResult = ((HttpStep*)step_iter->second)->Callback(stMsgShell, oInHttpMsg);
                    if (eResult != STATUS_CMD_RUNNING)
                    {
                        DeleteCallback(step_iter->second);
                    }
                }
                else
                {
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %lu had been timeout!",
                                    *http_step_iter->second.begin());
                    LOG4_ERROR(m_pErrBuff);
                }
                http_step_iter->second.erase(http_step_iter->second.begin());
            }
            else
            {
                LOG4_ERROR("no callback for http response from %s!", oInHttpMsg.url().c_str());
            }
        }
    }
    return(true);
}

//
bool OssWorker::QueryNodeTypeByCmd(std::string& sNodeType,const int iCmd) {
  std::map<int, std::string>::iterator it = m_mpCmdNodeType.find(iCmd);
  if (it == m_mpCmdNodeType.end()) {
    LOG4_ERROR("not find any node type for cmd: %u", iCmd);
    return false;
  }

  sNodeType = it->second;
  return true;
}

//json format is: {"****": [1,2,3,4,5,6], "**":[1,2,3,4,6]}
//
bool OssWorker::SetNodeTypeCmdCnf(const loss::CJsonObject& oJsonConf) {
  m_mpCmdNodeType.clear();
  m_mpNodeTypeCmd.clear();
  return ParseNodeTypeCmdConf(oJsonConf);
}

/////
bool OssWorker::SendBusiAlarmReport(const loss::CJsonObject& jsReportData) {
  std::string sData = jsReportData.ToString();
  LOG4_TRACE("alam report data: %s", sData.c_str());
  
  Step* pSendAlarm = new StepSendAlarm(jsReportData, CMD_REQ_ALARM_REPORT_CMD);
  if (pSendAlarm == NULL) {
    return false;
  }

  if (!RegisterCallback(pSendAlarm)) {
    LOG4_ERROR("register sendalarmstep fail");
    delete pSendAlarm;
  }

  oss::E_CMD_STATUS eStatus = pSendAlarm->Emit(ERR_OK);
  if (STATUS_CMD_RUNNING != eStatus) {
    DeleteCallback(pSendAlarm);
  } 
  return false;
}

bool OssWorker::SendAlarmInfoFromManager(const loss::CJsonObject& jsAlarm) {
  if (jsAlarm.IsEmpty()) {
    LOG4_ERROR("recv alarm from manager is empty json");
    return false;
  }
  std::string sNodeType;
  if (jsAlarm.Get("node_type",sNodeType) == false) {
    LOG4_ERROR("manager sent alarm info not include node_type");
    return false;
  }
  return SendBusiAlarmReport(jsAlarm);
}

bool OssWorker::RegisterCallback(CTimer* pTimer) {
  LOG4_TRACE("%s()", __FUNCTION__);
  if (pTimer == NULL) {
    return false;
  }

  if (pTimer->IsStarted()) {
    return true;
  }

  pTimer->SetLogger(&m_oLogger);
  std::pair<std::map<std::string, CTimer*>::iterator, bool> ret;

  std::map<std::string,CTimer*>::iterator it = m_mpTimers.find(pTimer->GetTimerId());
  if (it == m_mpTimers.end()) {
    ret = m_mpTimers.insert(std::pair<std::string, CTimer*>(pTimer->GetTimerId(), pTimer));
  } else {
    ret.second = false;
  }

  if (ret.second == false) {
    LOG4_INFO("timer: %s has register into framework", pTimer->GetTimerId().c_str());
    return true;
  }

  if (pTimer->GetTimerTm() != 0) {
    ev_timer* pTimeoutWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (pTimeoutWatcher == NULL)
    {
      return(false);
    }

    ev_timer_init (pTimeoutWatcher, TimerTmOutCallback, pTimer->GetTimerTm(), 0.0);
    pTimer->SetTimeoutWatcher(pTimeoutWatcher);
    pTimeoutWatcher->data = (void*)pTimer;
    ev_timer_start (m_loop, pTimeoutWatcher);
    LOG4_TRACE("start timer succ, timer_id: %s", pTimer->GetTimerId().c_str());
  }
  return true;
}



/////////////////
} /* namespace oss */
