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
#include "TestSingleton.h"

//test global static var

namespace im
{
  
TestTimer::TestTimer(const std::string& sTimerId, ev_tstamp tm,int x)
    : oss::CTimer(sTimerId,tm), m_X(x) {
    //
}

TestTimer::~TestTimer() {
}

E_CMD_STATUS TestTimer::TimerDoWork() {
  LOG4_TRACE("this is test doing for timer,x: %u", m_X);
  return oss::STATUS_CMD_RUNNING;
}

//--------------------------------//
ModuleHttp::ModuleHttp(): pStepTQry(NULL),pTimer(NULL)
{
    //����http post req times;
    m_iPostTimes = 1;
}

ModuleHttp::~ModuleHttp()
{
  if (pTimer) {
    pTimer->StopTimer();
    pTimer = NULL;
  }
}

bool ModuleHttp::Init() {
    /***
  if (pTimer == NULL) {
    time_t tmNow = time(NULL);
    std::stringstream ios;
    ios << tmNow;
    m_sTimerId = ios.str();
    pTimer = new TestTimer(m_sTimerId,0.05, 2);
    pTimer->StartTimer(GetLabor());
  }
  ***/
  return true;
}

bool ModuleHttp::AnyMessage(
                const oss::tagMsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg)
{
    std::string sErr;
    LOG4_INFO("method: %d,======> ", oInHttpMsg.method());
    //LOG4_ALARM_REPORT("add alarm test: %u", 1000);

    if (HTTP_POST != oInHttpMsg.method()) {
        LOG4_INFO("req not post method");
        LOG4_ALARM_REPORT("recv http method not: %u, check it", oInHttpMsg.method());
        sErr = "req not post method";
        SendAck(sErr);
        return false;
    }
    //
    if (HTTP_REQUEST != oInHttpMsg.type()) {
        LOG4_ERROR("http not req tye");
        sErr = "http not req type";
        SendAck(sErr);
        LOG4_ALARM_REPORT("recv http type not: %u, check it", HTTP_REQUEST);
        return false;
    }
    /***
    //test local static var
    static int static_test = 12;

    static_test = 100;
    gloabl_static_test = 100;
    TestGlobalStatic = 100;

    LOG4_INFO("local static test: %u, global static test: %u, test global static: %u",
    static_test, gloabl_static_test, TestGlobalStatic);
     ***/ 
    std::string sSingletonVal = "this is singleton test >> 113";
    SINGLETEST->SetVal(sSingletonVal);
    LOG4_INFO("singleton test: %s", SINGLETEST->GetVal().c_str());

    TestGG gg;
    gg.SetX(200);
    LOG4_INFO("singleton test: %d", gg.GetX());


    loss::CJsonObject JsonParam;
    if (false == JsonParam.Parse(oInHttpMsg.body())) 
    {
        LOG4_ERROR("parse http body failed");
        sErr = "parse http body failed";
        SendAck(sErr);
        LOG4_ALARM_REPORT("parse recv http msg body json fail");
        return false;
    }

    std::string sTestVal;
    if (false == JsonParam.Get("Name", sTestVal)) {
        LOG4_ERROR("Get Name from json failed");
        sErr = "Get Name from json failed";
        SendAck(sErr);
        LOG4_ALARM_REPORT("recv body has not Name item");
        return false;
    }

    //
    pStepTQry = new StepTestQuery(stMsgShell, oInHttpMsg, "http_co", sTestVal, m_iPostTimes--);
    if (RegisterCoroutine(pStepTQry) == false)
    {
        DeleteCoroutine(pStepTQry);
        SendAck("register failed");
        LOG4_ALARM_REPORT("reg new step fail");

        return false;
    }

    LOG4_TRACE("test main run after co yield");
    return true;
}

void ModuleHttp::SendAck(const std::string sErr) {
  if (sErr.empty()) {
    return ;
  }

  Module::SendAck(sErr);
}

} /* namespace im */
