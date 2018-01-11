/**
 * @file: alarm_push.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-09
 */
#ifndef __ALARM_PUSH_H__
#define __ALARM_PUSH_H__

#include "mysql_lib.h"
#include "base_task.h"
#include "base_timer.h"
#include "lib_push_email.h"

using namespace  BASE_TASK;
using namespace  BASE_TIMER;
using namespace  LIB_PUSH_EMAIL; 
using namespace  MYSQLUSE;

#define DB_IS_PH_PUSH_FIELD_NAME      "is_phone_push"
#define DB_IS_EMAIL_PUSH_FIELD_NAME   "is_email_push"

#define DB_PUSH_NUM_LIMIT   "10" 

class AlarmPush;

enum ALARM_PUSH_TYPE {
  PUSH_TP_NONE = 0,
  PUSH_TP_EMAIL = 1,
  PUSH_TP_PHONE = 2,
};

//------------------//
struct NodeTypeAlarmCnf {
  NodeTypeAlarmCnf(): mBeginPushEmailTm(time(NULL)),
      mBeginPushPhoneTm(time(NULL)) {
    mToEmailList.clear(); 
    mToPhoneList.clear();
  }

  ~NodeTypeAlarmCnf() {
    //
  }
  
  std::string mNodeType;
  std::vector<std::string> mToEmailList;
  std::vector<std::string> mToPhoneList;
  int32_t mBeginPushEmailTm;
  int32_t mBeginPushPhoneTm;
};
//-----------------------//
class AlarmInfoPusher {
  public:
   AlarmInfoPusher() {}
   virtual ~AlarmInfoPusher() {} 
   virtual bool PushAlarm(const NodeTypeAlarmCnf* alarmCnf, std::map<int64_t, std::string>& mpAlarm) = 0;
  private:
   //
};

//-----------------------------//
class EmailAlarmPusher: public AlarmInfoPusher {
 public:
  EmailAlarmPusher(loss::CJsonObject& jsEmailCnf);
  virtual ~EmailAlarmPusher();
  virtual bool PushAlarm(const NodeTypeAlarmCnf* emailCnf, std::map<int64_t, std::string>& mpAlarm);
 private:
  LibPushEamil m_EmailPusher;
};
//-------------------------------//
class PhoneAlarmPusher: public AlarmInfoPusher {
  public:
   PhoneAlarmPusher();
   virtual ~PhoneAlarmPusher();
   virtual bool PushAlarm(const NodeTypeAlarmCnf* phoneCnf, std::map<int64_t, std::string>& mpAlarm);
};

///////////////////////////////////////////////
class TimerLoadPushCnf: public BaseTimer {
 public:
  TimerLoadPushCnf(struct ev_loop* ptrLoop, ev_tstamp timerTm,
                   const loss::CJsonObject& oJsonConf);
  virtual ~TimerLoadPushCnf();
  virtual BASETIME_TMOUTRET TimeOut();
  virtual bool PreStart();
  
  bool GetLoadCnf(std::map<std::string, NodeTypeAlarmCnf*>& confData);
  
  log4cplus::Logger GetTLog() { return *m_pLoger; } 
  void SetLoger(log4cplus::Logger* pLoger) { m_pLoger = pLoger; }
 private:
  bool ParseCnf();
  std::map<std::string, NodeTypeAlarmCnf*> m_LoadCnfData;
  loss::CJsonObject m_alarmCnf;

  std::string m_sIp;
  unsigned int m_uiPort;

  std::string m_sUser; 
  std::string m_sPasswd;

  std::string m_sDbName;
  std::string m_sTabName;

  MysqlOp* m_pMySqlOp;
  log4cplus::Logger* m_pLoger;
};

//-----------------------------------------//
class TimerPushAlarm: public BaseTimer {
  public:
   TimerPushAlarm(struct ev_loop* ptrLoop, ev_tstamp timerTm, 
                  const loss::CJsonObject& oJsonConf, 
                  AlarmPush* alarmPush);
   virtual ~TimerPushAlarm();
   virtual BASETIME_TMOUTRET TimeOut();
   virtual bool PreStart();
   //
   bool PushAlarmInfo(const NodeTypeAlarmCnf* pPushCnf);
   bool LoadAlarmRecords(const NodeTypeAlarmCnf* pPushCnf,
                        std::map<int64_t,std::string>& mpAlarm,
                        int iAlarmType = PUSH_TP_NONE);

   bool UpdatePushedAlarm(const std::map<int64_t,std::string>& mpAlarm,
                          int iAlarmType = PUSH_TP_NONE);
   
   log4cplus::Logger GetTLog() { return *m_pLoger; } 
   void SetLoger(log4cplus::Logger* pLoger) { m_pLoger = pLoger; }
  private:
   bool ParseCnf();
   AlarmInfoPusher* GetPuhserInterance(const int iType);

   loss::CJsonObject m_AlarmCnf;
   AlarmPush* m_pAlarmPush;

   std::string m_sIp;
   unsigned int m_uiPort;

   std::string m_sUser; 
   std::string m_sPasswd;

   std::string m_sDbName;
   std::string m_sTabName;

   MysqlOp* m_pMySqlOp;
   std::map<int, AlarmInfoPusher*> m_mpPusher;
   log4cplus::Logger* m_pLoger;
};


class AlarmPush: public BaseTask {
  public: 
   AlarmPush();
   virtual ~AlarmPush();
   virtual int TaskInit(loss::CJsonObject& oJsonConf, uint32_t uiWkId);
   virtual int HandleLoop();
  
   bool GetLoadCnf(std::map<std::string, NodeTypeAlarmCnf*>& confData);
  private:
   bool PreRun();
   void Process();
 
  private:
   struct ev_loop*   m_PLoop;
   TimerLoadPushCnf* p_LoadTimer;
   TimerPushAlarm*   p_PushTimer;
   ev_tstamp m_CnfTmrVal;
   ev_tstamp m_PushTmrVal;
   loss::CJsonObject m_CnfJS;
   loss::CJsonObject m_LoadJS;
};

////
#endif

