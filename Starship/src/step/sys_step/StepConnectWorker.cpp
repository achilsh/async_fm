/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepConnectWorker.cpp
 * @brief 
 * @author   
 * @date:    2015年8月14日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepConnectWorker.hpp"

namespace oss
{

StepConnectWorker::StepConnectWorker(const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
    : m_iTimeoutNum(0),
      m_stMsgShell(stMsgShell), m_oConnectMsgHead(oInMsgHead), m_oConnectMsgBody(oInMsgBody),
      pStepTellWorker(NULL)
{
}

StepConnectWorker::~StepConnectWorker()
{
}

E_CMD_STATUS StepConnectWorker::Emit(
                int iErrno,
                const std::string& strErrMsg,
                const std::string& strErrShow)
{
    m_oConnectMsgHead.set_seq(GetSequence());
    SendTo(m_stMsgShell, m_oConnectMsgHead, m_oConnectMsgBody);
    return(STATUS_CMD_RUNNING);
}

E_CMD_STATUS StepConnectWorker::Callback(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
    OrdinaryResponse oRes;
    if (oRes.ParseFromString(oInMsgBody.body()))
    {
        if (oRes.err_no() == ERR_OK)
        {
            for (int i = 0; i < 3; ++i)
            {
                pStepTellWorker = new StepTellWorker(m_stMsgShell);
                if (pStepTellWorker == NULL)
                {
                    LOG4_ERROR("error %d: new StepTellWorker() error!", ERR_NEW);
                    return(STATUS_CMD_FAULT);
                }

                if (RegisterCallback(pStepTellWorker))
                {
                    pStepTellWorker->Emit(ERR_OK);
                    return(STATUS_CMD_COMPLETED);
                }
                else
                {
                    delete pStepTellWorker;
                }
            }
            return(STATUS_CMD_FAULT);
        }
        else
        {
            LOG4_ERROR("error %d: %s!", oRes.err_no(), oRes.err_msg().c_str());
            return(STATUS_CMD_FAULT);
        }
    }
    else
    {
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
        return(STATUS_CMD_FAULT);
    }
}

E_CMD_STATUS StepConnectWorker::Timeout()
{
    ++m_iTimeoutNum;
    if (m_iTimeoutNum <= 3)
    {
        return(Emit(ERR_OK));
    }
    else
    {
        LOG4_ERROR("timeout %d times!", m_iTimeoutNum);
        return(STATUS_CMD_FAULT);
    }
}

} /* namespace oss */
