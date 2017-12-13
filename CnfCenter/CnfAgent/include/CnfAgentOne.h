#ifndef  _ONE_CNF_AGENT_H_
#define  _ONE_CNF_AGENT_H_

#include "CnfAgentBase.h"

#define HOST_CNF_CHANNEL   "host_conf"
#define SRV_NAME_CHANNEL   "srv_name"

#define SRVNAME_CNFFILE_KEY  "down_stream"
#define SRVNAME_NODETYPE     "node_type"
#define SRVNAME_IP           "ip"
#define SRVNAME_PORT         "port"

#define HOSTCNF_IP          "ip"
#define HOSTCNF_PATH        "cnf_path"
#define HOSTCNF_DATA        "cnf_dat"
#define HOSTCNF_PORT        "port"

using namespace LIB_REDIS;
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


//define cnf center agent instance.
//just do work: add some sub response process instance.
class CnfAgentOne :public SubCnfAgent {
 public:
  CnfAgentOne();
  ~CnfAgentOne ();
  virtual bool AddSubProcMethod();
 public:
  bool GetSrvNameData(std::string& sSrvName);
  bool WriteNewSrvNameDatFile(const std::string& sSrvNameCnf);
};

//////////////
}


#endif
