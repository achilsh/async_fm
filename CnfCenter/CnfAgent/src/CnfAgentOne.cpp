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

bool CnfAgentOne::AddSubProcMethod() { //call by base init...

  SubRetProcBase* retProc = new HostCnfRetProc(HOST_CNF_CHANNEL, this);
  retProc->SetLog(GetTLog());
  if (false == RegisteSubRetProc(HOST_CNF_CHANNEL,retProc )) {
    TLOG4_ERROR("register hsot_conf  sub ret proc failed");
    return false;
  }
  if (false == RegisteFullSyncFuc(HOST_CNF_CHANNEL,std::bind(
              &CnfAgentOne::SyncHostCnfDetailInfo, this))) {
    TLOG4_ERROR("register host_cnf sync handler failed");                                       
    return false;  
  }
  //
  retProc = new WhiteListRetProc(WHIT_LIST_CHANNEL ,this);
  retProc->SetLog(GetTLog());
  if (false == RegisteSubRetProc(WHIT_LIST_CHANNEL,retProc)) {
    TLOG4_ERROR("register white_list  sub ret proc failed");
    return false;
  }
  if (false == RegisteFullSyncFuc(WHIT_LIST_CHANNEL,std::bind(
              &CnfAgentOne::SyncAllWhiteListCnf,this))) {
      TLOG4_ERROR("register white_list sync handle failed");
      return false;
  }
  
  //////////////////
  retProc = new SrvNameRetProc(SRV_NAME_CHANNEL, this);
  retProc->SetLog(GetTLog());
  if (false == RegisteSubRetProc(SRV_NAME_CHANNEL,retProc)) {
    TLOG4_ERROR("register srv_name sub ret proc failed");
    return false;
  }
  if (false == RegisteFullSyncFuc(SRV_NAME_CHANNEL,std::bind(
              &CnfAgentOne::SyncAllCnfSerNameList,this))) {
    TLOG4_ERROR("register srv_name sync handle failed");
    return false;
  }
  return true;
}

bool CnfAgentOne::WriteCnfFile(const std::string& fName, const std::string& jsConttent) {
  std::string sFilePath = fName;
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

  if (jsConttent.empty()) {
    ::close(iFd);
    return true;
  }

  ssize_t iWRet = ::write(iFd,  jsConttent.c_str(), jsConttent.size());
  if (iWRet <= -1) {
    TLOG4_ERROR("write file failed, errmsg: %s, file: %s", strerror(errno), sFilePath.c_str());
    ::close(iFd);
    return false;
  }
  ::close(iFd);

  return true;
}

bool CnfAgentOne::WriteNewSrvNameDatFile(const std::string& sCnfContent) {
  return WriteCnfFile(m_srvNameStoreFileName,sCnfContent);
  
}

bool CnfAgentOne::GetSrvNameData(std::string& sSrvNameContent) {
  return GetCnfData(m_srvNameStoreFileName, sSrvNameContent);
}

void CnfAgentOne::GetNewNodeTypeCmdVer() {
  std::string sKey = NODETYPE_CMD_VER_KEY;
  GetShmVersion(sKey);
}

void CnfAgentOne::GetNewestSrvNameVer() {
  std::string  sKey = SRV_NAME_VER_KEY;
  GetShmVersion(sKey);
}

bool CnfAgentOne::GetShmVersion(const std::string& sKey) {
  if (m_srvNameShmInit == false) {
    TLOG4_ERROR("shm init failed, shm key: %s", sKey.c_str());
    return false;
  }

  RedisKey  rKey(m_syncRedisCli);
  int64_t iLVerNo = 0;

  bool bRet = rKey.Incr(sKey, iLVerNo);  
  if (bRet == false) {
    TLOG4_ERROR("incr version fail, key: %s", sKey.c_str());
    return false;
  }
  TLOG4_TRACE("ver No: %lld, key: %s", iLVerNo, sKey.c_str());
  
  if (false == m_srvNameVerShm.SetValue(sKey,iLVerNo)) {
    TLOG4_ERROR("set shm value fail, errmsg: %s, key: %s",
               m_srvNameVerShm.GetErrMsg().c_str(), sKey.c_str());
    return false;
  }
  return true;
}

void CnfAgentOne::DoWorkAfterSync() {
  this->GetNewestSrvNameVer();
  this->SendUSR2SignelToLocalHostSrv();
}

void CnfAgentOne::SendUSR2SignelToLocalHostSrv() {
  std::vector<std::string> vHostCnfRedisKey;
  if (false == GetHostCnfRedisKey(vHostCnfRedisKey)) {
    TLOG4_ERROR("get local host conf key from redis failed");
    return ;
  }
  if (vHostCnfRedisKey.empty()) {
    TLOG4_TRACE("has not config any host srv for this local host");
    return ;
  }
  for (std::vector<std::string>::iterator it = vHostCnfRedisKey.begin();
       it != vHostCnfRedisKey.end(); ++it) {
    std::vector<std::string> vIpPortSrvName;
    LIB_COMM::LibString::str2vec(*it, vIpPortSrvName, ":");
    const std::string& sHost = vIpPortSrvName[1];
    uint32_t uiPort = ::atoi(vIpPortSrvName[2].c_str());
    SendKillSignToListenProcess(sHost, uiPort, "USR2");
    TLOG4_TRACE("send USR2 signal to process, ip: %s, port: %u",
                sHost.c_str(), uiPort);
  }
}

void CnfAgentOne::SendUSR2ToLocalUpdateNodeTypeCmd() {
  this->SendUSR2SignelToLocalHostSrv();
  TLOG4_TRACE("send usr2 signal to update node type cmd");
}

bool CnfAgentOne::GetNodeTypeCmdFromLocalFile(loss::CJsonObject& jsCnf)  {
  std::string sNodeTypeCmdCnfFile = NODETYPECMDSTOREFILE;
  std::string sCnfData; 
  if (false == GetCnfData(sNodeTypeCmdCnfFile,sCnfData)) {
    TLOG4_ERROR("get cnf data failed, file: %s", sNodeTypeCmdCnfFile.c_str());
    return false;
  }
  if (sCnfData.empty()) {
    return true;
  }
  
  if (false == jsCnf.Parse(sCnfData)) {
    TLOG4_ERROR("parse node type cmd relation from str to js fail");
    return false;
  }
  return true;
}

bool CnfAgentOne::WriteNodeTypeCmdToFile(const std::string& strjsCnf) {
  std::string sNodeTypeCmdCnfFile = NODETYPECMDSTOREFILE;
  return WriteCnfFile(sNodeTypeCmdCnfFile, strjsCnf);
}

/*****************************************/
//服务名配置存储格式为：string
//key: 服务名
//value: [{ip, port}, {"ip":"","port": 0} ] 
bool CnfAgentOne::SyncAllCnfSerNameList() {
  std::string sKey;
  sKey.append(PREFIXSVRNMKEY);
  std::vector<std::string> vKeyRet;
  GetRedisKeys(sKey, vKeyRet);

  if (vKeyRet.empty()) {
    TLOG4_TRACE("srv name cnf key: %s  is empty", sKey.c_str());
    ResetSrvNameFileContent(m_srvNameStoreFileName);
    //del all srv name list in srv name file
    return true;
  }

  RedisKey rKOp(m_syncRedisCli);
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

  DoWorkAfterSync();
  return true;
}

//[{},{},{} ]
bool CnfAgentOne::WriteSrvNameInfoInLocal(const std::map<std::string,std::string>& srvNameSet,
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

//redis format is hash, key =>  prefix:ip:port:servername
//field: 1 ==> conf path file name, 2 ===> cnf content,json format
// 服务配置文件存储格式为： (hash)
// key: ip + port + servername, 
// field: 1, value: cnf path value,include file name
// field: 2, value: json formate conf file.
bool CnfAgentOne::SyncHostCnfDetailInfo() {
  if (false == SyncNodeTyeCmd()) {
    TLOG4_ERROR("sync node type cmd relation fail");
  }

  std::vector<std::string> vKeyRet;
  if (GetHostCnfRedisKey(vKeyRet) == false) {
    TLOG4_ERROR("get host conf redis key failed");
    return false;
  }
  if (vKeyRet.empty()) {
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
    const std::string& sHostIp = vIpPortSrvName[1];
    SendKillSignToListenProcess(sHostIp, uiPort);
  }
  return true;
}


bool CnfAgentOne::SyncNodeTyeCmd() {
  std::map<std::string, std::vector<std::string> > mpNodeTypeCmd;
  std::string sKey;
  sKey.append(NODETYPE_CMD_KEY_PREX);
  sKey.append(":*");
  TLOG4_TRACE("get nodetype cmd key:  %s" , sKey.c_str());

  RedisKey rKOp(m_syncRedisCli);
  std::vector<std::string> vKeyRet;
  if (false == rKOp.Keys(sKey, vKeyRet)) {
    TLOG4_ERROR("cmd: KEYS %s  failed", sKey.c_str());
    return false;
  }
  
  if (vKeyRet.empty()) {
   TLOG4_TRACE("get keys ret is empty");
   WriteNodeTypeCmdToFile("");
   return true;
  }

 
  std::vector<std::string>::iterator it;
  for (it = vKeyRet.begin(); it != vKeyRet.end(); ++it) {
    sKey = *it;

    std::size_t sIndex = it->find_last_of(":");
    std::string sNodeType = it->substr(sIndex + 1); 
    
    RedisSets setOp(m_syncRedisCli);
    if (false == setOp.Smembers(sKey, mpNodeTypeCmd[sNodeType])) {
      TLOG4_ERROR("get member failed, set key: %s", sKey.c_str());
      mpNodeTypeCmd.erase(sNodeType);
      continue;
    }
  }
  
  loss::CJsonObject jsNodeTypeCmd;
  std::map<std::string, std::vector<std::string> >::iterator mpIt; 
  
  for (mpIt = mpNodeTypeCmd.begin(); mpIt != mpNodeTypeCmd.end(); ++mpIt) {
    jsNodeTypeCmd.AddEmptySubArray(mpIt->first);
    loss::CJsonObject& vJsCmd = jsNodeTypeCmd[mpIt->first];
    
    std::vector<std::string>::iterator itCmd;
    for (itCmd = mpIt->second.begin(); itCmd != mpIt->second.end(); ++itCmd) {
      vJsCmd.Add(atoi(itCmd->c_str()));
    }
  }

  std::string strNodeTypeCmd = jsNodeTypeCmd.ToFormattedString();
  if (false == WriteNodeTypeCmdToFile(strNodeTypeCmd)) {
    TLOG4_ERROR("write node type cmd to file failed");
    return false;
  }

  GetNewNodeTypeCmdVer();
  SendUSR2ToLocalUpdateNodeTypeCmd();
  return true;
}

bool CnfAgentOne::GetWhiteListFromFile(std::string& sWhitListCnf) {
    return GetCnfData(m_whiteListStoreFileName, sWhitListCnf);
}

bool CnfAgentOne::WriteWhiteListToFile(const std::string& sWhiteListCnf) {
    return WriteCnfFile(m_whiteListStoreFileName, sWhiteListCnf);
}

void CnfAgentOne::GetNewWhiteListVer() {
  std::string  sKey = WHITE_LIST_VER_KEY;
  GetShmVersion(sKey);
}

bool CnfAgentOne::GetRedisKeys(const std::string& sKeyPre,
                            std::vector<std::string>& vKeyRet) {
    if (sKeyPre.empty()) {
        return false;
    }

    std::string sKeyPrefix = sKeyPre;
    sKeyPrefix.append(":*");
    RedisKey rKOp(m_syncRedisCli);
    if (false == rKOp.Keys(sKeyPrefix, vKeyRet)) {
        TLOG4_ERROR("cmd: KEYS %s  failed", sKeyPrefix.c_str());
        return false;
    }
    return true;
}

bool CnfAgentOne::SyncAllWhiteListCnf () {
    std::vector<std::string> vKeyRet;
    std::string sKeyPre = PREFIXWLKEY;

    if (false == GetRedisKeys(sKeyPre, vKeyRet)) {
        return false;
    }

    if (vKeyRet.empty()) {
        ResetSrvNameFileContent(m_whiteListStoreFileName);
        return true;
    }

    //get white list from redis, key is srvname, val is <field, field_val>
    std::map<std::string, std::vector<std::string> >mpWhiteList;
    RedisHash rHash(m_syncRedisCli);
    
    std::vector<std::string>::iterator it;
    for (it = vKeyRet.begin();it != vKeyRet.end(); ++it) {
        std::string& sKey = *it; //pre + srvname

        std::size_t sIndex = it->find_last_of(":");
        std::string sSrvName = it->substr(sIndex + 1);
        
        if (rHash.hGetAll(sKey, mpWhiteList[sSrvName]) == false) {
            TLOG4_ERROR("hgetall faild, key: %s",sKey.c_str());
            mpWhiteList.erase(sSrvName); //del empty of vector
            continue;
        }
    }

    //每次全量去拉取 覆盖文件写
    if (false == WriteWhiteListInLocal(mpWhiteList, m_whiteListStoreFileName)) {
        TLOG4_ERROR("write white list to local fail");
        return false;
    }

    this->GetNewWhiteListVer();
    this->SendUSR2SignelToLocalHostSrv();
    //send version update and signal
    return true;
}

bool CnfAgentOne::WriteWhiteListInLocal(const std::map<std::string, 
                                        std::vector<std::string> >&mpWList, 
                                        const std::string& sFileName) {
    if (sFileName.empty()) {
        TLOG4_INFO("store white list cnf file name is empty string");
        return true;
    }
    if (mpWList.empty()) {
        WriteCnfFile(sFileName, "");
        return true ;
    }

    std::string sWhiteList;
    loss::CJsonObject jsWhiteList;
    std::vector<std::string>::const_iterator itVct;
    std::map<std::string, std::vector<std::string> >::const_iterator it;
    
    for (it = mpWList.begin(); it !=  mpWList.end(); ++it) {
        std::string sSrvName  = it->first;
        jsWhiteList.AddEmptySubArray(sSrvName);
        int ii = 0;
        for (itVct = it->second.begin(); itVct != it->second.end(); ++itVct, ++ii) {
            //each itVct is field, val;
            if (ii%2 == 0) {
                //is ip:port; field
            } else {
                //is {"ip": "","port":0,"id":["","",""]}
                loss::CJsonObject jsFieldVal;
                if (jsFieldVal.Parse(*itVct) == false) {
                    TLOG4_ERROR("parse field val fail from js, val: %s", 
                                itVct->c_str());
                    continue;
                }
                jsWhiteList[sSrvName].Add(jsFieldVal); //add in {"srv_name": [{}, {}, {}]}
            }
        }
    }

    sWhiteList = jsWhiteList.ToString();
    if (false == WriteWhiteListToFile(sWhiteList)) {
        TLOG4_ERROR("write data: %s to file fail", sWhiteList.c_str());
        return false;
    }
    return true;
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
  //pub ret format: {"nodetype_cmd": "***", "***": [191,21,3]}
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

  std::string sNodeTypeCmdVal;
  if (GetSubRetItem(NODETYPECMD_TYPE, jsHostCnf, sNodeTypeCmdVal)) {
    if (sNodeTypeCmdVal.empty()) {
      TLOG4_ERROR("item: %s value is empty", NODETYPECMD_TYPE);
      return false;
    }
    // input params is {"nodetype_cmd": "***","****":[]},  "*****"
    return ProcSubNodeTypeCmdModify(sNodeTypeCmdVal, jsHostCnf);
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

//pub ret format: {"nodetype_cmd": "***", "***": [191,21,3]}
//sNodeType is "****", is node type value.
bool HostCnfRetProc::ProcSubNodeTypeCmdModify(const std::string& sNodeType,
                                              const loss::CJsonObject& jsSubRet) {
  if (sNodeType.empty() || jsSubRet.IsEmpty()) {
    TLOG4_ERROR("input invaild param, is empty");
    return false;
  }

  loss::CJsonObject vCmdJson; //[1,2,3,4,5,7]
  if (false == GetSubRetItem(sNodeType,jsSubRet, vCmdJson)) {
    TLOG4_ERROR("get cmd list from sub ret, cmd key: %s", sNodeType.c_str());
    return false;
  }

  if (vCmdJson.IsArray() == false) {
    TLOG4_ERROR("cmd list key: %s, value not array", sNodeType.c_str());
    return false;
  }

  loss::CJsonObject jsFileCnf;
  //node type cmd format is: {"****": [1,2,3,4,5], "**": [7,8,9]}
  if (false == m_pCnfAgent->GetNodeTypeCmdFromLocalFile(jsFileCnf)) {
    TLOG4_ERROR("get node type cmd rel from local file fail");
    return false;
  }

  if (false == UpdateSubRetToLocalHostCnf(jsFileCnf, vCmdJson, sNodeType)) { //1. {"***": [1,2,3,4], "***":[3,45,6]}, 2. [12,3,6,7]
    TLOG4_ERROR("update sub ret to local host cnf fail");
    return false;
  }
  std::string sNewNodeTyeCmdJsCnf = jsFileCnf.ToFormattedString();
  //将该数据写入到特定目录下的文件,文件内容格式为： {"node_type1": [1,2,3,4,5,6],
  // "node_type2": [1,2,3,4,5,6]}; 其中 node_typen等同于srv_name
  if (false == m_pCnfAgent->WriteNodeTypeCmdToFile(sNewNodeTyeCmdJsCnf)) {
    TLOG4_ERROR("write node type cmd relation to file fail, node type: %s", sNodeType.c_str());
    return false;
  }
  
  m_pCnfAgent->GetNewNodeTypeCmdVer();
  m_pCnfAgent->SendUSR2ToLocalUpdateNodeTypeCmd();
  return true;
}


//add new item not exist toJson but exist in fromJson, 
//del del item 
bool HostCnfRetProc::UpdateSubRetToLocalHostCnf(loss::CJsonObject& toJson, /**{"***":[1,3,4,5], "***":[4,5,67]} ***/
                                                const loss::CJsonObject& vfromJson, /***[1,2,3,4,6] ***/
                                                const std::string& sNodeType) {
  if (sNodeType.empty()) {
    TLOG4_ERROR("node type is empty");
    return false;
  }

  loss::CJsonObject vjsCmdListCnf; 
  if (false == GetSubRetItem(sNodeType, toJson, vjsCmdListCnf)) {
    toJson.Add(sNodeType,vfromJson);
    return true;
  }

  std::vector<int> vCmdTo, vCmdFrom;
  for (int ii = 0; ii < vjsCmdListCnf.GetArraySize(); ++ii) {
    int iCmd = 0;
    if (vjsCmdListCnf.Get(ii, iCmd)) {
      vCmdTo.push_back(iCmd);
    }
  }

  for (int ii = 0; ii < vfromJson.GetArraySize(); ++ii) {
    int iCmd = 0;
    if (vfromJson.Get(ii, iCmd)) {
      vCmdFrom.push_back(iCmd);
    }
  }
  
  for (std::vector<int>::iterator it = vCmdFrom.begin(); it != vCmdFrom.end(); ++it) {
    std::vector<int>::iterator itFind = std::find(vCmdTo.begin(), vCmdTo.end(), *it);
    if (itFind == vCmdTo.end()) {
      vCmdTo.push_back(*it);
    }
  }

  for (std::vector<int>::iterator it = vCmdTo.begin(); it != vCmdTo.end(); ) {
    std::vector<int>::iterator itFind = std::find(vCmdFrom.begin(), vCmdFrom.end(), *it);
    if (itFind == vCmdFrom.end()) {
      it = vCmdTo.erase(it);
      continue;
    }
    ++it;
  }

  loss::CJsonObject newCmdIdList;
  for (std::vector<int>::iterator it = vCmdTo.begin(); it != vCmdTo.end(); ++it) {
    newCmdIdList.Add(*it);
  }

  toJson.Replace(sNodeType, newCmdIdList);
  return true;
}

///------------------------------///
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
  m_pCnfAgent->SendUSR2SignelToLocalHostSrv();
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

//-----------------------------------//
WhiteListRetProc::WhiteListRetProc(const std::string& sCh,
                                   SubCnfTask::CnfAgentOne *pAgent) 
    :SubRetProcBase (sCh), m_pCnfAgent(pAgent) {
    //
}
WhiteListRetProc::~WhiteListRetProc() {
    //
}
//
//recv push data like this:
//
// {"test":[{"ip":"192.168.1.106","port":25000,"id":["111","222"]}]}
//
bool WhiteListRetProc::operator()(const std::string& sCh, 
                                  const std::string& sSubRet) {
    TLOG4_INFO("recv ch: %s, sub info: %s", sCh.c_str(), sSubRet.c_str());
    loss::CJsonObject  jsWhiteList, jsWhiteListCnf;
    std::string sWhiteListCnf;
    if (false == m_pCnfAgent->GetWhiteListFromFile(sWhiteListCnf)) {
        TLOG4_ERROR("get white list from file fail");
        return false;
    }
    bool bRet = MergeSubRetAndLocalFile(sWhiteListCnf,sSubRet);
    if (bRet == false) {
        TLOG4_ERROR("merge sub ret to local file fail");
        return false;
    }
    if (false == m_pCnfAgent->WriteWhiteListToFile(sWhiteListCnf)) {
        TLOG4_ERROR("write newest white list to file fail");
        return false;
    }

    m_pCnfAgent->GetNewWhiteListVer();
    m_pCnfAgent->SendUSR2SignelToLocalHostSrv();

    return true;
}

//sWLSubRet format: 
// {"test":[{"ip":"192.168.1.106","port":25000,"id":["111",
//      "222"]}]}
//
//  sWLCnf format:
//  {"test":[{"":"", "": , "id":["","",""]}, {}], "bb":[], "cc":[] }
bool WhiteListRetProc::MergeSubRetAndLocalFile(std::string& sWLCnf,
                                               const std::string& sWLSubRet) {
    if (sWLSubRet.empty()) {
        return false;
    }
    Json::Value pubJsRoot;
    Json::Reader reader;
    if (false == reader.parse(sWLSubRet,pubJsRoot)) {
        TLOG4_ERROR("parse pub white list string from json fail, %s",
                   sWLSubRet.c_str());
        return false;
    }
    Json::Value::Members vNodeTypeKey = pubJsRoot.getMemberNames();
    if (vNodeTypeKey.empty()) {
        TLOG4_ERROR("pub white list has not any index name");
        return true;
    }

    std::string sWLSrvName = vNodeTypeKey[0];
    if (sWLSrvName.empty()) {
        TLOG4_ERROR("srv name is empty");
        return false;
    }
    //parse pub ret;
    Json::Value jsPubIpPortList = pubJsRoot[sWLSrvName];
    if (jsPubIpPortList.isArray() == false) {
        TLOG4_ERROR("%s value not list", sWLSrvName.c_str());
        return false;
    }
    if (sWLCnf.empty()) {
        sWLCnf.assign(sWLSubRet);
        return true;
    }

    loss::CJsonObject jsWLCnf, jsWLRet;
    loss::CJsonObject retArrWL;
    
    if (false == jsWLCnf.Parse(sWLCnf) || jsWLRet.Parse(sWLSubRet) == false) {
        TLOG4_ERROR("parse white list conf js fail,dat: %s",
                    sWLCnf.c_str());
        return false;
    }

    if (false == jsWLRet.Get(sWLSrvName, retArrWL) ||
        retArrWL.IsArray() == false) {
        TLOG4_ERROR("parse pub ret fail");
        return false;
    }

    //parse cnf
    loss::CJsonObject cnfArrWL = jsWLCnf[sWLSrvName];
    if (cnfArrWL.IsArray() == false) {
        jsWLCnf.Delete(sWLSrvName);
        jsWLCnf.Add(sWLSrvName, retArrWL);

        sWLCnf.assign(jsWLCnf.ToString());
        TLOG4_TRACE("new white list cnf: %s", sWLCnf.c_str());
        return true;
    }
    TLOG4_TRACE("from cnf get white list, srv: %s, array: %s",
               sWLSrvName.c_str(),cnfArrWL.ToString().c_str());

    std::map<std::string, loss::CJsonObject*> mpPubRet;
    std::map<std::string, loss::CJsonObject*>::iterator iterPubRet;
    //check pub ret each item, and add it into cnf
    for (int ii = 0; ii < retArrWL.GetArraySize(); ++ii) {
        std::string sIpPort;
        std::string sIp; unsigned int uiPort;
        if (false == retArrWL[ii].Get("ip", sIp)) {
            continue;
        }
        if (false == retArrWL[ii].Get("port", uiPort)) {
            continue;
        }
        std::stringstream ios;
        ios << sIp << ":" << uiPort;
        sIpPort = ios.str();
        mpPubRet[sIpPort] = &retArrWL[ii];
    }
    bool bReset = false;
    //reset cnf list to empty
    if (retArrWL.GetArraySize() == 0) {
        TLOG4_TRACE("clear array of white list in cnf");
        bReset = true;
    }
    if ( bReset == true ) {
        int ii = 0;
        while (ii < cnfArrWL.GetArraySize()) {
            cnfArrWL.Delete(ii);
        }
    }
    
    for (int ii = 0; ii < cnfArrWL.GetArraySize(); ++ii) {
        std::string sIp; unsigned int uiPort;
        if (cnfArrWL[ii].Get("ip",sIp) == false) {
            continue;
        }
        if (cnfArrWL[ii].Get("port",uiPort) == false) {
            continue;
        }
        std::stringstream ios;
        ios << sIp << ":" << uiPort;
        std::string sIpPort = ios.str();

        if (mpPubRet.find(sIpPort) == mpPubRet.end()) {
            continue;
        }
        //over write data
        cnfArrWL.Replace(ii,(*mpPubRet[sIpPort]));
        mpPubRet.erase(sIpPort);
    }
    
    //add suplus.
    for (iterPubRet = mpPubRet.begin(); iterPubRet != mpPubRet.end(); ++ iterPubRet)  {
        cnfArrWL.Add(*(iterPubRet->second));
    }
    
    jsWLCnf.Delete(sWLSrvName);
    jsWLCnf.Add(sWLSrvName,cnfArrWL);
    sWLCnf.assign(jsWLCnf.ToString());
   
    TLOG4_TRACE("new white list cnf: %s", sWLCnf.c_str());
    return true;
}
//////////////////
}
