#ifndef _CNF_AGNET_BASE_H_
#define _CNF_AGNET_BASE_H_ 

#include "base_task.h"
#include "lib_redis.h"
#include <set>
#include <map>
#include <string>
#include <functional>

#define CNFSRVNAMESTORGEFILE   "/etc/srv_name/srvname.json"
#define PREFIXSVRNMKEY         "cnf_N1"
#define PREFIXHOSTKEY          "cnf_H1"
#define FieldNameFilePath      "1"
#define FieldNameCnfContent    "2"

using namespace LIB_REDIS;
using namespace BASE_TASK;

typedef std::function<bool () > FullSyncHandler;

namespace SubCnfTask {
  class SubCnfAgent : public BaseTask {
    public:
      SubCnfAgent();
      virtual ~SubCnfAgent();
      virtual bool AddSubProcMethod() = 0; //
      int TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWorkid);
      bool GetChannelsForProcess(loss::CJsonObject& oJsonConf, uint32_t uiWorkid);
      bool RegisteSubRetProc(const std::string& sChType, 
                             SubRetProcBase* pRetProc); 
      bool RegisteFullSyncFuc(const std::string& sChType,
                              FullSyncHandler syncHandler);

      int HandleLoop();
      void SetSynTimer(); 
      static void StartSyncTimerWork(int fd, short event, void * arg);
      bool CheckRedisConnected(); 
      bool FullSyncCnfFromCnfCenterSvr(); 
      bool SyncHostCnfDetailInfo();
      bool ReWriteHostCnfInfo(const std::string& sFilePath, 
                              const std::string& sCnfContent); 
      bool ResetSrvNameFileContent(const std::string& sFileName);
      bool SyncAllCnfSerNameList();
      bool WriteSrvNameInfoInLocal(const std::map<std::string,std::string>& srvNameSet,
                                   const std::string& cnfFile); 
      std::string GetEth0Ip();
      int32_t CheckChTypeForWork(const std::string& sChType);

      void SendKillSignToListenProcess(const std::string& sIp,
                                       uint32_t uiPort,
                                       const std::string& sSignel="USR1"); 
      bool GetHostCnfRedisKey(std::vector<std::string>& vRedisKey);
    protected:
      virtual void DoWorkAfterSync() {}
    protected:
      struct event_base*  m_pEvent;
      bool                m_IsConnected;
      AsyncClient         m_asyncRedisCli;
      Client              m_syncRedisCli;
      std::set<std::string>  m_sChsCnf;
      std::set<std::string>  m_sCnfChAll;
      std::map<std::string, SubRetProcBase*>  m_mpSubRetProc;
      std::map<std::string, FullSyncHandler>  m_mpFullSyncFunc;

      struct event*     m_pSyncTimer ;
      struct timeval    m_TimerTm;
      uint32_t          m_uiWorkerId;
      std::string       m_srvNameStoreFileName;
  };
}

#endif
