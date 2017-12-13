/*******************************************************************************
 * Project:  InterfaceServer
 * @file     ModuleHttp.cpp
 * @brief 
 * @author   
 * @date:    2015��11��19��
 * @note
 * Modify history:
 ******************************************************************************/
#include "ModuleHttp.h"
#include "StepTestQuery.h"
#include "OssDefine.hpp"

#ifdef __cplusplus
extern "C" {
#endif
oss::Cmd* create()
{
    oss::Cmd* pCmd = new im::ModuleHttp();
    return(pCmd);
}
#ifdef __cplusplus
}
#endif

namespace im
{

ModuleHttp::ModuleHttp(): pStepTQry(NULL)
{
}

ModuleHttp::~ModuleHttp()
{
}

bool ModuleHttp::AnyMessage(
                const oss::tagMsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg)
{
    std::string sErr;
    m_tagMsgShell = stMsgShell;
    m_oInHttpMsg = oInHttpMsg; 
    LOG4_INFO("method: %d ", oInHttpMsg.method());
    if (HTTP_POST != oInHttpMsg.method()) {
      LOG4_INFO("req not post method");
      sErr = "req not post method";
      SendAck(sErr);
      return false;
    }
    //
    if (HTTP_REQUEST != oInHttpMsg.type()) {
      LOG4_ERROR("http not req tye");
      sErr = "http not req type";
      SendAck(sErr);
      return false;
    }

    loss::CJsonObject JsonParam;
    if (false == JsonParam.Parse(oInHttpMsg.body())) {
      LOG4_ERROR("parse http body failed");
      sErr = "parse http body failed";
      SendAck(sErr);
      return false;
    }

    std::string sTestVal;
    if (false == JsonParam.Get("Name", sTestVal)) {
      LOG4_ERROR("Get Name from json failed");
      sErr = "Get Name from json failed";
      SendAck(sErr);
      return false;
    }
    //
    pStepTQry =  new StepTestQuery(m_tagMsgShell, oInHttpMsg, sTestVal);
    if (false == RegisterCallback(pStepTQry)) {
      delete pStepTQry;
      pStepTQry = NULL;
      SendAck("register failed");
      return false;
    }
    if (oss::STATUS_CMD_RUNNING != pStepTQry->Emit(0)) {
      DeleteCallback(pStepTQry);
      SendAck("run step query faild");
      return false;
    }
    return true;
}

void ModuleHttp::SendAck(const std::string sErr) {
  if (sErr.empty()) {
    return ;
  }

  HttpMsg oOutHttpMsg;
  oOutHttpMsg.set_type(HTTP_RESPONSE);
  oOutHttpMsg.set_status_code(200);
  oOutHttpMsg.set_http_major(m_oInHttpMsg.http_major());
  oOutHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());
  
  loss::CJsonObject retJson;
  retJson.Add("code", sErr);
  oOutHttpMsg.set_body(retJson.ToString());
  GetLabor()->SendTo(m_tagMsgShell,oOutHttpMsg);
  return ;
}

} /* namespace im */
