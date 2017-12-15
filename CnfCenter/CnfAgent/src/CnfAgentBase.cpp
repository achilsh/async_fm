#include "CnfAgentBase.h"
#include "LibComm/include/lib_string.h"

#include <fstream>      // std::ofstream
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

namespace SubCnfTask {
  
  ////////////////////////////////
  SubCnfAgent::SubCnfAgent (): m_pEvent (NULL), m_IsConnected(false),m_pSyncTimer (NULL) {
  }

  SubCnfAgent::~SubCnfAgent() {

  }

  int SubCnfAgent::TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWorkid) {
    if (false == GetChannelsForProcess(oJsonConf, uiWorkid)) {
      return -1;
    }
    m_uiWorkerId = uiWorkid;
    //
    loss::CJsonObject cJsonRedis;
    bool bRet = oJsonConf.Get("redis", cJsonRedis);
    if (bRet == false) {
      TLOG4_ERROR("get redis info from cnf failed");
      return -2;
    }

    m_srvNameStoreFileName.clear();
    bRet = oJsonConf.Get("srvname_filename", m_srvNameStoreFileName);
    if (bRet == false || m_srvNameStoreFileName.empty()) {
      m_srvNameStoreFileName.append(CNFSRVNAMESTORGEFILE);
    }
    std::string sRedisHost = "";
    int32_t iRedisPort = 6379;
    std::string sRedisPWD = "";
    int32_t sRedisDbId = 0;

    cJsonRedis.Get("host",sRedisHost); 
    cJsonRedis.Get("port",iRedisPort); 
    cJsonRedis.Get("pwd",sRedisPWD); 
    cJsonRedis.Get("db",sRedisDbId); 

    m_pEvent = event_base_new();
    m_asyncRedisCli.Init(sRedisHost, iRedisPort, sRedisPWD, sRedisDbId, m_pEvent);

    if (false == AddSubProcMethod()) {
      TLOG4_ERROR("add all subProc method failed");
      return -3;
    }

    if (0 != m_syncRedisCli.Init(sRedisHost, iRedisPort, sRedisPWD, sRedisDbId)) {
      TLOG4_ERROR("init sync redis failed");
      return -4;
    }

    double tm_timer= 1.0;
    oJsonConf.Get("timerchecktm", tm_timer);
    m_TimerTm.tv_sec = ::floor(tm_timer);
    long tvUsec = ::floor(tm_timer*1000000);
    tvUsec %= 1000000;
    m_TimerTm.tv_usec = tvUsec;
    return 0;
  }

  //
  bool SubCnfAgent::GetChannelsForProcess(loss::CJsonObject& oJsonConf, uint32_t uiWorkid) {
    m_sChsCnf.clear();
    loss::CJsonObject chListJson;
    bool bRet =  oJsonConf.Get("channel",chListJson);
    if (bRet == false) {
      TLOG4_ERROR("get channel from cnf failed");
      return false;
    }

    uint32_t uiTotalWorkNums = GetWorkerNums();
    if (uiTotalWorkNums <= 0) {
      TLOG4_ERROR("total worker num is zero");
      return false;
    }

    for (int iArr = 0 ; iArr < chListJson.GetArraySize(); iArr++) {
      if (iArr % uiTotalWorkNums == uiWorkid) {
        m_sChsCnf.insert(chListJson(iArr));
      }
      m_sCnfChAll.insert(chListJson(iArr));
    }
    return true;
  }

  int32_t SubCnfAgent::CheckChTypeForWork(const std::string& sChType) {
    std::set<std::string>::iterator it;
    it = m_sCnfChAll.find(sChType);
    if (it == m_sCnfChAll.end()) {
      TLOG4_INFO("not find ch: %s in cnf channel type", sChType.c_str());
      return -1;
    }

    it = m_sChsCnf.find(sChType);
    if (it == m_sChsCnf.end()) {
      TLOG4_TRACE("this worker not regiter channel [ %s ] ", sChType.c_str());
      return 1;
    }
    return 0;
  }
  //called by derive class in  AddSubProcMethod()
  bool SubCnfAgent::RegisteSubRetProc(const std::string& sChType, 
                                      SubRetProcBase* pRetProc) {
    int32_t iRet = CheckChTypeForWork(sChType); 
    if (iRet < 0) {
      return false;
    } else if (iRet >0) {
      return true;
    }

    std::map<std::string, SubRetProcBase*>::iterator itMp;
    itMp =  m_mpSubRetProc.find(sChType); 
    if (m_mpSubRetProc.end() != itMp) {
      delete itMp->second;
      itMp->second = pRetProc;
    } else {
      m_mpSubRetProc[sChType] = pRetProc;
      TLOG4_INFO("register channel type: [ %s ],sub ret resp proc :[%p],worker: [%d]",
                 sChType.c_str(), pRetProc, m_uiWorkerId);
    }
    return true;
  }

  //called by derive class in  AddSubProcMethod()
  bool SubCnfAgent::RegisteFullSyncFuc(const std::string& sChType,
                                       FullSyncHandler syncHandler) {
    int32_t iRet = CheckChTypeForWork(sChType); 
    if (iRet < 0) {
      return false;
    } else if (iRet >0) {
      return true;
    }

    std::map<std::string, FullSyncHandler>::iterator it;
    it = m_mpFullSyncFunc.find(sChType);
    if (it == m_mpFullSyncFunc.end()) {
      m_mpFullSyncFunc[sChType] = syncHandler;
      TLOG4_INFO("regiter sync handler for channel: %s",
                 sChType.c_str());
    } else {
      it->second = syncHandler;
    }
    return true;
  }


  int SubCnfAgent::HandleLoop() {
    static bool bProcStart = false;
    if (bProcStart == false) {
      TLOG4_INFO("worker [%u] first sync, need to syns full cnf from cnf_srv", m_uiWorkerId);
      if (FullSyncCnfFromCnfCenterSvr() == false) {
        TLOG4_ERROR("full sync cnf from cnf center svr failed when first sync");
      } else {
        bProcStart = true;
      }
    }

    if (m_pSyncTimer == NULL) {
      TLOG4_TRACE("SetSynTimer()");
      SetSynTimer();
    }
    static int ii = 0;
    RedisSubscriber subReq(m_asyncRedisCli);
    if (!subReq.Subscribe(m_mpSubRetProc)) {
      TLOG4_ERROR("sub redis failed, err: %s", subReq.Errmsg());
    }
    ++ii;

    //
    //usleep(10000);
    sleep(5);
    return 0;
  }

  void SubCnfAgent::SetSynTimer() {
    m_pSyncTimer = (struct event *)malloc(sizeof(struct event));

    evtimer_set(m_pSyncTimer, SubCnfAgent::StartSyncTimerWork, this);
    event_base_set(m_pEvent,m_pSyncTimer);
    evtimer_add(m_pSyncTimer,&m_TimerTm);

    TLOG4_TRACE("set timer tm dur: %lu, us: %lu", 
                m_TimerTm.tv_sec, m_TimerTm.tv_usec);
  }

  void SubCnfAgent::StartSyncTimerWork(int fd, short event, void * arg) {
    SubCnfAgent* pAgent = (SubCnfAgent*)arg;
    if (pAgent == NULL) {
      std::cout << "timer check cb arg is empty" << std::endl;
      return ;
    }

    evtimer_set(pAgent->m_pSyncTimer, SubCnfAgent::StartSyncTimerWork, arg);
    event_base_set(pAgent->m_pEvent, pAgent->m_pSyncTimer);
    evtimer_add(pAgent->m_pSyncTimer, &(pAgent->m_TimerTm));
    pAgent->CheckRedisConnected();

    LOG4CPLUS_TRACE_FMT(pAgent->GetTLog(), "sync timer handle is called again");
    //
  }

  bool SubCnfAgent::CheckRedisConnected() {
    TLOG4_TRACE("timer check async redis connect status");
    if (m_asyncRedisCli.ConnectStatus() == 0)  {
      if (FullSyncCnfFromCnfCenterSvr() == true) {
        TLOG4_INFO("full sync conf from conf server succ");
      } else {
        TLOG4_ERROR("full sync conf from conf server failed");
        return false;
      }
    }
    return true;
  }

  //
  bool SubCnfAgent::FullSyncCnfFromCnfCenterSvr() {
    size_t ii = 0;
    std::map<std::string, FullSyncHandler>::iterator it; 
    for (it = m_mpFullSyncFunc.begin(); it != m_mpFullSyncFunc.end(); ++it) {
      if (it->second() == false) {
        TLOG4_ERROR("sync failed for type: %s", it->first.c_str());
        continue;
      }
      ++ii;
    }
    if (ii < m_mpFullSyncFunc.size()) {
      TLOG4_INFO("sync info not complete");
      return false;
    }
    return true;
  }
  
  //redis format is hash, key =>  prefix:ip:port:servername
  //field: 1 ==> conf path file name, 2 ===> cnf content,json format
  // 服务配置文件存储格式为： (hash)
  // key: ip + port + servername, 
  // field: 1, value: cnf path value,include file name
  // field: 2, value: json formate conf file.
  bool SubCnfAgent::SyncHostCnfDetailInfo() {
    std::string sHostIp = GetEth0Ip();
    if (sHostIp.empty()) {
      TLOG4_ERROR("get host ip is empty");
      return true; 
    }
    TLOG4_TRACE("local ip: %s", sHostIp.c_str());
    
    std::string sKey;
    sKey.append(PREFIXHOSTKEY);
    sKey.append(":");
    sKey.append(sHostIp);
    sKey.append(":*");
    TLOG4_INFO("get related local host cnf key: %s", sKey.c_str());

    RedisKey rKOp(m_syncRedisCli);
    std::vector<std::string> vKeyRet;
    if (false == rKOp.Keys(sKey, vKeyRet)) {
      TLOG4_ERROR("cmd: KEYS %s  failed", sKey.c_str());
      return false;
    }
    if (vKeyRet.empty()) {
        TLOG4_TRACE("key: %s in redis nums is 0", sKey.c_str());
        return true;
    }
    
    std::vector<std::string> vField;
    vField.push_back(FieldNameFilePath);
    vField.push_back(FieldNameCnfContent);

    for (std::vector<std::string>::iterator it = vKeyRet.begin();
         it != vKeyRet.end(); ++it) {
      //*it format: prex:ip:port:srvname
      RedisHash rHashOp(m_syncRedisCli);
      std::map<std::string, std::string> mHVals;  
      if (false == rHashOp.hMGet(*it,vField, mHVals)) {
        TLOG4_ERROR("get hash value failed, key: %s", it->c_str());
        continue;
      }
      if (mHVals.find(FieldNameFilePath) == mHVals.end() \
          || mHVals.find(FieldNameCnfContent) == mHVals.end()) {
        TLOG4_ERROR("not find field filepath or conf file content  value");
        continue;
      }
      //TLOG4_TRACE("host conf file path: %s", mHVals[FieldNameFilePath].c_str());

      loss::CJsonObject jsonCnfConent(mHVals[FieldNameCnfContent]);
      std::string sCnfContent = jsonCnfConent.ToFormattedString();
      //if content is empty or file path empty, should reset file content?
      if (ReWriteHostCnfInfo(mHVals[FieldNameFilePath],
                             sCnfContent) == false) {
        TLOG4_ERROR("write host cnf file to local failed");
        continue;
      }
      TLOG4_TRACE("sync host conf, path: %s\nhost conf content: %s", 
                  mHVals[FieldNameFilePath].c_str(),sCnfContent.c_str());
      std::string sKeyData = *it;
      std::vector<std::string> vIpPortSrvName;
      LIB_COMM::LibString::str2vec(sKeyData, vIpPortSrvName, ":");
      if (vIpPortSrvName.size() < 4) {
        continue;
      }
      uint32_t uiPort = ::atoi(vIpPortSrvName[2].c_str());
      SendKillSignToListenProcess(sHostIp, uiPort);
    }
    return true;
  }

  bool SubCnfAgent::ReWriteHostCnfInfo(const std::string& sFilePath, 
                                       const std::string& sCnfContent) {
    if (sFilePath.empty() || sCnfContent.empty()) {
      TLOG4_ERROR("file path is empty or cnf content empty");
      return false;
    }

    std::size_t sIndex = sFilePath.find_last_of("/");
    std::string sPath = sFilePath.substr(0,sIndex); 
    int iRet = ::access(sPath.c_str(), F_OK);
    if (iRet < 0 && errno == ENOENT) {
      iRet = ::mkdir(sPath.c_str(), 0777);
      if (iRet < 0) {
        TLOG4_ERROR("creat dir: %s failed, err: %s",
                   sPath.c_str(), strerror(errno));
        return false;
      }
    }

    int iMode = S_IRWXU |S_IRWXG|S_IRWXO; //每次都全量覆盖写文件。
    int iFd = ::open(sFilePath.c_str(), O_CREAT|O_RDWR|O_TRUNC, iMode); 
    if (iFd <= 0) {
      TLOG4_ERROR("reopen file failed, err msg: %s, file: %s", strerror(errno), sFilePath.c_str());
      return false;
    }

    ssize_t iWRet = ::write(iFd,  sCnfContent.c_str(), sCnfContent.size());
    if (iWRet <= -1) {
      TLOG4_ERROR("write file failed, errmsg: %s, file: %s", strerror(errno), sFilePath.c_str());
      ::close(iFd);
      return false;
    }
    ::close(iFd);

    return true;
  }

  /*****************************************/
  //服务名配置存储格式为：string
  //key: 服务名
  //value: [{ip, port}, {"ip":"","port": 0} ] 
  bool SubCnfAgent::SyncAllCnfSerNameList() {
      std::string sKey;
      sKey.append(PREFIXSVRNMKEY);
      sKey.append(":*");
      TLOG4_TRACE("get srv name key:  %s", sKey.c_str());

      RedisKey rKOp(m_syncRedisCli);
      std::vector<std::string> vKeyRet;
      if (false == rKOp.Keys(sKey, vKeyRet)) {
        TLOG4_ERROR("cmd: KEYS %s  failed", sKey.c_str());
        return false;
      }
      if (vKeyRet.empty()) {
        TLOG4_TRACE("srv name cnf key: %s  is empty", sKey.c_str());
        ResetSrvNameFileContent(m_srvNameStoreFileName);
        //del all srv name list in srv name file
        return true;
      }

      std::map<std::string, std::string> mSrvNameListInfo;
      for (std::vector<std::string>::iterator it = vKeyRet.begin();
           it != vKeyRet.end(); ++it) {
        std::size_t sIndex = it->find_last_of(":");
        std::string sSrvName = it->substr(sIndex + 1);
        
        if (false == rKOp.Get(*it, mSrvNameListInfo[sSrvName])) {
          TLOG4_ERROR("call redis failed, key: %s", it->c_str());
          continue;
        }
        TLOG4_TRACE("srv name: %s \n\r srv name cnf: %s",sSrvName.c_str(),
                    mSrvNameListInfo[sSrvName].c_str());
      }
      
      //每次全量去拉取 覆盖文件写
      if (false == WriteSrvNameInfoInLocal(mSrvNameListInfo, m_srvNameStoreFileName)) {
        TLOG4_ERROR("write srv name info to local failed");
        return false;
      }

      return true;
  }

  //[{},{},{} ]
  bool SubCnfAgent::WriteSrvNameInfoInLocal(const std::map<std::string,std::string>& srvNameSet,
                                            const std::string& cnfFile) {
    //{"down_stream":[{"node_type":"TestLogic","ip":"192.168.1.100","port":36000}] }
    if (cnfFile.empty()) {
      TLOG4_INFO("store srv name info file path empty");
      return true;
    }
    if (srvNameSet.empty()) {
      return ResetSrvNameFileContent(cnfFile);
    }
    
    loss::CJsonObject downStreaJson;
    loss::CJsonObject jsonList; //all list
    std::map<std::string, std::string>::const_iterator it;
    for (it = srvNameSet.begin(); it != srvNameSet.end(); ++it) {
      //it->second format is: [{},{},{}]
      //it->first format is: srv_name 
      std::string sSrvName = it->first;
      loss::CJsonObject onejsonCnfList(it->second);
      
      if (onejsonCnfList.IsArray() == false) {
        TLOG4_ERROR("srv name: %s  host list json not array", sSrvName.c_str());
        continue;
      }

      for (int ii = 0; ii < onejsonCnfList.GetArraySize(); ++ii) {
        //item: {"ip":"","port":0}
        loss::CJsonObject& jsHostOne = onejsonCnfList[ii];
        //add node_type node in this {}
        jsHostOne.Add("node_type", sSrvName);
        jsonList.AddAsFirst(jsHostOne);
      }
    }

    downStreaJson.Add("down_stream", jsonList);
    std::string sToWriteSrvName = downStreaJson.ToFormattedString();
    TLOG4_TRACE("write srv name info: %s", sToWriteSrvName.c_str());
    
    std::string sSrvNameFile(m_srvNameStoreFileName);
    if (false == ReWriteHostCnfInfo(sSrvNameFile, sToWriteSrvName)) {
      TLOG4_ERROR("write srv name to local file: %s failed", sSrvNameFile.c_str());
      return false;
    }

    TLOG4_TRACE("sync all srv name, file path: %s\n srv name content: %s",
                sSrvNameFile.c_str(), sToWriteSrvName.c_str());
    return true;
  }

  std::string SubCnfAgent::GetEth0Ip() {
    struct ifreq stIf;
    ::strncpy(stIf.ifr_name, "eth0", sizeof(stIf.ifr_name));
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);

    if (-1 == sock)
      return "";

    if (::ioctl(sock, SIOCGIFADDR, &stIf) < 0) {
      ::close(sock);
      return "";
    }
    ::close(sock);

    struct sockaddr_in * adr = (sockaddr_in *)&stIf.ifr_addr;
    return ::inet_ntoa((struct in_addr)adr->sin_addr);
  }

  bool SubCnfAgent::ResetSrvNameFileContent(const std::string& sFilePath) {
    if (sFilePath.empty()) {
      return false;
    }

    std::size_t sIndex = sFilePath.find_last_of("/");
    std::string sPath = sFilePath.substr(0,sIndex); 
    int iRet = ::access(sPath.c_str(), F_OK);
    if (iRet < 0 && errno == ENOENT) {
      iRet = ::mkdir(sPath.c_str(), 0777);
      if (iRet < 0) {
        TLOG4_ERROR("creat dir: %s failed, err: %s",
                   sPath.c_str(), strerror(errno));
        return false;
      }
    }

    int iMode = S_IRWXU |S_IRWXG|S_IRWXO; //每次都全量覆盖写文件。
    int iFd = ::open(sFilePath.c_str(), O_CREAT|O_RDWR|O_TRUNC, iMode); 
    if (iFd <= 0) {
      TLOG4_ERROR("reopen file failed, err msg: %s, file: %s", strerror(errno), sFilePath.c_str());
      return false;
    }
    ::close(iFd);
    return true;
  }

  void SubCnfAgent::SendKillSignToListenProcess(
      const std::string& sIp, uint32_t uiPort) {
    if (sIp.empty() || uiPort <= 0) {
      return ;
    }

    std::stringstream ios;
    ios << "lsof -i:" << uiPort << "|grep LISTEN|awk '{print $2}'";
    std::string sPort2PidCmd = ios.str();
    TLOG4_TRACE("get pid cmd: %s", sPort2PidCmd.c_str());
    ios.str("");

    FILE *fOpen = NULL;
    char buff[1024];
    fOpen = ::popen(sPort2PidCmd.c_str(),"r");
    if (fOpen == NULL) {
      TLOG4_ERROR("cmd: %s run failed, err: %s",
                  sPort2PidCmd.c_str(), strerror(errno));
      return ;
    }

    if (NULL == fgets(buff,sizeof(buff), fOpen)) {
      TLOG4_ERROR("get cmd: %s ret failed, err: %s",
                  sPort2PidCmd.c_str(), strerror(errno));
      ::pclose(fOpen);
      return ;
    }
    ::pclose(fOpen); fOpen = NULL;

    int32_t uiListenPid = ::atoi(buff);
    TLOG4_TRACE("get listen port: %u, pid: %u",uiPort, uiListenPid);

    ios << "kill -s USR1 " << uiListenPid;
    std::string sKillUSR1Cmd = ios.str();
    TLOG4_TRACE("signal cmd: %s", sKillUSR1Cmd.c_str());
    if( -1 == ::system(sKillUSR1Cmd.c_str())) {
      TLOG4_ERROR("run cmd: %s failed", sKillUSR1Cmd.c_str());
      return ;
    }
    TLOG4_INFO("send proc signal cmd: %s succ", sKillUSR1Cmd.c_str());
  }
  ////////
}
