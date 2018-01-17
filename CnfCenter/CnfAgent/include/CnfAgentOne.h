#ifndef  _ONE_CNF_AGENT_H_
#define  _ONE_CNF_AGENT_H_

#include "CnfAgentBase.h"
#include "lib_shm.h"
#include "OssDefine.hpp"

#define HOST_CNF_CHANNEL   "host_conf"
#define SRV_NAME_CHANNEL   "srv_name"
#define WHIT_LIST_CHANNEL  "white_list"

#define SRVNAME_CNFFILE_KEY  "down_stream"
#define SRVNAME_NODETYPE     "node_type"
#define SRVNAME_IP           "ip"
#define SRVNAME_PORT         "port"

#define HOSTCNF_IP          "ip"
#define HOSTCNF_PATH        "cnf_path"
#define HOSTCNF_DATA        "cnf_dat"
#define HOSTCNF_PORT        "port"

#define NODETYPECMD_TYPE   "nodetype_cmd"
#define NODETYPE_CMD_KEY_PREX     "nodetype_cmd"

using namespace LIB_REDIS;
using namespace LIB_SHM;
//define sub ret proc for host conf sub

namespace  SubCnfTask {

class CnfAgentOne;

//defie sub ret proc for host conf sub
class HostCnfRetProc: public SubRetProcBase {
  public:
   HostCnfRetProc(const std::string& sCh, SubCnfTask::CnfAgentOne *pAgent);
   virtual ~HostCnfRetProc();
   bool operator()(const std::string& sCh,
                   const std::string& sSubRet);
  private:
   template<class T>
   bool GetSubRetItem(const std::string& sItemName,
                           const loss::CJsonObject& jsSubRet,
                           T& tData) {
     if (sItemName.empty() || jsSubRet.IsEmpty()) {
       return false;
     }
     if (false == jsSubRet.Get(sItemName, tData)) {
       return false;
     }
     return true;
   }
   bool ProcSubNodeTypeCmdModify(const std::string& sNodeType,
                                 const loss::CJsonObject& jsSubRet);
   bool UpdateSubRetToLocalHostCnf(loss::CJsonObject& toJson,
                                   const loss::CJsonObject& fromJson,
                                   const std::string& sNodeType);
  private:
   SubCnfTask::CnfAgentOne *m_pCnfAgent;
};

//define sub ret proc for srv name sub
class SrvNameRetProc : public SubRetProcBase {
 public: 
  SrvNameRetProc (const std::string& sCh, SubCnfTask::CnfAgentOne *pAgent);
  virtual ~SrvNameRetProc();
  bool operator() (const std::string& sCh,
                   const std::string& sSubRet);
 private:
  bool UpdateSrvNameWithPubRet(std::string& sCnfSrvName, 
                               const std::string& spubSrvName);
  bool WriteSrvNameCnfToEmptyFile(std::string& sCnfSrvNameContent, 
                                  const Json::Value& jsPubIpPortList,
                                  const std::string& srvname);
 private:
  SubCnfTask::CnfAgentOne *m_pCnfAgent;
};
//define sub ret proc for white list sub
class WhiteListRetProc: public SubRetProcBase {
    public:
     WhiteListRetProc(const std::string& sCh,   
                      SubCnfTask::CnfAgentOne *pAgent);
     virtual ~WhiteListRetProc();
     bool operator()(const std::string& sCh, 
                     const std::string& sSubRet);

     bool MergeSubRetAndLocalFile(std::string& sWLCnf,
                                  const std::string& sWLSubRet);
     // 
    private:
     SubCnfTask::CnfAgentOne *m_pCnfAgent;
};


//define cnf center agent instance.
//just do work: add some sub response process instance.
class CnfAgentOne :public SubCnfAgent {
 public:
  CnfAgentOne();
  ~CnfAgentOne ();
  virtual bool AddSubProcMethod();
 public:
  bool WriteNewSrvNameDatFile(const std::string& sSrvNameCnf);
  void GetNewestSrvNameVer();
  bool GetSrvNameData(std::string& sSrvName);
  void SendUSR2SignelToLocalHostSrv();

  bool WriteSrvNameInfoInLocal(const std::map<std::string,std::string>& srvNameSet,
                               const std::string& cnfFile); 
  bool SyncAllCnfSerNameList();
  bool SyncHostCnfDetailInfo();
  bool SyncNodeTyeCmd();
  // 
  bool GetNodeTypeCmdFromLocalFile(loss::CJsonObject& jsCnf);
  bool WriteNodeTypeCmdToFile(const std::string & strjsCnf);

  void GetNewNodeTypeCmdVer();
  void SendUSR2ToLocalUpdateNodeTypeCmd();
  //comm interface
  bool WriteCnfFile(const std::string& fName, const std::string& jsConttent);
  bool GetShmVersion(const std::string& sKey);
  //
  bool GetWhiteListFromFile(std::string& sWhitListCnf);
  bool WriteWhiteListToFile(const std::string& sWhiteListCnf);
  void GetNewWhiteListVer();
  bool SyncAllWhiteListCnf();
  bool WriteWhiteListInLocal(const std::map<std::string, std::vector<std::string> >&mpWList, 
                             const std::string& sFileName);
 protected:
  void DoWorkAfterSync();
 private: 
  bool GetRedisKeys(const std::string& sKeyPrefix, std::vector<std::string>& vKeyRet);
  //
  LIB_SHM::LibShm m_srvNameVerShm;
  bool m_srvNameShmInit; 
};

//////////////
}


#endif

