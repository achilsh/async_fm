/**
 * @file: base_task.h
 * @brief: 定一个多进程定时任务基类
 * @author:  wusheng Hu
 * @version: 
 * @date: 2017-12-06
 */
#include <map>
#include <string>
#include <stdint.h>
#include "ev.h"
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "util/json/CJsonObject.hpp"
#include "unix_util/process_helper.h"
#include "unix_util/proctitle_helper.h"

using namespace loss;

namespace BASE_TASK {

#define TLOG4_FATAL(args...) LOG4CPLUS_FATAL_FMT(GetTLog(), ##args)
#define TLOG4_ERROR(args...) LOG4CPLUS_ERROR_FMT(GetTLog(), ##args)
#define TLOG4_WARN(args...)  LOG4CPLUS_WARN_FMT(GetTLog(), ##args)
#define TLOG4_INFO(args...)  LOG4CPLUS_INFO_FMT(GetTLog(), ##args)
#define TLOG4_DEBUG(args...) LOG4CPLUS_DEBUG_FMT(GetTLog(), ##args)
#define TLOG4_TRACE(args...) LOG4CPLUS_TRACE_FMT(GetTLog(), ##args)

class BaseTask {
  public:
    BaseTask ();
    virtual ~BaseTask();
    //child class need to implement
    virtual int TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWkId) = 0;
    virtual int HandleLoop();
    //client need to call interface.
    int32_t Run(int32_t argc, char *argv[]);
    log4cplus::Logger GetTLog() { return m_oLogger; }
  protected:
    void ChildHandleExit();
    int32_t CreateWorker();
    int32_t CreateEvent();
    int32_t RunWorker(loss::CJsonObject& cnfJson, uint32_t uiWkId);
    int32_t ChildSignalInit();
    
    bool ChildCoreExit(struct ev_signal* watcher);
    static void ChildCoreCallback(struct ev_loop* loop, 
                                  struct ev_signal* watcher, 
                                  int revents);
    //
    bool ParentExist(struct ev_signal* watcher);
    static void ExitParentCallBack(struct ev_loop* loop,
                                   struct ev_signal* watcher,
                                   int revents);
    static void SigTerm(int32_t signo);
  protected:
    int32_t Init(int32_t argc, char *argv[]);
    bool GetConf();
    bool RestartWorker(int iPid);
    uint32_t GetWorkerNums() { return m_uiWorkerNum; }

    static int32_t static_exit_;

    std::string m_strWorkPath;
    loss::CJsonObject m_oLastConf;
    loss::CJsonObject m_oCurrentConf;
    uint32 m_uiWorkerNum;
    int m_iLogLevel;
    log4cplus::Logger m_oLogger;
    std::map<pid_t, uint32_t> m_mpWorkPid;
    struct ev_loop* m_loop;
    std::string m_strConfFile;
};
//
}

