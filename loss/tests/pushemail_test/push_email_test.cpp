#include "lib_push_email.h"
#include <vector>
#include <string>
#include <iostream>

using namespace LIB_PUSH_EMAIL;

class TestPushEmail {
  public:
   TestPushEmail();
   virtual ~TestPushEmail();
   bool Run();
};

TestPushEmail::TestPushEmail() {
}

TestPushEmail::~TestPushEmail() {
}

bool TestPushEmail::Run() {
  // {"from": "", "authtype": "","smtp_user_name":"", "smtp_user_passwd":"",
  //"smtp_server": ""}
  loss::CJsonObject cnf;
  cnf.Add("from", "push_email_test@126.com");
  cnf.Add("authtype", "PLAIN");
  cnf.Add("smtp_user_name", "push_email_test@126.com");
  cnf.Add("smtp_user_passwd", "test123");
  cnf.Add("smtp_server", "smtp.126.com");

  std::string sSubject = "上帝的福音";
  std::string sMsg = "世界末日，到家躲起来吧";

  std::vector<std::string> vToEmail;
  vToEmail.push_back("hwshtongxin@126.com");
  vToEmail.push_back("push_email_test@126.com");
  
  LibPushEamil m_Pusher(cnf);
  if (false == m_Pusher.SendMail(vToEmail, sSubject, sMsg )) {
    std::cout << "send email fail" << std::endl;
    return false;
  }
  std::cout << m_Pusher.GetEmailResp() << std::endl;
  return true;
}

int main() {

  TestPushEmail Test;
  Test.Run();
  return 0;
}




