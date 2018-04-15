#include "base_task.h"
#include <iostream>

namespace  BASE_TASK {
  int32_t BaseTask::static_exit_ = 0;
  
  BaseTask::BaseTask():m_strWorkPath(""), m_uiWorkerNum(0), m_iLogLevel(0), m_loop(NULL) { }
  BaseTask::~BaseTask () { 
    if (m_loop != NULL)  {
      ev_loop_destroy(m_loop);
    }
  }

  int32_t BaseTask::Run(int32_t argc, char *argv[]) {
    int32_t iRet = Init(argc, argv);
    if (iRet < 0) {
      return -1;
    }

    CreateEvent();
    CreateWorker();

    TLOG4_TRACE("%s()", __FUNCTION__);

    ev_signal* sign_exit_watcher = new ev_signal();
    ev_signal_init (sign_exit_watcher , ExitParentCallBack, SIGTERM);
    sign_exit_watcher->data = (void*)this;
    ev_signal_start (m_loop, sign_exit_watcher);
    ev_run (m_loop, 0);
    return 0;
  }


  int32_t BaseTask::Init(int32_t argc, char *argv[]) {
    if (argc < 2) {
      std::cerr << "server Init failed \r\n" 
          << "usage: " << argv[0] << " conf.json " << std::endl;
      exit(-1);
    }

    ngx_init_setproctitle(argc, argv);
    if (std::string(argv[1]).empty()) {
      std::cerr << "conf file is empty" << std::endl;
      return -1;
    }
    m_strConfFile.assign(argv[1]);
    
    if (!GetConf()) {
      std::cerr << "GetConf() failed" << std::endl;
      return -1;
    }
    //
    ngx_setproctitle(m_oCurrentConf("server_name").c_str());
    daemonize(m_oCurrentConf("server_name").c_str(), 1, 0);
    //
    char sBuf[256] = {0};
    snprintf(sBuf,sizeof(sBuf),"/tmp/%s.lock", getproctitle());
    if (false == m_SingleprocessCheck.IsRun(sBuf)) {
        std::cerr << "start task fail, " << m_SingleprocessCheck.GetErrMsg() << std::endl;
        return -2;
    }

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
    m_oLogger = log4cplus::Logger::getInstance(szLogName);
    m_oLogger.addAppender(append);
    m_oLogger.setLogLevel(m_iLogLevel);
    TLOG4_INFO("%s begin, and work path %s", m_oCurrentConf("server_name").c_str(), m_strWorkPath.c_str());

    static_exit_ = 0;
    return 0;
  }

  bool BaseTask::GetConf() {
    char szFilePath[256] = {0};
    if (m_strWorkPath.length() == 0)
    {
      if (getcwd(szFilePath, sizeof(szFilePath)))
      {
        m_strWorkPath = szFilePath;
      }
      else
      {
        return(false);
      }
    }

    m_oLastConf = m_oCurrentConf;
    std::ifstream fin(m_strConfFile.c_str());
    if (fin.good())
    {
      std::stringstream ssContent;
      ssContent << fin.rdbuf();
      if (!m_oCurrentConf.Parse(ssContent.str()))
      {
        ssContent.str("");
        m_oCurrentConf = m_oLastConf;
        fin.close();
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
      if (m_oLastConf.ToString().length() == 0)
      {
        if (m_oCurrentConf("process_num").empty()) {
          m_uiWorkerNum = 2;
        } else {
          m_uiWorkerNum = strtoul(m_oCurrentConf("process_num").c_str(), NULL, 10);
        }
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
    return true;
  }


  int32_t BaseTask::CreateWorker() {
    for (uint32_t uiWorkId = 0; uiWorkId < m_uiWorkerNum; ++uiWorkId) {
      pid_t  pid = fork();
      if (pid == 0) {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
        RunWorker(m_oCurrentConf, uiWorkId);
        exit(0);
      } else {
        m_mpWorkPid.insert(std::pair<pid_t,uint32_t>(pid, uiWorkId));
      }
    }
    return 0;
  }

  int32_t BaseTask::CreateEvent() {
    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
    if (m_loop == NULL) {
      return -2;
    }

    ev_signal* signal_watcher = new ev_signal();
    ev_signal_init (signal_watcher, ChildCoreCallback, SIGCHLD);
    signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, signal_watcher);
    //
    return 0;
  }

  int32_t BaseTask::ChildSignalInit() {
    signal(SIGTERM, SigTerm);
    return 0;
  }

  void BaseTask::SigTerm(int32_t signo)
  {
    if (signo == SIGTERM)
    {
      static_exit_ = 1;
    }
  }

  void BaseTask::ChildHandleExit()
  {
    if (static_exit_)
    {
      sleep(1);
      TLOG4_INFO("child proc safe exit");
      exit(0);
    }
  }

  int BaseTask::HandleLoop()
  {
    return 0;
  }

  void BaseTask::ChildCoreCallback(struct ev_loop* loop, 
                                 struct ev_signal* watcher, 
                                 int revents) {
    if (watcher->data != NULL) {
      BaseTask* task =(BaseTask*)watcher->data;
      task->ChildCoreExit(watcher);
    }
  }

  bool BaseTask::ChildCoreExit(struct ev_signal* watcher) {
    pid_t   iPid = 0;  
    int     iStatus = 0;  
    int     iReturnCode = 0;
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

      TLOG4_WARN("error %d: process %d exit and sent signal %d with code %d!",
                iStatus, iPid, watcher->signum, iReturnCode);
      RestartWorker(iPid);
    }
    return(true);
  }

  bool BaseTask::RestartWorker(int iExitPid) {
    std::map<pid_t, uint32_t>::iterator it = m_mpWorkPid.find(iExitPid);
    uint32_t uiWorkId = 0;
    if (it != m_mpWorkPid.end()) {
      uiWorkId = it->second;
      m_mpWorkPid.erase(it);
    }

    int pid = 0;
    pid = fork();
    if (pid == 0) {
      ev_loop_destroy(m_loop);
      RunWorker(m_oCurrentConf, uiWorkId);
      exit(0);
    } else if (pid >0) {
      m_mpWorkPid.insert(std::pair<pid_t, uint32_t>(pid, uiWorkId));
      TLOG4_INFO("restart child proc, pid: %lu", pid);
      return true;
    } else {
      return false;
    }
  }
  //
  int32_t BaseTask::RunWorker(loss::CJsonObject& oJsonConf, uint32_t uiWkId) {
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", 
             oJsonConf("server_name").c_str(), uiWkId);
    ngx_setproctitle(szProcessName);
    
    ChildSignalInit();
    //init log
    int32 iMaxLogFileSize = 0;
    int32 iMaxLogFileNum = 0;
    int32 iLogLevel = 0;

    std::string strLogname = m_strWorkPath + std::string("/") + oJsonConf("log_path")
        + std::string("/") + getproctitle() + std::string(".log");
    std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
    oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
    oJsonConf.Get("max_log_file_num", iMaxLogFileNum);

    if (oJsonConf.Get("log_level", iLogLevel)) {
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
    
    //init busi conf  
    int iRet = TaskInit(oJsonConf, uiWkId);
    if ( iRet < 0) {
      TLOG4_ERROR("init busi fail, iRet: %d, child exit", iRet);
      exit(-1);
    }

    TLOG4_INFO("work [%u] going to run ", uiWkId);
    HandleLoop();
    return true;
  }

  void BaseTask::ExitParentCallBack(struct ev_loop* loop,
                                    struct ev_signal* watcher,
                                    int revents) {
    if (watcher->data != NULL) {
      BaseTask* task = (BaseTask*)watcher->data;
      task->ParentExist(watcher);
    }
  }

  bool BaseTask::ParentExist(struct ev_signal* watcher) {
    ev_loop_destroy(m_loop);
    exit(0);
  }
}
