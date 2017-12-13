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
    LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s", oOutMsgBody.body().c_str());
    std::string sBody =  oInMsgBody.body();
    std::string sRetBody = sBody + "CmdHello world";
    oOutMsgBody.set_body(sRetBody);
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
    bResult = true;
  }
  return(bResult);
}
