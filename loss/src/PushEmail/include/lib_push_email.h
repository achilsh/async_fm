/**
 * @file: lib_push_email.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-11
 */
#ifndef __PUSH_EMAIL_H__
#define __PUSH_EMAIL_H__

#include <vector>
#include <map>
#include "jwsmtp/mailer.h"
#include "util/json/CJsonObject.hpp"

using namespace loss;

namespace LIB_PUSH_EMAIL {

#define RECEIVER_MAX_NUM_ONECE   (100)

//同步发送邮件接口
class LibPushEamil {
  public:
   LibPushEamil(const loss::CJsonObject& jsSendCnf);
   virtual ~LibPushEamil();
   bool SendMail(const std::vector<std::string>& vRecver, 
                 const std::string& sSubjuect,
                 const std::string& sContent);
   std::string GetEmailResp() { return m_MailHandle.response(); }
  private:
   bool Init();
  private:
   jwsmtp::mailer m_MailHandle;
   // {"from": "", "authtype": "","smtp_user_name":"", "smtp_user_passwd":"", "smtp_server": ""}
   loss::CJsonObject m_SendCnf; 
   bool m_Init;
};
///
}
#endif
