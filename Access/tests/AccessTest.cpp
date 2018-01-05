/**
 * @file: AccessTest.cpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x01
 * @date: 2018-01-02
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>


#include "protocol/msg.pb.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "util/json/CJsonObject.hpp"

#include <iostream>
#include <string>
using namespace std;

#define TLOG4_FATAL(args...) LOG4CPLUS_FATAL_FMT(GetTLog(), ##args)
#define TLOG4_ERROR(args...) LOG4CPLUS_ERROR_FMT(GetTLog(), ##args)
#define TLOG4_WARN(args...)  LOG4CPLUS_WARN_FMT(GetTLog(), ##args)
#define TLOG4_INFO(args...)  LOG4CPLUS_INFO_FMT(GetTLog(), ##args)
#define TLOG4_DEBUG(args...) LOG4CPLUS_DEBUG_FMT(GetTLog(), ##args)
#define TLOG4_TRACE(args...) LOG4CPLUS_TRACE_FMT(GetTLog(), ##args)


struct SndMsg {
  int iCmd;
  std::string sToSendMsg;
};
struct RcvMsg {
  int iCmd;
  std::string sRecvMsg;
};

//
class TestClient {
  public:
   TestClient(const std::string& sIp, unsigned int uiPort);
   virtual ~TestClient(); 
   bool Init();
   //
   bool PackMsg(const SndMsg& strMsg, std::string& strPack);
   bool DoSendMsg(const SndMsg& strSend);
   
   bool DoRecvMsg(RcvMsg& strRecv);
   bool UnPackMsg(const std::string& strUnPack, RcvMsg& strMsg);
  
   log4cplus::Logger GetTLog() { return m_oLogger; }
   bool CreateSock();
  private:
   std::string m_sIp;
   unsigned int m_uiPort;
   int m_iFd;
   log4cplus::Logger m_oLogger;
};

TestClient::TestClient(const std::string& sIp, unsigned int uiPort) 
  :m_sIp(sIp), m_uiPort(uiPort),m_iFd(0) {
    Init();
}

TestClient::~TestClient() {
  if (m_iFd > 0) { ::close(m_iFd); m_iFd = 0; }
}

bool TestClient::Init() {
  char szLogName[256] = {0};
  snprintf(szLogName, sizeof(szLogName), "%s/access_test.log", ".");
  std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
  log4cplus::initialize();
  log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
          szLogName, 500000, 5));
  append->setName(szLogName);
  std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
  append->setLayout(layout);
  m_oLogger = log4cplus::Logger::getInstance(szLogName);
  m_oLogger.addAppender(append);
  m_oLogger.setLogLevel(0);
}

bool TestClient::DoSendMsg(const SndMsg& strSend) {
  TLOG4_INFO("call: %s", __FUNCTION__);
  if (m_iFd <= 0) {
    if (CreateSock() == false) {
      return false;
    }
    TLOG4_INFO("create conn fd: %u", m_iFd);
  }

  std::string sSendMsg;
  if (false == PackMsg(strSend,sSendMsg)) {
    TLOG4_ERROR("pack send msg failed");
    return false;
  }

  int iSendLen = sSendMsg.size();
  const void* ptr = sSendMsg.c_str();
  int iRet = ::send(m_iFd,ptr,iSendLen, 0);
  TLOG4_INFO("send ret: %u, send msg len: %u",iRet, iSendLen);
  if (iRet < 0) {
    TLOG4_ERROR("send fail, errmsg: %s, ret: %u", strerror(errno), iRet);
    return false;
  }

  TLOG4_INFO("send ok");
  return true;
}

bool TestClient::DoRecvMsg(RcvMsg& strRecv) {
  if (m_iFd <= 0) {
    return false;
  }
  std::cout << __FUNCTION__ << std::endl;
  int iRevLen = 0;
  char buf[1024] = {0};
  iRevLen = ::recv(m_iFd, buf, sizeof(buf),0);
  if (iRevLen <= 0) {
    TLOG4_ERROR("recv buf from access resp failed");
    return false;
  }
  TLOG4_INFO("recv msg len: %u", iRevLen);
  
  std::string sMsgData;
  sMsgData.assign(buf, iRevLen);

  if (false == UnPackMsg(sMsgData, strRecv)) {
    TLOG4_ERROR("unpack recv msg failed");
    return false;
  }
  return true;
}

bool TestClient::CreateSock() {
  if (m_iFd >0) {
    return true;
  }
  m_iFd = ::socket(AF_INET,SOCK_STREAM,0);
  if (m_iFd < 0) {
    std::cout << "create socket fail, errmsg: %s" << strerror(errno) <<  std::endl;
    return false;
  }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port=htons(m_uiPort);
  addr.sin_addr.s_addr = inet_addr(m_sIp.c_str());
  int iRet = ::connect(m_iFd,(sockaddr*)&addr,sizeof(addr));
  if (iRet < 0) {
    TLOG4_ERROR("connect srv fail, err msg: %s", strerror(errno));
    return false;
  }
  return true;
}

bool TestClient::PackMsg(const SndMsg& strMsg, std::string& strPack) {
  MsgHead oMsgHead;
  static int ii = 100;
  ++ii;
  loss::CJsonObject tData;
  tData.Add("key",strMsg.sToSendMsg);
  MsgBody oMsgBody;
  oMsgBody.set_body(tData.ToString());

  oMsgHead.set_cmd(strMsg.iCmd);
  oMsgHead.set_seq(ii);
  oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
  
  strPack.append(oMsgHead.SerializeAsString());
  strPack.append(oMsgBody.SerializeAsString());
  TLOG4_INFO("call: %s", __FUNCTION__);
  return true;
}

bool TestClient::UnPackMsg(const std::string& strUnPack, RcvMsg& strMsg) {
  MsgHead iMsgHead;
  iMsgHead.set_cmd(0);
  iMsgHead.set_seq(0);
  iMsgHead.set_msgbody_len(0);
  
  int iMsgheadLen = iMsgHead.ByteSize();
  if (strUnPack.size() < iMsgheadLen) {
    TLOG4_ERROR("recv data less head size"); 
    return false;
  }

  bool bRet = iMsgHead.ParseFromArray(strUnPack.c_str(),iMsgheadLen);
  if (bRet == false) {
    TLOG4_ERROR("parse msg head fail, head len: %u", iMsgheadLen);
    return false;
  }
  if (iMsgHead.has_cmd() && iMsgHead.has_msgbody_len()) {
    TLOG4_INFO("recv cmd: %u, body len: %u in head, head size: %u",
               iMsgHead.cmd(), iMsgHead.msgbody_len(), iMsgheadLen);        
  }

  strMsg.iCmd = iMsgHead.cmd();
  int msgbodylen = iMsgHead.msgbody_len();
  if (strUnPack.size() >= msgbodylen + iMsgheadLen) {
    MsgBody oMsgBody;
    loss::CJsonObject tData;
    bRet = oMsgBody.ParseFromArray(strUnPack.c_str() + iMsgheadLen, msgbodylen);
    
    tData.Parse(oMsgBody.body());
    strMsg.sRecvMsg = tData("key");
    TLOG4_INFO("recv response msg:[ %s ], cmd: [%u]", strMsg.sRecvMsg.c_str(), strMsg.iCmd);
  }
  return true;
}

int main() {
  SndMsg sMsg;
  sMsg.iCmd = 101;
  sMsg.sToSendMsg = "=> client msg cross by access to logic <=";

  std::string sIp = "192.168.1.106";
  unsigned int uiPort = 15000;
  TestClient tCli(sIp, uiPort);
  if (false == tCli.DoSendMsg(sMsg)) {
    //TLOG4_ERROR("send msg failed to ip: %s, port: %u", sIp.c_str(), uiPort);
    return false;
  }

  RcvMsg rMsg;
  if (false == tCli.DoRecvMsg(rMsg)) {
    //TLOG4_ERROR("recv msg from access failed");
    return false;
  }
  //TLOG4_INFO("recv cmd: %u, content: %s",rMsg.iCmd, rMsg.sRecvMsg.c_str());
  return 0;
}
