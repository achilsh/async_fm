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

RedisStep::RedisStep(const std::string& sCoName)
    : Step(sCoName), m_pRedisCmd(NULL)
{
    m_pRedisCmd = new loss::RedisCmd();
}

RedisStep::RedisStep(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, 
                     const MsgBody& oReqMsgBody, const std::string& sCoName)
    : Step(stReqMsgShell, oReqMsgHead, oReqMsgBody, sCoName), m_pRedisCmd(NULL)
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

bool RedisStep::ExecuteRedisCmd(OssReply*& preply)
{
    return GetLabor()->ExecuteRedisCmd(preply, this);
}

void RedisStep::SetRedisRetBody(redisReply* replyRedis)
{
    m_redisRetBody = OssReply(replyRedis);
}


//////////
} /* namespace oss */
