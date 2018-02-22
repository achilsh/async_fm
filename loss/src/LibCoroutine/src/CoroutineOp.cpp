#include <iostream>
#include <unistd.h>
#include "CoroutineOp.hpp"

#include "util/LibStdCout.h"

namespace LibCoroutine
{
const int32_t CoroutinerMgr::STACK_SIZE;
uint32_t CoroutinerMgr::m_stcCoYieldPoint = 100;

//仅仅本文件内部打印用，建议不要扩展到其他目录或者文件上
#define LOG4_FATAL(args...) LOG4CPLUS_FATAL_FMT(GetLogger(), ##args)
#define LOG4_ERROR(args...) LOG4CPLUS_ERROR_FMT(GetLogger(), ##args)
#define LOG4_WARN(args...) LOG4CPLUS_WARN_FMT(GetLogger(), ##args)
#define LOG4_INFO(args...) LOG4CPLUS_INFO_FMT(GetLogger(), ##args)
#define LOG4_DEBUG(args...) LOG4CPLUS_DEBUG_FMT(GetLogger(), ##args)
#define LOG4_TRACE(args...) LOG4CPLUS_TRACE_FMT(GetLogger(), ##args)

bool CoroutinerMgr::CreateCoMgr()
{
    if (m_Init)
    {
        return true;
    }

    m_mpCo.clear();
    this->m_RunningId = -1;
    m_Init = true;

    m_pLogger = NULL;  
    SetGLobalMechineID(::time(NULL));
    return true;
}

bool CoroutinerMgr::DestroyCoMgr()
{
    if (m_Init == false)
    {
        return true;
    }
    m_Init = false;

    IterTypeMAPCo it ;
    for (it = m_mpCo.begin(); it != m_mpCo.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
            it->second = NULL;
        }
    }

    TypeMAPCo emptyCo;
    std::swap(emptyCo, m_mpCo);
    return true;
}

CoroutinerMgr::CoroutinerMgr():m_Init(false)
{
    CreateCoMgr();
}

CoroutinerMgr::~CoroutinerMgr()
{
    DestroyCoMgr();
}

bool  CoroutinerMgr::AddNewCoroutine(Coroutiner *pCo) 
{
    if (pCo == NULL)
    {
        return false;
    }

    pCo->SetMgr(this);

    int64_t iLCoIdNew =  GLOBAL_ID;
    m_mpCo.insert(std::make_pair(iLCoIdNew, pCo));

    pCo->SetId(iLCoIdNew);
    return true;
}

int32_t  CoroutinerMgr::GetCoStatus(const int64_t iCoId)
{
    if (iCoId < 0)
    {
        return COROUTINE_DEAD; 
    }
    
    IterTypeMAPCo it =  m_mpCo.find(iCoId);
    if (it == m_mpCo.end())
    {
        DEBUG_LOG("co obj null in get status, co id: %ld", iCoId);
        return COROUTINE_DEAD;
    }

    Coroutiner* pFindCo = it->second;
    if (pFindCo == NULL)
    {
        DEBUG_LOG("co obj null in get status, co id: %ld", iCoId);
        return COROUTINE_DEAD;
    }

    return pFindCo->GetCoStatus();
}

bool CoroutinerMgr::YieldCurrentCo()
{
    int64_t iRunId = m_RunningId;
    if (iRunId < 0)
    {
        return false;
    }
   
    IterTypeMAPCo it = m_mpCo.find(iRunId);
    if (it == m_mpCo.end() || it->second == NULL)
    {
        return false;
    }
    it->second->SetYieldCheckPoint(++m_stcCoYieldPoint);
    it->second->SaveStack( m_Stack + STACK_SIZE );
    it->second->SetStatus( COROUTINE_SUSPEND );
    m_RunningId = -1;
    
    LOG4_TRACE("co id: %ld, node: %p, begin yeild point: %u", 
               iRunId, it->second, m_stcCoYieldPoint);
    ::swapcontext(it->second->GetCoCtx(), &m_Main);
    return true;
}

bool CoroutinerMgr::ResumeCo(int64_t iCorId)
{
    DEBUG_LOG("%s(coid: %ld)", __FUNCTION__, iCorId);
    
    if (m_RunningId != -1)
    {
        return false;
    }
    if (iCorId < 0)
    {
        return false;
    }

    IterTypeMAPCo it = m_mpCo.find(iCorId);
    if (it == m_mpCo.end() || it->second == NULL)
    {
        return false;
    }

    int iCoStatus = it->second->GetStatus();
    if (iCoStatus == COROUTINE_READY)
    {
        this->m_RunningId = iCorId;
        it->second->ResumeCoReadyToRunning(this);
        return true;
    } 
    else if (iCoStatus == COROUTINE_SUSPEND)
    {
        m_RunningId = iCorId;
        it->second->ResumeCoSuspendToRunning(this);
        return true;
    } 
    else 
    {

    }
    return true;
}

Coroutiner* CoroutinerMgr::GetCoByCoId(const int64_t iCoId)
{
    IterTypeMAPCo it = m_mpCo.find(iCoId);
    if (it == m_mpCo.end())
    {
        return NULL;
    }
    return it->second;
}

//删除当前协程,设置当前正在运行的协程号为-1
void CoroutinerMgr::DeleteCo(const int64_t iCoId)
{
    IterTypeMAPCo it = m_mpCo.find(iCoId);
    if (it != m_mpCo.end())
    {
        if (it->second)
        {
            //delete it->second; 释放资源由外部接口去完成
            it->second = NULL;
        }
        m_mpCo.erase(it);

        m_RunningId = -1;
        DEBUG_LOG("del co, co_id: %ld", iCoId);
    }
}

bool CoroutinerMgr::CoStatusDead(const int64_t iCoId)
{
    try {
        if (GetCoStatus(iCoId) == COROUTINE_DEAD)
        {
            return true;
        }
    }
    catch (const std::exception &ex)
    {
        m_sErr = ex.what();
    }

    return false;
}

/**************************************************
* 协程实现
*
**************************************************/

bool Coroutiner::CreateCo()
{
    this->m_Cap    = 0;
    this->m_Size   = 0;
    this->m_Status = COROUTINE_READY;
    this->m_pStack = NULL;
    this->m_pCoMgr = NULL;
    this->m_uiYieldCheckPoint = 0;
    this->m_pcoLogger = NULL;
    
    this->m_iErrNo = 0;
    this->m_sErrMsg.clear();

    return true;
}

int Coroutiner::GetCoStatus() 
{
    return m_Status;
}


void Coroutiner::FreeRes()
{
    delete [] m_pStack;
    m_pStack = NULL;
}

Coroutiner::Coroutiner()
{
    CreateCo();
}

Coroutiner::~Coroutiner()
{
    FreeRes();
}

bool  Coroutiner::SaveStack(char *pTop)
{
    char dummy = 0;
    ClearErrMsg();
    
    if ((pTop - &dummy) > CoroutinerMgr::STACK_SIZE)
    {
        m_sErrMsg = "save stack fail";
        return false;
    }

    if (m_Cap < (pTop - &dummy))
    {
        delete m_pStack;
        m_Cap = (ptrdiff_t)(pTop - &dummy);
        m_pStack = new char[m_Cap];
    }
    m_Size = pTop - &dummy;
    memcpy(m_pStack, &dummy, m_Size);
    return true;
}

bool Coroutiner::ResumeCoReadyToRunning(CoroutinerMgr* pMgr)
{
    ::getcontext(&m_Ctx);
    m_Ctx.uc_stack.ss_sp   = pMgr->GetStack();
    m_Ctx.uc_stack.ss_size = CoroutinerMgr::STACK_SIZE;
    m_Ctx.uc_link          = pMgr->GetMainCtx();
    m_Status               = COROUTINE_RUNNING;

    uintptr_t ptr          = (uintptr_t)pMgr;

    DEBUG_LOG("status from ready to running, co id: %ld", GetId());
    makecontext( &m_Ctx, 
                (void (*)(void)) Coroutiner::GloblCorFunc, 2,
                 (uint32_t)ptr, (uint32_t)(ptr>>32)
               );

    swapcontext(pMgr->GetMainCtx(), &m_Ctx); 
    return true;
}

bool Coroutiner::ResumeCoSuspendToRunning(CoroutinerMgr* pMgr)
{
    ::memcpy(pMgr->GetStack() + CoroutinerMgr::STACK_SIZE - m_Size,
             m_pStack, m_Size);
    
    m_Status = COROUTINE_RUNNING;
   
    ::swapcontext(pMgr->GetMainCtx(), &m_Ctx);
    return true;
}

void Coroutiner::GloblCorFunc(uint32_t low32, uint32_t hi32)
{
    uintptr_t ptr       = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    CoroutinerMgr *pMgr = (CoroutinerMgr*)ptr;
    int64_t iCoid           = pMgr->GetRunningCoroutineId();

    Coroutiner* pCo     = pMgr->GetCoByCoId(iCoid);
    if (pCo == NULL)
    {
        DEBUG_LOG("not get any co instance, co id: %ld", iCoid);
        return ;
    }

    pCo->CorFunc();
    pCo->AfterFuncWork(); //由此处来释放协程自身的资源,由step内部接口来释放
    pMgr->DeleteCo(iCoid);
}

bool Coroutiner::YieldCurCoInCo() 
{
    if ( NULL == m_pCoMgr)
    {
        return false;
    }
    
    int64_t iCoId = m_pCoMgr->GetRunningCoroutineId();
    if (iCoId < 0)
    {
        ClearErrMsg();
        m_sErrMsg = "get runing co id fail";
        return false;
    }
    return  m_pCoMgr->YieldCurrentCo();
}

//
}
