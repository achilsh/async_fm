/**
 * @file: CmdAlarmReport.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-08
 */

#ifndef PLUGINGS_CMD_ALARMREPORT_H_
#define PLUGINGS_CMD_ALARMREPORT_H_ 


#include "cmd/Cmd.hpp"
#include "OssDefine.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 创建函数声明
 * @note 插件代码编译成so后存放到PluginServer的plugin目录，PluginServer加载动态库后调用create()
 * 创建插件类实例。
 */
  oss::Cmd* create();
#ifdef __cplusplus
}
#endif

#define ALARM_DB_IP_JSCNF_KEY       "db_ip"
#define ALARM_DB_PORT_JSCNF_KEY     "db_port"
#define ALARM_DB_USER_JSCNF_KEY     "db_user"
#define ALARM_DB_NAME_JSCNF_KEY     "db_name"
#define ALARM_DB_PASSWD_JS_KEY      "db_passwd"
#define ALARM_DB_TABNAME_JSCNF_KEY  "db_tbname"

#define ALARM_DB_NODTYPE_FIELD_NAME    "node_type"
#define ALARM_DB_DATA_FIELD_NAME       "alarm_content"
#define ALARM_DB_INSERT_TM_FIELD_NAME  "insert_time"
#define ALARM_DB_ISNOTIFY_FIELD_NAME   "is_notify"

//////////////////////////
class AlamInfoOp {
  public:
   AlamInfoOp(const loss::CJsonObject& jsCnf);
   virtual ~AlamInfoOp();
   virtual bool WriteAlarm() = 0;
   void SetAlarmInfo(const loss::CJsonObject& jsData) {
     m_AlarmData = jsData;
   }
   //
   log4cplus::Logger GetLogger();
   //
   void SetLogger(log4cplus::Logger* pLogger);
   void SetLabor(oss::OssLabor* pLabor);
  protected:
   loss::CJsonObject m_AlarmCnf;
   loss::CJsonObject m_AlarmData;

   log4cplus::Logger* m_pLoger;
   oss::OssLabor* m_pLabor;
};

//write alarm info to db.
class AlarmInfoDB: public AlamInfoOp {
  public:
   AlarmInfoDB(const loss::CJsonObject& jsCnf);
   virtual ~AlarmInfoDB();
   virtual bool WriteAlarm();
  private:
   bool ParseDbCnf();
   bool ParseAlarmInfo();
  private:
   std::string m_DbIp;
   unsigned int m_DbPort;
   std::string m_DBUser;
   std::string m_DbName;
   std::string m_DbPasswd;
   std::string m_TabNamePrefix;
   //
   std::string m_AlarmNodeType;
};


//////////////////////////////////////
class CmdAlarmReport: public oss::Cmd {
  public:
   CmdAlarmReport();
   virtual ~CmdAlarmReport();
   virtual bool Init(); 
   //
   virtual bool AnyMessage(
       const oss::tagMsgShell& stMsgShell,
       const MsgHead& oInMsgHead,
       const MsgBody& oInMsgBody);
  private:
   loss::CJsonObject m_AlarmCnf;
   AlamInfoOp* m_AlarmOp;
};

#endif

