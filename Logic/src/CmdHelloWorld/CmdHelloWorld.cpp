/**
 * @file: CmdHelloWorld.cpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00000001
 * @date: 2017-11-14
 */
#include "CmdHelloWorld.h"

#ifdef __cplusplus
extern "C" {
#endif
    oss::Cmd* create()
    {
        oss::Cmd* pCmd = new CmdHelloWorld();
        return(pCmd);
    }
#ifdef __cplusplus
}
#endif

CmdHelloWorld::CmdHelloWorld() {
}

CmdHelloWorld::~CmdHelloWorld() {
}

bool CmdHelloWorld::AnyMessage(
       const oss::tagMsgShell& stMsgShell,
       const MsgHead& oInMsgHead,
       const MsgBody& oInMsgBody) {
  bool bResult = false;
  MsgHead oOutMsgHead;
  MsgBody oOutMsgBody;
  oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
  oOutMsgHead.set_seq(oInMsgHead.seq());
  if (GetCmd() == (int)oInMsgHead.cmd())
  {
    std::string msgVal;
    LOG4CPLUS_DEBUG_FMT(GetLogger(),"logic recv msg body: %s", oInMsgBody.DebugString().c_str());
    loss::CJsonObject tData, retData;
    
    if (false == tData.Parse(oInMsgBody.body())) {
      LOG4CPLUS_DEBUG_FMT( GetLogger(), "parse msg body failed");
      msgVal = "parse failed";
    } else {
      if (tData.Get("key", msgVal)) {
        LOG4CPLUS_DEBUG_FMT(GetLogger(),"logic recv strdata field");
      } else {
        msgVal = "logic recv not set strdata field";
      }

      LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s", msgVal.c_str());
      msgVal += ":CmdHello world";
      bResult = true;
    } 

    retData.Add("key",msgVal);
    oOutMsgBody.set_body(retData.ToString());

    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
  }
  return(bResult);
}
