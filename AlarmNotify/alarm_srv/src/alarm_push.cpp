#include "alarm_push.h"

//---------------------------------------//
EmailAlarmPusher::EmailAlarmPusher(loss::CJsonObject& jsEmailCnf)
  : m_EmailPusher(jsEmailCnf) {

}
EmailAlarmPusher::~EmailAlarmPusher() {
}

//----- Del fail pushed id  ---//
bool EmailAlarmPusher::PushAlarm(const NodeTypeAlarmCnf* emailCnf,
                                 std::map<int64_t, std::string>& mpAlarm) {
  std::map<int64_t, std::string>::iterator it;
  for (it = mpAlarm.begin();it != mpAlarm.end();) {
    
    std::string sNodeType = emailCnf->mNodeType;
    std::string sAlarmSubject = sNodeType + " alarm notify";

    if (m_EmailPusher.SendMail(emailCnf->mToEmailList,sAlarmSubject, it->second) == false) {
      mpAlarm.erase(it++);
      continue;
    }
    ++it;
  }
  return true;
}
//-------------------------------------//
PhoneAlarmPusher::PhoneAlarmPusher() {
}

PhoneAlarmPusher::~PhoneAlarmPusher() {
}

//----- Del fail pushed id  ---//
bool PhoneAlarmPusher::PushAlarm(const NodeTypeAlarmCnf* phoneCnf,
                                 std::map<int64_t, std::string>& mpAlarm) {
  mpAlarm.clear();
  return true;
}

//---------------------------------------//
TimerLoadPushCnf::TimerLoadPushCnf(struct ev_loop* ptrLoop,
                                   ev_tstamp timerTm,
                                   const loss::CJsonObject& oJsonConf)
  :BaseTimer(ptrLoop, timerTm),m_alarmCnf(oJsonConf), m_pMySqlOp(NULL), m_pLoger(NULL) {

}

TimerLoadPushCnf::~TimerLoadPushCnf() {
  std::map<std::string, NodeTypeAlarmCnf*>::iterator it;
  for (it = m_LoadCnfData.begin(); it != m_LoadCnfData.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }
  m_LoadCnfData.clear();

  if (m_pMySqlOp) {
    delete m_pMySqlOp; m_pMySqlOp = NULL;
  }
}

bool TimerLoadPushCnf::GetLoadCnf(std::map<std::string, NodeTypeAlarmCnf*>& confData) {
  confData.clear();
  confData.insert(m_LoadCnfData.begin(), m_LoadCnfData.end());
  return true;
}

/***
 * {"ip": "", "port": , "db_name": "", "tab_name": "", "user":"", "passwd": ""}
 *
 */
bool TimerLoadPushCnf::ParseCnf() {
  if (m_alarmCnf.IsEmpty()) {
    TLOG4_ERROR("push conf time is empty");
    return false;
  }
  if (m_alarmCnf.Get("ip", m_sIp) == false) {
    TLOG4_ERROR("ip in alarm conf fail");
    return false;
  }
  if (m_alarmCnf.Get("port", m_uiPort) == false) {
    TLOG4_ERROR("port in alarm conf fail");
    return false;
  }
  if (m_alarmCnf.Get("db_name", m_sDbName) == false) {
    TLOG4_ERROR("db_name in alarm conf fail");
    return false;
  }
  if (m_alarmCnf.Get("tab_name", m_sTabName) == false) {
    TLOG4_ERROR("tab_name in alarm conf fail");
    return false;
  }
  if (m_alarmCnf.Get("user", m_sUser) == false) {
    TLOG4_ERROR("user in alarm conf fail");
    return false;
  }
  if (m_alarmCnf.Get("passwd", m_sPasswd) == false) {
    TLOG4_ERROR("passwd in alarm conf fail");
    return false;
  }
  return true;
}

bool TimerLoadPushCnf::PreStart() {
  if (ParseCnf() == false) {
    return false;
  }

  m_pMySqlOp = new MysqlOp(m_sIp, m_sUser, m_sPasswd, m_uiPort);
  m_pMySqlOp->Connect();
  
  return true;
}

BASETIME_TMOUTRET TimerLoadPushCnf::TimeOut() {
  MysqlSelect sqlSelect;
  std::string sMultiField("node_type, email_list, phone_list, email_begin_tm, phone_begin_tm ");
  std::string sTab = m_sDbName + "." + m_sTabName;

  sqlSelect.MultiField(sMultiField);
  sqlSelect.From(sTab);

  std::string sSql = sqlSelect.sql();
  TLOG4_TRACE("load push cnf sql: %s", sSql.c_str());

  MysqlRes retSelectSql;
  int iRet = m_pMySqlOp->Query(sSql,retSelectSql);
  if (iRet != 0) {
    TLOG4_ERROR("execuate sql fail, sql: %s", sSql.c_str());
    return  RET_TIMER_RUNNING;
  }

  int iRowNum = retSelectSql.num_rows();
  for (int i = 0; i < iRowNum; ++i) {
    iRet = retSelectSql.FetchRow();
    if (iRet != 0) {
      continue;
    }

    std::string sNodeType = retSelectSql.Field();
    TLOG4_TRACE("node type: %s", sNodeType.c_str());

    //email list like: ***@126.com,***@126.com,
    std::string sEmailLIst = retSelectSql.Field();
    TLOG4_TRACE("email list: %s", sEmailLIst.c_str());

    //phone list like: 13870064182,13870017653,
    std::string sPhoneList = retSelectSql.Field();
    TLOG4_TRACE("phone list: %s", sPhoneList.c_str());

    std::vector<std::string> vEmailList;
    LIB_COMM::LibString::str2vec(sEmailLIst,vEmailList,",");

    std::vector<std::string> vPoneList;
    LIB_COMM::LibString::str2vec(sPhoneList,vPoneList,",");
    
    int iEmailBeginTm = strtoul(retSelectSql.Field(), NULL, 10);
    TLOG4_TRACE("email alarm begin tm: %u", iEmailBeginTm);

    int iPhoneBeginTm = strtoul(retSelectSql.Field(), NULL, 10); 
    TLOG4_TRACE("phone alarm begin tm: %u", iPhoneBeginTm);
    
    NodeTypeAlarmCnf* pNodeTyeDetail = new NodeTypeAlarmCnf();
    pNodeTyeDetail->mNodeType = sNodeType;
    
    pNodeTyeDetail->mToEmailList.clear();
    pNodeTyeDetail->mToEmailList.insert(
        pNodeTyeDetail->mToEmailList.end(), vEmailList.begin(), vEmailList.end());

    pNodeTyeDetail->mBeginPushEmailTm = iEmailBeginTm;
    
    pNodeTyeDetail->mToPhoneList.clear();
    pNodeTyeDetail->mToPhoneList.insert(
        pNodeTyeDetail->mToPhoneList.end(), vPoneList.begin(), vPoneList.end());

    pNodeTyeDetail->mBeginPushPhoneTm = iPhoneBeginTm;

    std::map<std::string, NodeTypeAlarmCnf*>::iterator it;
    it = m_LoadCnfData.find(sNodeType);
    if (it != m_LoadCnfData.end()) {
      delete it->second; it->second = pNodeTyeDetail;
    } else {
      m_LoadCnfData[sNodeType] = pNodeTyeDetail;
    }
  }
  //
  return RET_TIMER_RUNNING;
}

//--------------------------------------//
TimerPushAlarm::TimerPushAlarm(struct ev_loop* ptrLoop, ev_tstamp timerTm, 
                               const loss::CJsonObject& oJsonConf, AlarmPush* alarmPush)
  :BaseTimer(ptrLoop, timerTm), m_AlarmCnf(oJsonConf), m_pAlarmPush(alarmPush), m_pMySqlOp(NULL), m_pLoger(NULL) {
}

TimerPushAlarm::~TimerPushAlarm() {
  m_pAlarmPush = NULL;
  if (m_pMySqlOp) {
    delete m_pMySqlOp; m_pMySqlOp = NULL;
  }
  
  for (std::map<int, AlarmInfoPusher*>::iterator it = m_mpPusher.begin();
       it != m_mpPusher.end(); ++it) {
    if (it->second) {
      delete it->second;
    }
  }

  m_mpPusher.clear();
}

bool TimerPushAlarm::PreStart() {
  if (ParseCnf() == false) {
    return false;
  }

  m_pMySqlOp = new MysqlOp(m_sIp, m_sUser, m_sPasswd, m_uiPort);
  m_pMySqlOp->Connect();
  //...
  return true;
}

/***
 * {"ip": "", "port": , "db_name": "", "tab_name": "", "user":"", "passwd": ""}
 *
 */
bool TimerPushAlarm::ParseCnf() {
  if (m_AlarmCnf.IsEmpty()) {
    TLOG4_ERROR("push alarm timer cnf json is empty");
    return false;
  }
  if (m_AlarmCnf.Get("ip", m_sIp) == false) {
    TLOG4_ERROR("get ip item in alarm push conf fail");
    return false;
  }
  if (m_AlarmCnf.Get("port", m_uiPort) == false) {
    TLOG4_ERROR("get port item in alarm push conf fail");
    return false;
  }
  if (m_AlarmCnf.Get("db_name", m_sDbName) == false) {
    TLOG4_ERROR("get db_name  item in alarm push conf fail");
    return false;
  }
  if (m_AlarmCnf.Get("tab_name", m_sTabName) == false) {
    TLOG4_ERROR("get tab_name  item in alarm push conf fail");
    return false;
  }
  if (m_AlarmCnf.Get("user", m_sUser) == false) {
    TLOG4_ERROR("get user  item in alarm push conf fail");
    return false;
  }
  if (m_AlarmCnf.Get("passwd", m_sPasswd) == false) {
    TLOG4_ERROR("get passwd  item in alarm push conf fail");
    return false;
  }
  return true;
}

BASETIME_TMOUTRET TimerPushAlarm::TimeOut() {
  if (m_pAlarmPush == NULL) {
    return RET_TIMER_RUNNING;
  }

  std::map<std::string, NodeTypeAlarmCnf*> cnfNodeType;
  cnfNodeType.clear();
  if (m_pAlarmPush->GetLoadCnf(cnfNodeType) == false) {
    return RET_TIMER_RUNNING;
  }

  std::map<std::string, NodeTypeAlarmCnf*>::iterator it = cnfNodeType.begin();
  for (; it != cnfNodeType.end(); ++it) {
    PushAlarmInfo(it->second);
  }
  return RET_TIMER_RUNNING;
}

bool TimerPushAlarm::PushAlarmInfo(const NodeTypeAlarmCnf* pPushCnf) {
  if (pPushCnf == NULL || m_pMySqlOp == NULL) {
    return false;
  }

  do {
    std::map<int64_t, std::string> mpAlarmInfo;
    if (pPushCnf->mToEmailList.empty()) {
      break;
    }

    loss::CJsonObject jsEmailCnf;
    if (m_AlarmCnf.Get("email_pusher", jsEmailCnf) == false) {
      TLOG4_ERROR("get email_pusher item from jsconf fail");
      break;
    }

    std::map<int, AlarmInfoPusher*>::iterator it;
    it = m_mpPusher.find(PUSH_TP_EMAIL);
    if (it == m_mpPusher.end()) {
      m_mpPusher.insert(
          std::pair<int, AlarmInfoPusher*>(PUSH_TP_EMAIL, new EmailAlarmPusher(jsEmailCnf)));
    }

    /** load alarm record from alarm_tab ***/
    if (!LoadAlarmRecords(pPushCnf, mpAlarmInfo, PUSH_TP_EMAIL)) {
      break;
    }

    AlarmInfoPusher* m_pPusher = NULL;
    m_pPusher = GetPuhserInterance(PUSH_TP_EMAIL);
    if (m_pPusher == NULL) {
      break;
    }

    /** push email alarm  **/
    if (!m_pPusher->PushAlarm(pPushCnf, mpAlarmInfo)) {
    }

    /*** update pushed flag ***/
    if (!UpdatePushedAlarm(mpAlarmInfo, PUSH_TP_EMAIL)) {
      break;
    }

  } while(0);

  do {
    std::map<int64_t, std::string> mpAlarmInfo;
    if (pPushCnf->mToPhoneList.empty()) {
      break;
    }

    std::map<int, AlarmInfoPusher*>::iterator it;
    it = m_mpPusher.find(PUSH_TP_PHONE);
    if (it == m_mpPusher.end()) {
      m_mpPusher.insert(
          std::pair<int, AlarmInfoPusher*>(PUSH_TP_PHONE, new PhoneAlarmPusher()));
    }
    //load phone record from alarm_record_tab;
    if (!LoadAlarmRecords(pPushCnf, mpAlarmInfo, PUSH_TP_PHONE)) {
      break;
    }

    AlarmInfoPusher* m_pPusher = NULL;
    m_pPusher = GetPuhserInterance(PUSH_TP_PHONE);
    if (m_pPusher == NULL) {
      break;
    }

    /** push phone  alarm  ***/
    if (!m_pPusher->PushAlarm(pPushCnf, mpAlarmInfo)) {
    }

    /***update pushed flag ***/
    if (!UpdatePushedAlarm(mpAlarmInfo, PUSH_TP_PHONE)) {
      break;
    }

  } while(0);
  return true;
}

AlarmInfoPusher* TimerPushAlarm::GetPuhserInterance(const int iType) {
  if (m_mpPusher.find(iType) == m_mpPusher.end()) {
    return NULL;
  }
  return m_mpPusher[iType];
}

bool TimerPushAlarm::LoadAlarmRecords(const NodeTypeAlarmCnf* pPushCnf,
                                      std::map<int64_t, std::string>& mpAlarm,
                                      int iAlarmType) {
  if (PUSH_TP_NONE == iAlarmType) {
    return true;
  }

  std::string sNodeType = pPushCnf->mNodeType;
  int32_t tPushTm  = 0;
  std::string sPushTyCond;
  if (iAlarmType == PUSH_TP_EMAIL) {
    tPushTm =  pPushCnf->mBeginPushEmailTm;
    sPushTyCond = DB_IS_EMAIL_PUSH_FIELD_NAME + std::string(" =0");

  } else if (iAlarmType == PUSH_TP_PHONE) {
    tPushTm =  pPushCnf->mBeginPushPhoneTm;
    sPushTyCond = DB_IS_PH_PUSH_FIELD_NAME + std::string(" =0");
  }
  sPushTyCond += std::string(" limit ") + DB_PUSH_NUM_LIMIT;
  
  std::string sSql = "select alarm_content, id from ";
  sSql += m_sDbName + "." + m_sTabName + " where ";
  sSql += "node_type = '" + sNodeType +"' and insert_time >= ";
  sSql += tPushTm + " and " + sPushTyCond;
  TLOG4_TRACE("load alarm record sql: %s", sSql.c_str());
  //
  MysqlRes retSelectSql;
  int iRet = m_pMySqlOp->Query(sSql, retSelectSql);
  if (iRet != 0) {
    TLOG4_ERROR("excuate fail for sql: %s", sSql.c_str());
    return false;
  }

  int iRowNum = retSelectSql.num_rows();
  for (int i = 0; i < iRowNum; ++i) {
    if (retSelectSql.FetchRow() != 0) {
      continue;
    }
    //
    std::string sAlarmContent = retSelectSql.Field();
    int64_t iId = strtoul(retSelectSql.Field(), NULL, 10); 
    mpAlarm.insert(std::pair<int64_t, std::string>(
            iId, sAlarmContent));
  }

  return true;
}


bool TimerPushAlarm::UpdatePushedAlarm(const std::map<int64_t,std::string>& mpAlarm,
                                       int iAlarmType) {
  if (iAlarmType == PUSH_TP_NONE) {
    return true;
  }
  std::string sFieldSet;
  if (iAlarmType == PUSH_TP_EMAIL) {
    sFieldSet = DB_IS_EMAIL_PUSH_FIELD_NAME + std::string("=1");
  } else if (iAlarmType == PUSH_TP_PHONE) {
    sFieldSet = DB_IS_PH_PUSH_FIELD_NAME + std::string("=1");
  }

  std::map<int64_t,std::string>::const_iterator itCnt;
  for (itCnt = mpAlarm.begin(); itCnt != mpAlarm.end(); ++itCnt) {
    int64_t iIdLL = itCnt->first;
    std::stringstream ios;
    ios << iIdLL;
    
    std::string sSql = "update " + m_sDbName + "." + m_sTabName;
    sSql += " set " + sFieldSet;
    sSql += " where id = " + ios.str();
    
    TLOG4_TRACE("sql: %s", sSql.c_str());
    MysqlRes retSelectSql;
    int iRet = m_pMySqlOp->Query(sSql, retSelectSql);
    if (iRet != 0 ) {
      TLOG4_ERROR("excuate fail for sql: %s", sSql.c_str());
      continue;
    }
  }
  return true;
}

//-------------------------------------//
AlarmPush::AlarmPush(): m_PLoop(NULL), p_LoadTimer(NULL), p_PushTimer(NULL) {
  //
}

AlarmPush::~AlarmPush() {
  if (p_LoadTimer) {
    delete p_LoadTimer; p_LoadTimer = NULL;
  }
  if (p_PushTimer) {
    delete p_PushTimer; p_PushTimer = NULL;
  }

  if (m_PLoop != NULL) {
    ev_loop_destroy(m_PLoop); m_PLoop = NULL;
  }
}

int AlarmPush::TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWkId) {
  // get "alarm_conf": {"timer": 1.0, "": ""}
  // get "alarm_content": {"timer": , "": ""} 
  if (oJsonConf.Get("alarm_conf", m_CnfJS) == false) {
    TLOG4_ERROR("get alarm_conf item from conf fail");
    return -1;
  }
  if (oJsonConf.Get("alarm_content", m_LoadJS) == false) {
    TLOG4_ERROR("get alarm_content item from conf fail");
    return -1;
  }
  
  ev_tstamp defCnfTimerTm = 10.0, defLoadTimerTm = 3.0;
  if (m_CnfJS.Get("timer",defCnfTimerTm) == false) {
    TLOG4_ERROR("get load cnf timer time from cnf fail, use default: %u", defCnfTimerTm);
  }
  if (m_LoadJS.Get("timer", defLoadTimerTm) == false) {
    TLOG4_ERROR("get load content timer time from cnf fail, use default: %u", defLoadTimerTm);
  }

  m_CnfTmrVal = defCnfTimerTm;
  m_PushTmrVal = defLoadTimerTm;
  TLOG4_INFO("load cnf timer tm: %u, load content timer tm: %u", m_CnfTmrVal, m_PushTmrVal);
  return 0;
}

bool AlarmPush::PreRun() {
  m_PLoop = ev_loop_new(EVFLAG_AUTO);
  if (m_PLoop == NULL) {
    TLOG4_ERROR("new loop fail");
    return false;
  }

  p_LoadTimer = new TimerLoadPushCnf(m_PLoop, m_CnfTmrVal, m_CnfJS);
  if (p_LoadTimer == NULL) {
    TLOG4_ERROR("new TimerLoadPushCnf obj fail");
    return false;
  }
  p_LoadTimer->SetLoger(GetTLogPtr());

  p_PushTimer = new TimerPushAlarm(m_PLoop,m_PushTmrVal, m_LoadJS, this);
  if (p_PushTimer == NULL) {
    TLOG4_ERROR("new TimerPushAlarm obj fail");
    return false;
  }
  p_PushTimer->SetLoger(GetTLogPtr());
  
  TLOG4_TRACE("%s() done", __FUNCTION__);
  return true;
}

int AlarmPush::HandleLoop() {
  if (PreRun() == false) {
    return -1;
  }

  Process();
  return 0;
}

void AlarmPush::Process() {
  p_LoadTimer->StartTimer();
  p_PushTimer->StartTimer();
  
  ev_run(m_PLoop, 0);
}

bool AlarmPush::GetLoadCnf(std::map<std::string, NodeTypeAlarmCnf*>& confData) {
  if (p_LoadTimer == NULL) {
    return false;
  }
  return p_LoadTimer->GetLoadCnf(confData);
}
