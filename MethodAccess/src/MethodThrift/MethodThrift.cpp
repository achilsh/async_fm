#include "MethodThrift.h"
#include "demosvr.h"
#include "StepTestQuery.h"

using namespace Test;
using namespace apache::thrift;

namespace im 
{

MethodThrift::MethodThrift()
{
}

MethodThrift::~MethodThrift() 
{
}

bool MethodThrift::AnyMessage(
    const tagMsgShell& stMsgShell,
    const Thrift2Pb& oInThriftMsg,
    uint32_t uiPacketLen, const uint8_t *pPacketBuf)
{
    std::string sMethodName = oInThriftMsg.thrift_interface_name();
    int iSeq = oInThriftMsg.thrift_seq();
    Thrift2Pb outThriftMsg;
    outThriftMsg.Clear();

    demosvr_pingping_result pingping_result;
    pingping_result.__isset.success = true;
    
    if (sMethodName != GetMethodName()) 
    {
        LOG4_ERROR("recv interfae name: %s not config", sMethodName.c_str());
        pingping_result.success.retcode = -1;
        this->SendAck(stMsgShell, outThriftMsg, pingping_result);
        return false;
    }

    SetThriftSeq(iSeq, outThriftMsg);
    SetThriftInterfaceName(sMethodName, outThriftMsg);
    LOG4_TRACE("req interface: %s, seq: %u",sMethodName.c_str(),iSeq);

    demosvr_pingping_args pingping_args;
    
    if (false == GetThriftParams(pingping_args, oInThriftMsg, uiPacketLen, pPacketBuf))
    {
        LOG4_ERROR("get params fail, method: %s", sMethodName.c_str());
        pingping_result.success.retcode = -2;

        this->SendAck(stMsgShell, outThriftMsg, pingping_result);
        return  false;
    }

    LOG4_TRACE("each req, seq: %u, method: %s", iSeq, sMethodName.c_str());

#if 0
    pStepTQry = new StepTestQuery(stMsgShell, pingping_args,iSeq, sMethodName);
    if (pStepTQry != NULL)
    {
        if (RegisterCallback(pStepTQry))
        {
            if (oss::STATUS_CMD_RUNNING == pStepTQry->Emit(0))
            {
                return true;
            }
            else 
            {
                DeleteCallback(pStepTQry);
                pingping_result.success.retcode = -2;
                this->SendAck(stMsgShell, outThriftMsg, pingping_result);
                return false;
            }
        }
        else 
        {
            delete pStepTQry;
            pStepTQry = NULL;
            pingping_result.success.retcode = -3;
            this->SendAck(stMsgShell, outThriftMsg, pingping_result);
            return false;
        }
    }
#endif

    //采用协程模式
    pStepTQry = new StepTestQuery(stMsgShell, 
                                  pingping_args,iSeq, 
                                  sMethodName, "thrift_step");
    if (pStepTQry != NULL)
    {
        if (RegisterCoroutine(pStepTQry) == false)
        {
            LOG4_ERROR("start thrift co fail, thrift step: %p", pStepTQry);
            DeleteCoroutine(pStepTQry);
            delete pStepTQry;

            pingping_result.success.retcode = -2;
            this->SendAck(stMsgShell, outThriftMsg, pingping_result);
            return false;
        }
        return true;
    }

    pingping_result.success.retcode = 0;
    pingping_result.success.a = pingping_args.pi.a + 1 ;
    pingping_result.success.b = pingping_args.pi.b;

    this->SendAck(stMsgShell, outThriftMsg, pingping_result);
    LOG4_TRACE("send succ response to client, interface: %s", sMethodName.c_str());
    return true;
}


//
}
