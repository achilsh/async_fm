/**
 * @file: CmdHelloWorld.cpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00000001
 * @date: 2017-11-14
 */
#include "CmdHelloWorld.h"
#include "thrift_serialize.h"
#include "hello_test_types.h"


using namespace Test;

int MStatic::m_MTest = 100;

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
    
    MStatic::m_MTest = 1000; 
    LOG4CPLUS_DEBUG_FMT(GetLogger(), "test change num: 8, test: %u", MStatic::m_MTest);

    OneTest tData, retData;
    std::string sData = oInMsgBody.body();
    if (0 != ThrifSerialize<OneTest>::FromString(sData,tData)) { 
      LOG4CPLUS_DEBUG_FMT( GetLogger(), "parse msg body failed");
      msgVal = "parse failed";
    } else {
      msgVal = tData.fOne;
      msgVal += ":cmdHello world";
    }

    retData.__set_fOne(msgVal);
    if (0 != ThrifSerialize<OneTest>::ToString(retData, sData)) {
      sData = "serialize failed";
    }
    oOutMsgBody.set_body(sData);
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
  }
  return(bResult);
}
