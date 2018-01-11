#include "lib_push_email.h"


namespace LIB_PUSH_EMAIL {
  LibPushEamil::LibPushEamil(const loss::CJsonObject& jsSendCnf):m_SendCnf(jsSendCnf), m_Init(false) {
  }
  LibPushEamil::~LibPushEamil() {
  }

  bool LibPushEamil::Init() {
    if (m_SendCnf.IsEmpty()) {
      return false;
    }
    std::vector<std::string> sJsKey;
    std::map<std::string, std::string> mpCnfKV;
    
    sJsKey.push_back("from");
    sJsKey.push_back("authtype");
    sJsKey.push_back("smtp_user_name");
    sJsKey.push_back("smtp_user_passwd");
    sJsKey.push_back("smtp_server");
  
    for (std::vector<std::string>::iterator it = sJsKey.begin(); it != sJsKey.end(); ++it) {
      std::string sVal;
      if (m_SendCnf.Get(*it, sVal) == false) {
        continue;
      }
      mpCnfKV.insert(std::pair<std::string,std::string>(*it, sVal));
    }

    if (mpCnfKV.size() != sJsKey.size()) {
      return false;
    }

    if (mpCnfKV["authtype"] == "PLAIN") {
      m_MailHandle.authtype(jwsmtp::mailer::PLAIN);
    } else {
      //
    }
    m_MailHandle.setsender(mpCnfKV["from"].c_str());
    m_MailHandle.username(mpCnfKV["smtp_user_name"].c_str());
    m_MailHandle.password(mpCnfKV["smtp_user_passwd"].c_str());
    m_MailHandle.setserver(mpCnfKV["smtp_server"].c_str());

    m_Init = true;
    return true; 
  }

  bool LibPushEamil::SendMail(const std::vector<std::string>& vRecver, 
                              const std::string& sSubjuect,
                              const std::string& sContent) {
    if (m_Init == false) {
      if (Init() == false) {
        return false;
      }
    }

    m_MailHandle.clearrecipients();
    m_MailHandle.setsubject(sSubjuect);
    m_MailHandle.setmessage(sContent);
    int iNums = 0;
    //
    std::vector<std::string>::const_iterator itBegin = vRecver.begin();
    for (; itBegin != vRecver.end(); ++itBegin, ++iNums) {
      if (iNums / (RECEIVER_MAX_NUM_ONECE) >= 1 && iNums % RECEIVER_MAX_NUM_ONECE == 0) {
        m_MailHandle.send();
        iNums = 0;
        m_MailHandle.clearrecipients();
      }
      m_MailHandle.addrecipient(itBegin->c_str());
    }

    if (iNums != 0) {
      m_MailHandle.send();
    }
    return true;
  }
//////
}
