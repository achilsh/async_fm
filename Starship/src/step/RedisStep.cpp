/*******************************************************************************
 * Project:  AsyncServer
 * @file     RedisStep.cpp
 * @brief 
 * @author   
 * @date:    2015年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#include "RedisStep.hpp"

namespace oss
{

RedisStep::RedisStep(Step* pNextStep)
    : Step(pNextStep), m_pRedisCmd(NULL)
{
    m_pRedisCmd = new loss::RedisCmd();
}

RedisStep::RedisStep(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : Step(stReqMsgShell, oReqMsgHead, oReqMsgBody, pNextStep), m_pRedisCmd(NULL)
{
    m_pRedisCmd = new loss::RedisCmd();
}

RedisStep::~RedisStep()
{
    if (m_pRedisCmd != NULL)
    {
        delete m_pRedisCmd;
        m_pRedisCmd = NULL;
    }
}

} /* namespace oss */
