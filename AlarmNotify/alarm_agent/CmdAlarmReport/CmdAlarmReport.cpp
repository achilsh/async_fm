#include   "CmdAlarmReport.h"
#include   "mysql_lib.h"

#ifdef __cplusplus
extern "C" {
#endif
    oss::Cmd* create()
    {
        oss::Cmd* pCmd = new CmdAlarmReport();
        return(pCmd);
    }
#ifdef __cplusplus
}
#endif

using namespace MYSQLUSE;

AlamInfoOp::AlamInfoOp(const loss::CJsonObject& jsCnf)
  : m_AlarmCnf(jsCnf), m_pLoger(NULL), m_pLabor(NULL) { }

AlamInfoOp::~AlamInfoOp() {
  m_pLoger = NULL; m_pLabor = NULL;
}

log4cplus::Logger AlamInfoOp::GetLogger() { return *m_pLoger; }

void AlamInfoOp::SetLogger(log4cplus::Logger* pLogger) {
  m_pLoger = pLogger;
}

void AlamInfoOp::SetLabor(oss::OssLabor* pLabor) {
  m_pLabor = pLabor;
}

//////
AlarmInfoDB::AlarmInfoDB(const loss::CJsonObject& jsCnf)
  : AlamInfoOp(jsCnf) {
}

AlarmInfoDB::~AlarmInfoDB() {
}

//
bool AlarmInfoDB::WriteAlarm() {
  if (ParseDbCnf() == false) {
    return false;
  }
  //
  if (ParseAlarmInfo() == false) {
    return false;
  }
  std::string alarm_tb_name;
  alarm_tb_name = m_DbName + "." + m_TabNamePrefix + "0";

  //---------------------------------------//
  // tab field:
  // node_type  ===>  varchar(128) 
  // alarm_content ===> varchar(1024)
  // insert_time ====> timestamp
  // is_notify   =====> tinyint
  //---------------------------------------//
  MysqlInsert  stSql(alarm_tb_name);
  stSql.Field<std::string>(ALARM_DB_NODTYPE_FIELD_NAME, m_AlarmNodeType);
  stSql.Field<std::string>(ALARM_DB_DATA_FIELD_NAME, m_AlarmData.ToString());
  stSql.Field<std::string>(ALARM_DB_INSERT_TM_FIELD_NAME, "now()");
  stSql.Field<int>(ALARM_DB_ISNOTIFY_FIELD_NAME,0);
  stSql.End();
  std::string sSql = stSql.sql();
  LOG4_TRACE("insert sql: %s",  sSql.c_str());

  MysqlOp  sqlOp(m_DbIp, m_DBUser,m_DbPasswd,m_DbPort);
  if (sqlOp.Connect() != 0) {
    LOG4_ERROR("connect db failed,sql: %s", sSql.c_str());
    return false;
  }

  if (sqlOp.Query(sSql) != 0) {
    LOG4_ERROR("insert new record fail,sql: %s", sSql.c_str());
    return false;
  }

  return true;
}

bool AlarmInfoDB::ParseAlarmInfo() {
  m_AlarmNodeType.clear();
  if (false == m_AlarmData.Get("node_type", m_AlarmNodeType)) {
    LOG4_ERROR("get node_type from alarm info fail");
    return false;
  }
  return true;
}

bool AlarmInfoDB::ParseDbCnf() {
  // alarm db of mysql conf like:
  //  {
  //    "db_ip": "128.0.0.1",
  //    "db_port": 3065,
  //    "db_user": "root",
  //    "db_name": "db_alarm_report",
  //    "db_passwd": "1231313",
  //    "db_tbname": "tb_alarm_report_"
  //  }
  //
  if (m_AlarmCnf.Get(ALARM_DB_IP_JSCNF_KEY, m_DbIp) == false) {
    LOG4_ERROR("get %s from json failed",ALARM_DB_IP_JSCNF_KEY);
    return false;
  }

  if (m_AlarmCnf.Get(ALARM_DB_PORT_JSCNF_KEY, m_DbPort) == false) {
    LOG4_ERROR("get %s from json failed", ALARM_DB_PORT_JSCNF_KEY);
    return false;
  }

  if (m_AlarmCnf.Get(ALARM_DB_USER_JSCNF_KEY, m_DBUser) == false) {
    LOG4_ERROR("get %s from json failed", ALARM_DB_USER_JSCNF_KEY);
    return false;
  }


  if (m_AlarmCnf.Get(ALARM_DB_NAME_JSCNF_KEY,m_DbName) == false) {
    LOG4_ERROR("get %s from json failed", ALARM_DB_NAME_JSCNF_KEY);
    return false;
  }
  
  if (m_AlarmCnf.Get(ALARM_DB_PASSWD_JS_KEY,m_DbPasswd) == false) {
    LOG4_ERROR("get %s from json failed",ALARM_DB_PASSWD_JS_KEY);
    return false;
  }
  

  if (m_AlarmCnf.Get(ALARM_DB_TABNAME_JSCNF_KEY,m_TabNamePrefix) == false) {
    LOG4_ERROR("get %s from json failed", ALARM_DB_TABNAME_JSCNF_KEY);
    return false;
  }
  return true;
}


/////////////////////////////////
CmdAlarmReport::CmdAlarmReport(): m_AlarmOp(NULL) {
}

CmdAlarmReport::~CmdAlarmReport() {
  if (m_AlarmOp != NULL) {
    delete m_AlarmOp; m_AlarmOp = NULL;
  }
}

bool CmdAlarmReport::AnyMessage(const oss::tagMsgShell& stMsgShell,
                                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody) {
  MsgHead oOutMsgHead;
  MsgBody oOutMsgBody;
  oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
  oOutMsgHead.set_seq(oInMsgHead.seq());
  oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
  GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
  // 
  // {
  //   "node_type":      "logic",
  //   "ip":             "192.168.1.1"
  //   "worker_id":      1
  //   "note":"上面几项不需要业务填充，接口自行获取填充"
  //   "call_interface": "GetInfo()",
  //   "file_name":      "test.cpp",
  //   "line":           123,
  //   "time":           "2018-1-1 12:00:00"
  //   "detail":         "get info fail"
  // }
  // 
  do {
    if ((int)oInMsgHead.cmd() != oss::CMD_REQ_ALARM_REPORT_CMD ||
        GetCmd() != (int)oInMsgHead.cmd()) {
      LOG4_ERROR("req not cmd: %u", oss::CMD_REQ_ALARM_REPORT_CMD);
      break;
    }

    loss::CJsonObject infoAlarm;
    if (false == infoAlarm.Parse(oInMsgBody.body())) {
      LOG4_ERROR("parse recv alarm info with json failed");
      break;
    }
    
    if (m_AlarmOp == NULL) {
      m_AlarmOp = new AlarmInfoDB(m_AlarmCnf);
      m_AlarmOp->SetLogger(GetLoggerPtr());
      m_AlarmOp->SetLabor(GetLabor());
    }
    
    m_AlarmOp->SetAlarmInfo(infoAlarm);

    if (false == m_AlarmOp->WriteAlarm()) {
      LOG4_ERROR("write alarm fail");
      break;
    }

  } while(0);
  //
  return true;
}

///
bool CmdAlarmReport::Init() {
  //get alarm cnf info from "custom": {} item in json conf.
  m_AlarmCnf = GetLabor()->GetCustomConf();
  return true;
}

