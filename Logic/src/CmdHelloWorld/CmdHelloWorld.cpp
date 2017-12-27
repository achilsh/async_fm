/**
 * @file: CmdHelloWorld.cpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00000001
 * @date: 2017-11-14
 */
#include "CmdHelloWorld.h"
#include "hello_test.pb.h"

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
    hellotest tData, retData;
    std::string msgVal;
    
    if (false == tData.ParseFromString(oOutMsgBody.body())) {
     LOG4CPLUS_DEBUG_FMT( GetLogger(), "parse msg body failed");
     msgVal = "parse failed";
    } else {
      msgVal = tData.strdata();
      LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s", msgVal.c_str());
      msgVal = msgVal + ":CmdHello world";
      bResult = true;
    } 

    retData.set_strdata(msgVal);
    oOutMsgBody.set_body(retData.SerializeAsString());
    
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
  }
  return(bResult);
}
