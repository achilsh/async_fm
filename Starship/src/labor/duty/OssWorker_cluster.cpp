#ifndef __ossworker_cluster_cpp__
#define __ossworker_cluster_cpp__

namespace oss
{
#ifdef REDIS_CLUSTER 

void OssWorker::RedisConnectCallback(const redisClusterAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)c->data;
        pWorker->RedisConnect(c, status);
    }
}
void OssWorker::RedisDisconnectCallback(const redisClusterAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)c->data;
        pWorker->RedisDisconnect(c, status);
    }
}

void OssWorker::RedisCmdCallback(redisClusterAsyncContext*c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        OssWorker* pWorker = (OssWorker*)c->data;
        pWorker->RedisCmdResult(c, reply, privdata);
    }
}

bool OssWorker::RedisConnect(const redisClusterAsyncContext *c, int status)
{
    LOG4_TRACE("%s()", __FUNCTION__);

    std::map<redisClusterAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisClusterAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        tagRedisAttr* pRediAttr = attr_iter->second;
        if (status == REDIS_OK)
        {
            pRediAttr->bIsReady = true;
            int iCmdStatus;
            std::list<RedisStep*>::iterator step_iter ;

            for (step_iter = pRediAttr->listWaitData.begin(); step_iter != pRediAttr->listWaitData.end(); )
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
                const char* argv[args_size];
                size_t arglen[args_size];
                
                argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
                arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
                
                std::vector<std::pair<std::string, bool> >::const_iterator c_iter;
                c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
                for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
                {
                    argv[i] = c_iter->first.c_str();
                    arglen[i] = c_iter->first.size();
                }
                
                //update time for this step
                (*step_iter)->SetActiveTime(ev_now(m_loop));
                iCmdStatus = redisClusterAsyncCommandArgv((redisClusterAsyncContext*)c, RedisCmdCallback, NULL, args_size, argv, arglen);
                if (iCmdStatus == REDIS_OK)
                {
                    LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                    pRediAttr->listData.push_back(pRedisStep);
                    pRediAttr->listWaitData.erase(step_iter++);
                }
                else    // 命令执行失败，不再继续执行，等待下一次回调
                {
                    break;
                }
            }
        }
        else
        {
            std::list<RedisStep *>::iterator step_iter;
            for (step_iter = pRediAttr->listWaitData.begin(); step_iter != pRediAttr->listWaitData.end(); )
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                //TODO: 加入协程，再不用去实现callback
                //pRedisStep->Callback(c, status, NULL);
                pRedisStep->SetRespStatus(false);
                //UnRegisterCallback(pRedisStep);
                pRedisStep->SetErrMsg("redis conn fail");
                pRedisStep->SetErrNo(status);
                
                LOG4_ERROR("Redis step conn fail, now awake, RedisStep: %p", pRedisStep); 
                
                pRediAttr->listWaitData.erase(step_iter++);
                //awake正在阻塞的协程
                ResumeCoroutine(pRedisStep);
            }
            pRediAttr->listWaitData.clear();

            delete pRediAttr;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
    }
    return(true);
}

bool OssWorker::RedisDisconnect(const redisClusterAsyncContext *c, int status)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisClusterAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisClusterAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        tagRedisAttr* pRedisAttr = attr_iter->second;
        std::list<RedisStep *>::iterator step_iter ;

        for (step_iter = pRedisAttr->listData.begin(); step_iter != pRedisAttr->listData.end();)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                       c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());

            (*step_iter)->SetActiveTime(ev_now(m_loop));
            RedisStep* pRedisStep = (RedisStep*)(*step_iter);
            pRedisStep->SetRespStatus(false);
            //UnRegisterCallback(pRedisStep);
            pRedisStep->SetErrMsg("redis disconnect");
            pRedisStep->SetErrNo(c->err);

            LOG4_ERROR("Redis step disconn, now awake, RedisStep: %p", pRedisStep); 
            pRedisAttr->listData.erase(step_iter++);

            //awake正在阻塞的协程
            ResumeCoroutine(pRedisStep);
        }
        pRedisAttr->listData.clear();


        for (step_iter = pRedisAttr->listWaitData.begin(); step_iter != pRedisAttr->listWaitData.end(); )
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                       c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());

            (*step_iter)->SetActiveTime(ev_now(m_loop));
            RedisStep* pRedisStep = (RedisStep*)(*step_iter);
            pRedisStep->SetRespStatus(false);
            UnRegisterCallback(pRedisStep);
            pRedisStep->SetErrMsg("redis disconnect");
            pRedisStep->SetErrNo(c->err);

            LOG4_ERROR("Redis step disconn, now awake, RedisStep: %p", pRedisStep); 
            pRedisAttr->listWaitData.erase(step_iter++);

            //awake正在阻塞的协程
            ResumeCoroutine(pRedisStep);
        }
        pRedisAttr->listWaitData.clear();

        delete pRedisAttr;
        attr_iter->second = NULL;

        DelRedisContextAddr(c);
        m_mapRedisAttr.erase(attr_iter);
    }
    return(true);
}

bool OssWorker::RedisCmdResult(redisClusterAsyncContext*c, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisClusterAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisClusterAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        std::list<RedisStep*>::iterator step_iter = attr_iter->second->listData.begin();
        if (NULL == reply)
        {
            TypeMpRedisContextId::iterator identify_iter = m_mapContextIdentify.find(c);
            if (identify_iter != m_mapContextIdentify.end())
            {
                LOG4_ERROR("redis %s error %d: %s", identify_iter->second.c_str(), c->err, c->errstr);
            }
            for ( ; step_iter != attr_iter->second->listData.end(); )
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());

                RedisStep* pRedisStep  = (*step_iter);
                pRedisStep->SetRespStatus(false);
                //UnRegisterCallback(pRedisStep);
                pRedisStep->SetErrMsg(c->errstr);
                pRedisStep->SetErrNo(c->err);

                LOG4_ERROR("Redis cmd ret fail, now awake, RedisStep: %p", pRedisStep); 
                attr_iter->second->listData.erase(step_iter++);

                //awake正在阻塞的协程
                ResumeCoroutine(pRedisStep);
            }
            attr_iter->second->listData.clear();

            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
        else
        {
            if (step_iter != attr_iter->second->listData.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->GetRedisCmd()->ToString().c_str());

                RedisStep* pRedisStep  = (*step_iter);
                pRedisStep->SetRespStatus(true);
                pRedisStep->SetErrMsg(c->errstr);
                pRedisStep->SetErrNo(c->err);

                pRedisStep->SetRedisRetBody((redisReply*)reply);
                //freeReplyObject(reply);

                LOG4_TRACE("Redis cmd ret ok, now awake, RedisStep: %p", pRedisStep); 
                attr_iter->second->listData.erase(step_iter);

                //awake正在阻塞的协程
                ResumeCoroutine(pRedisStep);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }
    }
    return(true);
}

bool OssWorker::RegisterCallback(const redisClusterAsyncContext* pRedisContext, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pRedisStep == NULL)
    {
        return(false);
    }
    //update time out tm
    pRedisStep->SetActiveTime(ev_now(m_loop));

    if (!pRedisStep->IsRegistered()) 
    {
        pRedisStep->SetLabor(this);
        pRedisStep->SetLogger(&m_oLogger);
        pRedisStep->SetRegistered();
    }

    std::map<redisClusterAsyncContext*, tagRedisAttr*>::iterator iter = m_mapRedisAttr.find((redisClusterAsyncContext*)pRedisContext);
    if (iter == m_mapRedisAttr.end())
    {
        LOG4_ERROR("redis attr not exist!");
        return(false);
    }
    else
    {
        LOG4_TRACE("iter->second->bIsReady = %d", iter->second->bIsReady);
        if (iter->second->bIsReady)
        {
            int status;
            size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
            const char* argv[args_size];
            size_t arglen[args_size];
            argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
            arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();

            std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
            for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
            {
                argv[i] = c_iter->first.c_str();
                arglen[i] = c_iter->first.size();
            }
            status = redisClusterAsyncCommandArgv((redisClusterAsyncContext*)pRedisContext, RedisCmdCallback, NULL, args_size, argv, arglen);
            if (status == REDIS_OK)
            {
                LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                iter->second->listData.push_back(pRedisStep);
                return(true);
            }
            else
            {
                LOG4_ERROR("redis status %d!", status);
                return(false);
            }
        }
        else
        {
            LOG4_TRACE("listWaitData.push_back()");
            iter->second->listWaitData.push_back(pRedisStep);
            return(true);
        }
    }
}

bool OssWorker::AddRedisContextAddr(const std::string& strHost, int iPort, redisClusterAsyncContext* ctx)
{
    LOG4_TRACE("%s(%s, %d, 0x%X)", __FUNCTION__, strHost.c_str(), iPort, ctx);
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    TypeMpRedisIdContext::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter == m_mapRedisContext.end())
    {
        m_mapRedisContext.insert(std::pair<std::string, const redisClusterAsyncContext*>(szIdentify, ctx));
        TypeMpRedisContextId::iterator identify_iter = m_mapContextIdentify.find(ctx);
        if (identify_iter == m_mapContextIdentify.end())
        {
            m_mapContextIdentify.insert(std::pair<const redisClusterAsyncContext*, std::string>(ctx, szIdentify));
        }
        else
        {
            identify_iter->second = szIdentify;
        }
        return(true);
    }
    else
    {
        return(false);
    }
}

void OssWorker::DelRedisContextAddr(const redisClusterAsyncContext* ctx)
{
    TypeMpRedisContextId::iterator identify_iter = m_mapContextIdentify.find(ctx);
    if (identify_iter != m_mapContextIdentify.end())
    {
        TypeMpRedisIdContext::iterator ctx_iter = m_mapRedisContext.find(identify_iter->second);
        if (ctx_iter != m_mapRedisContext.end())
        {
            m_mapRedisContext.erase(ctx_iter);
        }
        m_mapContextIdentify.erase(identify_iter);
    }
}
///
bool OssWorker::AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s() redisAsyncConnect(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    std::ostreamstring os;
    os << strHost << ":" << iPort;

    redisClusterAsyncContext *c = redisClusterAsyncConnect(os.str(), HIRCLUSTER_FLAG_NULL);
    if (c->err)
    {
        /* Let *c leak for now... */
        pRedisStep->SetErrMsg(c->errstr);
        pRedisStep->SetErrNo(c->err);

        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }

    if (pRedisStep->IsRegistered() == false)
    {
        pRedisStep->SetLogger(&m_oLogger);
        pRedisStep->SetLabor(this);
        pRedisStep->SetRegistered();
    } 

    c->data = this;
    tagRedisAttr* pRedisAttr = new tagRedisAttr();
    //pRedisAttr->ulSeq = pRedisStep->GetSequence();
    pRedisAttr->listWaitData.push_back(pRedisStep);
    
    m_mapRedisAttr.insert(std::pair<redisClusterAsyncContext*, tagRedisAttr*>(c, pRedisAttr));
    redisClusterLibevAttach(m_loop, c);
    redisClusterAsyncSetConnectCallback(c, RedisConnectCallback);
    redisClusterAsyncSetDisconnectCallback(c, RedisDisconnectCallback);
    AddRedisContextAddr(strHost, iPort, c);
    return(true);
}


#endif
//
}
//
#endif
