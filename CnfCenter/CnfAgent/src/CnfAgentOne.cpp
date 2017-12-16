#include "CnfAgentOne.h"
#include "json/json.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

namespace SubCnfTask {

//-----------------------------------------//
CnfAgentOne::CnfAgentOne ():m_srvNameVerShm(LIB_SHM::OP_W), m_srvNameShmInit(false) {
  if (m_srvNameVerShm.Init(SRV_NAME_VER_SHM_KEY, SRV_NAME_VER_SHM_SZ) == true) {
    m_srvNameShmInit = true;
  }
}

CnfAgentOne::~CnfAgentOne () {
}
//==========================

bool CnfAgentOne::AddSubProcMethod(){ //call by base init...
  SubRetProcBase* retProc = new HostCnfRetProc(HOST_CNF_CHANNEL, this);
  retProc->SetLog(GetTLog());
  if (false == RegisteSubRetProc(HOST_CNF_CHANNEL,retProc )) {
    TLOG4_ERROR("register hsot_conf  sub ret proc failed");
    return false;
  }
  if (false == RegisteFullSyncFuc(HOST_CNF_CHANNEL,
                                  std::bind(
                                  &CnfAgentOne::SyncHostCnfDetailInfo, this))) {
    TLOG4_ERROR("register host_cnf sync handler failed");                                       
    return false;  
  }

  retProc = new SrvNameRetProc(SRV_NAME_CHANNEL, this);
  retProc->SetLog(GetTLog());
  if (false == RegisteSubRetProc(SRV_NAME_CHANNEL,retProc)) {
    TLOG4_ERROR("register srv_name sub ret proc failed");
    return false;
  }
  if (false == RegisteFullSyncFuc(SRV_NAME_CHANNEL,
                                  std::bind(
                                  &CnfAgentOne::SyncAllCnfSerNameList,this))) {
    TLOG4_ERROR("register srv_name sync handle failed");
    return false;
  }
  return true;
}

bool CnfAgentOne::WriteNewSrvNameDatFile(const std::string& sCnfContent) {
  std::string sFilePath = m_srvNameStoreFileName;
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

bool CnfAgentOne::GetSrvNameData(std::string& sSrvNameContent) {
  std::string sFilePath = m_srvNameStoreFileName;

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

  int iMode = S_IRWXU |S_IRWXG|S_IRWXO; 
  int iFd = ::open(sFilePath.c_str(), O_CREAT|O_RDWR, iMode);
  if (iFd <= 0) {
    TLOG4_ERROR("reopen file failed, err msg: %s, file: %s", strerror(errno), sFilePath.c_str());
    return false;
  }

  int iMaxSz = 102400;
  char buf[102400] = {0};
  ssize_t iRRet =  ::read(iFd, buf, iMaxSz); 
  if (iRRet <= -1) {
    TLOG4_ERROR("read file failed, errmsg: %s, file: %s", strerror(errno),sFilePath.c_str());
    ::close(iFd);
    return false;
  }
  sSrvNameContent.assign(buf, iRRet);
  ::close(iFd);
  
  TLOG4_TRACE("read srv cnf content: %s", sSrvNameContent.c_str());
  return true;
}

void CnfAgentOne::GetNewestSrvNameVer() {
  if (m_srvNameShmInit == false) {
    TLOG4_ERROR("srv name shm init failed");
    return ;
  }

  RedisKey  rKey(m_syncRedisCli);
  int64_t iLVerNo = 0;
  std::string  sKey = SRV_NAME_VER_KEY;

  bool bRet = rKey.Incr(sKey, iLVerNo);  
  if (bRet == false) {
    TLOG4_ERROR("incr version for srv name fail");
    return ;
  }
  TLOG4_TRACE("srv name ver no: %ld", iLVerNo);
  
  if (false == m_srvNameVerShm.SetValue(sKey,iLVerNo)) {
    TLOG4_ERROR("set srv name shm value fail, errmsg: %s",
               m_srvNameVerShm.GetErrMsg().c_str());
    return ;
  }
  //
}

void CnfAgentOne::DoWorkAfterSync() {
  this->GetNewestSrvNameVer();
}

//-------------------------------------//


HostCnfRetProc::HostCnfRetProc(const std::string& sC,
                               SubCnfTask::CnfAgentOne *pAgent)
  :SubRetProcBase(sC), m_pCnfAgent(pAgent) {
  //
}

HostCnfRetProc::~HostCnfRetProc() {
}
//------------------------------//
SrvNameRetProc::SrvNameRetProc (const std::string& sch,
                                SubCnfTask::CnfAgentOne *pAgent)
  :SubRetProcBase (sch), m_pCnfAgent(pAgent) {
}

SrvNameRetProc::~SrvNameRetProc() {
}
//
//
bool HostCnfRetProc::operator()(const std::string& sCh,
                            const std::string& sSubRet) {
  //TODO: parse json and write host conf json file to file
  //
  //pub ret format: {"ip":"", "port": 0, "cnf_path":"", "cnf_dat":""} 
  TLOG4_INFO("ch: %s, sub info: %s", sCh.c_str(), 
             sSubRet.c_str());
  loss::CJsonObject jsHostCnf;
  if (false == jsHostCnf.Parse(sSubRet)) {
    TLOG4_ERROR("parse host cnf return by sub from json failed");
    return false;
  }

  if (jsHostCnf.IsEmpty()) {
    TLOG4_INFO("host cnf sub ret is empty json");
    return true;
  }
  std::string sIp, sCnfPath;
  loss::CJsonObject sCnfData;
  uint32_t uiPort = 0;
  if ( (!GetSubRetItem(HOSTCNF_IP,jsHostCnf,sIp) || sIp.empty())
      || (!GetSubRetItem(HOSTCNF_PATH,jsHostCnf,sCnfPath) || sCnfPath.empty())
      || (!GetSubRetItem(HOSTCNF_DATA,jsHostCnf,sCnfData) || sCnfData.IsEmpty())
      || (!GetSubRetItem(HOSTCNF_PORT,jsHostCnf,uiPort) || uiPort <= 0) ) {
    TLOG4_ERROR("get sub ret item failed");
    return false;
  }
                         
  if (m_pCnfAgent->GetEth0Ip() != sIp) {
    TLOG4_TRACE("receive host cnf pub not local host");
    return true;
  }

  std::string sFilePath = sCnfPath;
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

  std::string sCnfContent = sCnfData.ToFormattedString();
  ssize_t iWRet = ::write(iFd,  sCnfContent.c_str(), sCnfContent.size());
  if (iWRet <= -1) {
    TLOG4_ERROR("write file failed, errmsg: %s, file: %s", strerror(errno), sFilePath.c_str());
    ::close(iFd);
    return false;
  }
  ::close(iFd);

  m_pCnfAgent->SendKillSignToListenProcess(sIp, uiPort); 
  //
  TLOG4_INFO("sub host conf file path: %s", sCnfPath.c_str()); 
  return true;
}

//////
bool SrvNameRetProc::operator() (const std::string& sCh,
                 const std::string& sSubRet) {
  //TODO: parse json and  write content to srv name json file 
  ////pub ret format:
  // {"XX": [{"ip":"","port":1}, {"ip":"","port":2}] }
  // XX is srv_name
  // ==============
  // srv name file content format:
  //{
  // "down_stream": 
  // [{
  //    "ip": "127.0.0.1",
  //    "port": 60102,
  //    "node_type":  "test_1"
  //  }, {
  //    "ip": "127.0.0.1",
  //    "port": 60101,
  //    "node_type":  "test_1"
  //  }, {
  //    "ip": "127.0.0.1",
  //    "port": 60100,
  //    "node_type":  "test_1"
  //  }]
  //}
  //
  TLOG4_INFO("ch: %s, data: %s", sCh.c_str(), sSubRet.c_str());
  loss::CJsonObject jsSrvNameCnf, jsSrvNamePubRet;
  std::string sSrvNameCnfContent;
  if (false == m_pCnfAgent->GetSrvNameData(sSrvNameCnfContent)) {
    TLOG4_ERROR("get srv_name json data from  srv name file");
    return false;
  }

  bool bRet = UpdateSrvNameWithPubRet(sSrvNameCnfContent, sSubRet); 
  if (bRet == false) {
    TLOG4_ERROR("update srv name with new pub failed");
    return false;
  }
  if (false == m_pCnfAgent->WriteNewSrvNameDatFile(sSrvNameCnfContent)) {
    TLOG4_ERROR("write newest srv name list to file failed");
    return false;
  }
  //TODO: add cur version in shm memory.
  m_pCnfAgent->GetNewestSrvNameVer();
  return true;
}

bool SrvNameRetProc::UpdateSrvNameWithPubRet(std::string& sSrvNameCnfContent,
                                             const std::string& spubSrvName) {
  if (spubSrvName.empty()) {
    TLOG4_ERROR("srv name cnf empty or pub srv name string is empty");
    return false;
  }

  //spubsrvname format:
  //{"srv_name":[{"ip":"","port":0}, {},{}] }
  Json::Value pubJsRoot, srvNameCnfRoot;
  Json::Reader reader;
  if (false == reader.parse(spubSrvName.c_str(), pubJsRoot)) {
    TLOG4_ERROR("parse pub srv name string failed, str: %s",
                spubSrvName.c_str());
    return false;
  }

  Json::Value::Members vKeyList = pubJsRoot.getMemberNames();
  if (vKeyList.empty()) {
    TLOG4_ERROR("pub srv name has not srv name index name");
    return true;
  }
  //format: {"XXX": [{},{},{"ip":"","port":0}]
  //now,假定每次只pub 一个服务名的变更。
  std::string sSrvName = vKeyList[0]; //sSrvName is: XXX
  Json::Value jsPubIpPortList = pubJsRoot[sSrvName];
  if (jsPubIpPortList.isArray() == false) {
    TLOG4_ERROR(" %s value not list", sSrvName.c_str());
    return false;
  }
  if (sSrvNameCnfContent.empty()) {
    return WriteSrvNameCnfToEmptyFile(sSrvNameCnfContent,
                                      jsPubIpPortList, sSrvName);
  }

  Json::Value::ArrayIndex srvNmPubListSz = jsPubIpPortList.size();


  if (false == reader.parse(sSrvNameCnfContent.c_str(), srvNameCnfRoot)) {
    TLOG4_ERROR("parse  srv name cnf json from string failed"
                ",str: %s", sSrvNameCnfContent.c_str());
    return false;
  }

  Json::Value srvNameCnfList = srvNameCnfRoot[SRVNAME_CNFFILE_KEY];
  if (srvNameCnfList.isArray() == false) {
    TLOG4_ERROR("svr name cnf value not json list, key: %s", SRVNAME_CNFFILE_KEY);
    return false;
  }

  std::map<uint32_t,bool>vpubRetExistIndex,vSrvNCnfNeedToDel;
  
  Json::Value::ArrayIndex srvNameCnfListSz = srvNameCnfList.size();

  Json::Value::ArrayIndex cnfListIndex = 0;
  for (; cnfListIndex < srvNameCnfListSz; ++ cnfListIndex) {
    //for each svr name cnf item
    Json::Value jsIpPort = srvNameCnfList[cnfListIndex];
    if (jsIpPort[SRVNAME_NODETYPE].isString() == false) {
      TLOG4_ERROR("srv name cnf item not string,key: %s",
                  SRVNAME_NODETYPE);
      return false;
    }
    std::string nodeTypeVal = jsIpPort[SRVNAME_NODETYPE].asString();
    if (nodeTypeVal != sSrvName) {
      continue;
    }

    //using srv name in pub ret  find  svr name cnf .
    TLOG4_TRACE("find same node_type item in srv name cnf, node_type: %s",
                sSrvName.c_str());

    if (jsIpPort[SRVNAME_IP].isString() == false) {
      TLOG4_ERROR("%s, item: %s value not string", sSrvName.c_str(),SRVNAME_IP);
      return false;
    }
    if (jsIpPort[SRVNAME_PORT].isInt() == false &&
        jsIpPort[SRVNAME_PORT].isUInt() == false) {
      TLOG4_ERROR("%s, item: %s value not int",
                  sSrvName.c_str(),SRVNAME_PORT);
      return false;
    }
    std::string sIp = jsIpPort[SRVNAME_IP].asString();
    uint32_t uiPort = jsIpPort[SRVNAME_PORT].asUInt();
   
    //using srv name conf item find srv name pub ret.
    Json::Value::ArrayIndex indexPubSrvNmList = 0;
    for ( ;indexPubSrvNmList < srvNmPubListSz; ++indexPubSrvNmList) {
      Json::Value pubOneNode = jsPubIpPortList[indexPubSrvNmList];
      if (pubOneNode[SRVNAME_IP].isString() == false ||
          pubOneNode[SRVNAME_PORT].isInt() == false) {
        TLOG4_ERROR("srvname: %s, %s, %s in pub ret format err",
                    sSrvName.c_str(), SRVNAME_IP,SRVNAME_PORT);
        return false;;
      }

      std::string pubIp = pubOneNode[SRVNAME_IP].asString(); 
      uint32_t pubPort = pubOneNode[SRVNAME_PORT].asUInt();
      if (sIp == pubIp && pubPort == uiPort) {
        vpubRetExistIndex[indexPubSrvNmList] = true;
        TLOG4_TRACE("srname: %s, ip: %s, port: %u exist srv name cnf",
                    sSrvName.c_str(), pubIp.c_str(), pubPort);
        break;
      }
    }
    if (indexPubSrvNmList == srvNmPubListSz) {
      vSrvNCnfNeedToDel[cnfListIndex] = true;
      TLOG4_TRACE("srv name cnf item not in pub srv name list, need del node type: %s",
                  sSrvName.c_str());
    }
  }

  Json::Value newSrvNameCnfRoot, newSrvNameCnf; 
  uint32_t uiNewIndex = 0;
  for (cnfListIndex = 0; cnfListIndex < srvNameCnfListSz; ++cnfListIndex) {
    std::map<uint32_t,bool>::iterator it;
    it = vSrvNCnfNeedToDel.find(cnfListIndex);
    if (it != vSrvNCnfNeedToDel.end() && it->second == true) {
      continue;
    }

    Json::Value jsIpPort = srvNameCnfList[cnfListIndex];

    //TLOG4_TRACE("add ip port conf : %s", jsIpPort.toStyledString().c_str());
    newSrvNameCnf[uiNewIndex++] = jsIpPort;
  }

  //TLOG4_TRACE("new srv name conf content: %s", newSrvNameCnf.toStyledString().c_str());
  if (newSrvNameCnf.isNull() == true) {
    newSrvNameCnfRoot[SRVNAME_CNFFILE_KEY].resize(0);
  } else {
    newSrvNameCnfRoot[SRVNAME_CNFFILE_KEY] = newSrvNameCnf;
  }
  
  for (cnfListIndex = 0; cnfListIndex < srvNmPubListSz; ++cnfListIndex) {
    std::map<uint32_t,bool>::iterator it;
    it = vpubRetExistIndex.find(cnfListIndex);
    if (it != vpubRetExistIndex.end() && it->second == true) {
      continue;
    }
    // {"ip":"","port": 0}
    Json::Value pubRetNode = jsPubIpPortList[cnfListIndex];
    
    Json::Value addNewSrvNameNode;
    addNewSrvNameNode[SRVNAME_NODETYPE] = sSrvName;
    addNewSrvNameNode[SRVNAME_IP] = pubRetNode[SRVNAME_IP];          
    addNewSrvNameNode[SRVNAME_PORT] = pubRetNode[SRVNAME_PORT]; 
    
    newSrvNameCnfRoot[SRVNAME_CNFFILE_KEY].append(addNewSrvNameNode);
  }

  Json::StyledWriter toStringWrite;
  sSrvNameCnfContent = toStringWrite.write(newSrvNameCnfRoot);
  TLOG4_TRACE("new content: %s",sSrvNameCnfContent.c_str());

  return true;
}

bool SrvNameRetProc::WriteSrvNameCnfToEmptyFile(std::string& srvNameContent, 
                                                const Json::Value& jsPubIpPortList,
                                                const std::string& srvname) {
  Json::Value srvNameListRoot;
  srvNameListRoot[SRVNAME_CNFFILE_KEY].resize(0);
  
  Json::Value::ArrayIndex srvNmPubListSz = jsPubIpPortList.size();
  for (Json::Value::ArrayIndex  index = 0;  index < srvNmPubListSz; ++index) {
    Json::Value pubOneNode = jsPubIpPortList[index];

    if (pubOneNode[SRVNAME_IP].isString() == false ||
        pubOneNode[SRVNAME_PORT].isInt() == false) {
      TLOG4_ERROR("srvname: %s, %s, %s in pub ret format err",
                  srvname.c_str(), SRVNAME_IP,SRVNAME_PORT);
      return false;;
    }

    std::string pubIp = pubOneNode[SRVNAME_IP].asString(); 
    uint32_t pubPort = pubOneNode[SRVNAME_PORT].asUInt();
    Json::Value oneJsonHost;
    oneJsonHost[SRVNAME_NODETYPE] = srvname;
    oneJsonHost[SRVNAME_IP] = pubIp;  
    oneJsonHost[SRVNAME_PORT] = pubPort;
    srvNameListRoot[SRVNAME_CNFFILE_KEY].append(oneJsonHost);
  }

  Json::StyledWriter toStringWrite;
  srvNameContent.assign(toStringWrite.write(srvNameListRoot));

  TLOG4_TRACE("srv name first new content: %s",srvNameContent.c_str());
  return true;
}


//////////////////
}
