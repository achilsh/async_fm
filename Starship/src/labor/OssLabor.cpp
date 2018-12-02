/*******************************************************************************
 * Project:  AsyncServer
 * @file     OssLabor.cpp
 * @brief 
 * @author   
 * @date:    2015年9月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "OssLabor.hpp"
#include "step/Step.hpp"

namespace oss
{

CoroutineLaborMgr::CoroutineLaborMgr() :m_pCoLibMgr(NULL)
{
    if (m_pCoLibMgr == NULL)
    {
        m_pCoLibMgr = new CoroutinerMgr();
    }
    m_mpStepCoId.clear();
}

CoroutineLaborMgr::~CoroutineLaborMgr()
{
    if (m_pCoLibMgr != NULL)
    {
        delete m_pCoLibMgr; m_pCoLibMgr = NULL;
    }

    TypeMultiStepID  emptyMp;
    std::swap(m_mpStepCoId, emptyMp);
}

bool CoroutineLaborMgr::AddNewCoroutine(const Step* pCo)
{
    std::string sEmpty;
    std::swap(m_sErrMsg, sEmpty);
    if (m_pCoLibMgr == NULL)
    {
        return false;
    }
    //调用协程库中的协程管理器来添加协程
    return m_pCoLibMgr->AddNewCoroutine(const_cast<Step*>(pCo));
}

void CoroutineLaborMgr::AddCoAndId(Step* pCo, int64_t iCoId)
{
    std::string sEmpty;
    std::swap(m_sErrMsg, sEmpty);

    if (pCo == NULL)
    {
        return ;
    }

    TypeMultiStepID::iterator it;
    it = m_mpStepCoId.find(pCo);
    if (it == m_mpStepCoId.end())
    {
        m_mpStepCoId.insert(std::pair<Step*, int64_t> (pCo, iCoId));
    }
    else
    {
        it->second = iCoId;
    }
}

void CoroutineLaborMgr::SetLoggerToCoLibMgr(log4cplus::Logger* pLogger)
{
    if (m_pCoLibMgr != NULL)
    {
        m_pCoLibMgr->SetLogger(pLogger);
    }
}

bool CoroutineLaborMgr::ResumeOneCo(Step* pCo, int64_t iCoId)
{
    std::string sEmpty;
    std::swap(m_sErrMsg, sEmpty);
    if (m_pCoLibMgr == NULL || pCo == NULL)
    {
        m_sErrMsg = "m_pCoLibMgr is null or input pCo null";
        return false;
    }

    if (m_mpStepCoId.find(pCo) == m_mpStepCoId.end())
    {
        m_sErrMsg = "not find pCo in m_mpStepCoId";
        return false;
    }
    if (m_mpStepCoId[pCo] != iCoId)
    {
        m_sErrMsg = "m_mpStepCoId has not pair<Step, int64_t>";
        return false;
    }

    if (m_pCoLibMgr->CoStatusDead(iCoId))
    {
        m_mpStepCoId.erase(pCo);
        m_sErrMsg = "to resume co is dead";
        return false;
    }

    //yeild cur coroutine, then iCoId can use cpu
    m_pCoLibMgr->YieldCurrentCo(); //(没有任何携程在运行的)，调用该接口失败，因为当前的运行协程running_id = -1
    return m_pCoLibMgr->ResumeCo(iCoId);
}

bool CoroutineLaborMgr::YeildCoRight(Step* pCo, int64_t iCoId)
{
    std::string sEmpty;
    std::swap(m_sErrMsg, sEmpty);
    if (pCo == NULL || iCoId < 0)
    {
        m_sErrMsg = "input param invaild";
        return false;
    }
    if (m_mpStepCoId.find(pCo) == m_mpStepCoId.end())
    {
        m_sErrMsg = "not find co in m_mpStepCoId";
        return false;
    }

    if (m_mpStepCoId[pCo] != iCoId)
    {
        m_sErrMsg = "m_mpStepCoId not find pair<Step*, coid>";
        return false;
    }

    if (m_pCoLibMgr->CoStatusDead(iCoId))
    {
        m_mpStepCoId.erase(pCo);
        m_sErrMsg = "to resume co is dead";
        return false;
    }
    if (m_pCoLibMgr->GetRunningCoroutineId() != iCoId)
    {
        m_sErrMsg = "co mgr running co id not eq to yield co id";
        return false;
    }
    return m_pCoLibMgr->YieldCurrentCo();
}

void CoroutineLaborMgr::DeleteCoStep(const Step* pCo)
{
    m_mpStepCoId.erase((Step*)pCo);
}

/***************************************************/
OssLabor::OssLabor()
{
}

OssLabor::~OssLabor()
{
    TypeCoMP::iterator it;
    for (it = m_mpCoroutines.begin(); it != m_mpCoroutines.end(); ++it)
    {
        if (it->second)  delete it->second;
    }
    m_mpCoroutines.clear();
}

/*****************************************************/
bool OssLabor::RegisterCoroutine(Step* pStep, double dTimeout)
{
    return true;
}

void OssLabor::DeleteCoroutine(Step* pStep)
{
    return ;
}

bool OssLabor::ResumeCoroutine(Step* pStep)
{
    return true;
}

bool OssLabor::YieldCorountine(Step* pStep)
{
    return true;
}



} /* namespace oss */
