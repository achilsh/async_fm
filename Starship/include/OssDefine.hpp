/*******************************************************************************
* Project:  AsyncServer
* @file     OssDefine.hpp
* @brief 
* @author   
* @date:    2015年7月27日
* @note
* Modify history:
******************************************************************************/
#ifndef OSSDEFINE_HPP_
#define OSSDEFINE_HPP_


#ifndef NODE_BEAT
#define NODE_BEAT 1.0
#endif

#define LOG4_FATAL(args...) LOG4CPLUS_FATAL_FMT(GetLogger(), ##args)
#define LOG4_ERROR(args...) LOG4CPLUS_ERROR_FMT(GetLogger(), ##args)
#define LOG4_WARN(args...) LOG4CPLUS_WARN_FMT(GetLogger(), ##args)
#define LOG4_INFO(args...) LOG4CPLUS_INFO_FMT(GetLogger(), ##args)
#define LOG4_DEBUG(args...) LOG4CPLUS_DEBUG_FMT(GetLogger(), ##args)
#define LOG4_TRACE(args...) LOG4CPLUS_TRACE_FMT(GetLogger(), ##args)

#define PROC_NUM_WORKER_DEF   (6)

#define  SRV_NAME_VER_SHM_KEY  (0X20171216)
#define  SRV_NAME_VER_SHM_SZ   (100*256)
#define  SRV_NAME_VER_KEY      "cnf_agent:srv_name_ver"
#define  NODETYPE_CMD_VER_KEY  "cnf_agent:nodetype_cmd_ver"
#define  WHITE_LIST_VER_KEY    "cnf_agent:white_list_ver"

namespace oss
{

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;

/** @brief 心跳间隔时间（单位:秒） */
const int gc_iBeatInterval = 5;

/** @brief 每次epoll_wait能处理的最大事件数  */
const int gc_iMaxEpollEvents = 100;

/** @brief 最大缓冲区大小 */
const int gc_iMaxBuffLen = 65535;

/** @brief 错误信息缓冲区大小 */
const int gc_iErrBuffLen = 256;

const uint32 gc_uiThriftHeadSize = 4;
const uint32 gc_uiMsgHeadSize = 15;
const uint32 gc_uiClientMsgHeadSize = 14;

enum E_CMD_STATUS
{
    STATUS_CMD_START            = 0,    ///< 创建命令执行者，但未开始执行
    STATUS_CMD_RUNNING          = 1,    ///< 正在执行命令
    STATUS_CMD_COMPLETED        = 2,    ///< 命令执行完毕
    STATUS_CMD_OK               = 3,    ///< 单步OK，但命令最终状态仍需调用者判断
    STATUS_CMD_FAULT            = 4,    ///< 命令执行出错并且不必重试
};

/**
 * @brief 消息外壳
 * @note 当外部请求到达时，因Server所有操作均为异步操作，无法立刻对请求作出响应，在完成若干个
 * 异步调用之后再响应，响应时需要有请求通道信息tagMsgShell。接收请求时在原消息前面加上
 * tagMsgShell，响应消息从通过tagMsgShell里的信息原路返回给请求方。若通过tagMsgShell
 * 里的信息无法找到请求路径，则表明请求方已发生故障或已超时被清理，消息直接丢弃。
 */
struct tagMsgShell
{
    int32 iFd;          ///< 请求消息来源文件描述符
    uint32 ulSeq;      ///< 文件描述符创建时对应的序列号

    tagMsgShell() : iFd(0), ulSeq(0)
    {
    }

    tagMsgShell(const tagMsgShell& stMsgShell) : iFd(stMsgShell.iFd), ulSeq(stMsgShell.ulSeq)
    {
    }

    tagMsgShell& operator=(const tagMsgShell& stMsgShell)
    {
        iFd = stMsgShell.iFd;
        ulSeq = stMsgShell.ulSeq;
        return(*this);
    }
};

/**
 * @brief 心跳结构
 */
struct tagBeat
{
    int32 iId;
    time_t tBeatTime;
    tagBeat()
    {
        tBeatTime = 0;
        iId = 0;
    }

    bool operator < (const tagBeat& stBeat) const
    {
        if (tBeatTime < stBeat.tBeatTime)
        {
            return(true);
        }
        else if (tBeatTime == stBeat.tBeatTime
                && iId < stBeat.iId)
        {
            return(true);
        }
        return(false);
    }
};

/**
 * @brief 序列号结构
 */
struct tagSequence
{
    int32 iId;
    uint32 ulSeq;
};

#define LOG4_ALARM_REPORT(format, arg...) do {  \
    loss::CJsonObject jsRep; \
    jsRep.Add("node_type", GetLabor()->GetNodeType()); \
    jsRep.Add("ip", GetLabor()->GetHostForServer());  \
    jsRep.Add("worker_id", GetLabor()->GetWorkerIndex()); \
    jsRep.Add("call_interface",__FUNCTION__); \
    jsRep.Add("file_name", __FILE__ ); \
    jsRep.Add("line", __LINE__); \
    time_t t1; \
    time(&t1); \
    char buf[32] = {0}; \
    struct tm* t2 = localtime(&t1); \
    strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", t2); \
    jsRep.Add("time", buf); \
    std::string sDetailData; \
    sDetailData = AddDetailContent(format, ##arg); \
    jsRep.Add("detail", sDetailData); \
    SendBusiAlarmToManager(jsRep); \
} while(0)

//------------------//
//定一个全局id 默认值
#define SESSION_ID_DEF  "sess_def_id_im"

////
}

#endif /* OSSDEFINE_HPP_ */
