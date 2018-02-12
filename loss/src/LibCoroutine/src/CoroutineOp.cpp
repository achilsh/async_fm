#include <iostream>
#include <unistd.h>
#include "CoroutineOp.hpp"

#ifdef DEBUG_TEST
    #include "util/LibStdCout.h"
#endif

namespace LibCoroutine
{
const int32_t CoroutinerMgr::STACK_SIZE;
const int32_t CoroutinerMgr::MAX_NUM_CO;

bool CoroutinerMgr::CreateCoMgr()
{
    if (m_Init)
    {
        return true;
    }

    this->m_Nco       = 0;                                             // 初始化调度器的当前协程数量
    this->m_Cap       = MAX_NUM_CO;                                    // 初始化调度器的最大协程数量
    this->m_RunningId = -1;

    this->m_vCo.reserve(0);
    this->m_vCo.resize(this->m_Cap, NULL);
    
    m_Init = true;
    return true;
}

bool CoroutinerMgr::DestroyCoMgr()
{
    if (m_Init == false)
    {
        return true;
    }
    m_Init = false;

    int32_t  iIndexCo;
    for (iIndexCo = 0; iIndexCo < this->m_Cap; iIndexCo++)
    {
        Coroutiner* co = this->m_vCo.at(iIndexCo);
        if (co)
        {
            delete co;
        }
    }
    std::vector<Coroutiner*> emptyVCo;
    std::swap(this->m_vCo, emptyVCo);

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
    if (this->m_Nco >= this->m_Cap)
    {
        try {
            int32_t id = this->m_Cap;
            pCo->SetId(id);

            this->m_vCo.resize(this->m_Cap*2, NULL); 
            this->m_vCo.at(this->m_Cap) = pCo;
            this->m_Cap *= 2;
            ++m_Nco;
        }
        catch(const std::exception &ex)
        {
            m_sErr = ex.what();
            return false;
        }

        return true;
    }
    else 
    {
        try {
            int32_t iIndex = 0;
            for (; iIndex < m_Cap; ++iIndex)
            {
                int32_t iCoId = (iIndex + m_Nco) % m_Cap;
                if (m_vCo.at(iCoId) == NULL)
                {
                    m_vCo.at(iCoId) = pCo;
                    ++m_Nco;
                    pCo->SetId(iCoId);
                    return true;
                }
            }
        }
        catch (const std::exception &ex)
        {
            m_sErr = ex.what();
            return false;
        }
    }
    return false;
}

int32_t  CoroutinerMgr::GetCoStatus(const int32_t iCoId)
{
    if (iCoId < 0 || iCoId >= m_Cap)  
    {
        return COROUTINE_DEAD; 
    }

    Coroutiner* pFindCo = m_vCo.at(iCoId);
    if (pFindCo == NULL)
    {
        DEBUG_LOG("co obj null in get status, co id: %d", iCoId);
        return COROUTINE_DEAD;
    }

    return pFindCo->GetCoStatus();
}

bool CoroutinerMgr::YieldCurrentCo()
{
    int32_t iRunId = m_RunningId;
    if (iRunId < 0)
    {
        return false;
    }
    Coroutiner* pRunCo = m_vCo.at(iRunId);
    if (pRunCo == NULL) 
    {
        return false;
    }

    pRunCo->SaveStack( m_Stack + STACK_SIZE );
    pRunCo->SetStatus( COROUTINE_SUSPEND );
    m_RunningId = -1;
    
    ::swapcontext(pRunCo->GetCoCtx(), &m_Main);
    return true;
}

bool CoroutinerMgr::ResumeCo(int32_t iCorId)
{
    DEBUG_LOG("%s(coid: %d)", __FUNCTION__, iCorId);
    
    if (m_RunningId != -1)
    {
        return false;
    }
    if (iCorId < 0 || iCorId >= m_Cap)
    {
        return false;
    }
    Coroutiner* pCoInstance = m_vCo.at(iCorId);
    if (pCoInstance == NULL)
    {
        return true;
    }

    int iCoStatus = pCoInstance->GetStatus();
    if (iCoStatus == COROUTINE_READY)
    {
        this->m_RunningId = iCorId;
        pCoInstance->ResumeCoReadyToRunning(this);
        return true;
    } 
    else if (iCoStatus == COROUTINE_SUSPEND)
    {
        m_RunningId = iCorId;
        pCoInstance->ResumeCoSuspendToRunning(this);
        return true;
    } 
    else 
    {

    }
    return true;
}

Coroutiner* CoroutinerMgr::GetCoByCoId(const int32_t iCoId)
{
    if (iCoId >= m_Cap)
    {
        return NULL;
    }
    if (m_vCo.size() <= iCoId)
    {
        return NULL;
    }

    return m_vCo.at(iCoId);
}

void CoroutinerMgr::DeleteCo(const int32_t iCoId)
{
    if (iCoId >= m_Cap)
    {
        return ;
    }
    if (m_vCo.size() <= iCoId)
    {
        return ;
    }
    
    delete m_vCo.at(iCoId);
    m_vCo.at(iCoId) = NULL;
    --m_Nco;
    m_RunningId = -1;
    DEBUG_LOG("del co, id: %d", iCoId);
}

bool CoroutinerMgr::CoStatusDead(const int32_t iCoId)
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

    DEBUG_LOG("status from ready to running, co id: %d", GetId());
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
    int iCoid           = pMgr->GetRunningCoroutineId();

    Coroutiner* pCo     = pMgr->GetCoByCoId(iCoid);
    if (pCo == NULL)
    {
        DEBUG_LOG("not get any co instance, co id: %d", iCoid);
        return ;
    }

    pCo->CorFunc();
    pCo->AfterFuncWork();
    pMgr->DeleteCo(iCoid);
}

bool Coroutiner::YieldCurCoInCo() 
{
    if ( NULL == m_pCoMgr)
    {
        return false;
    }
    
    int iCoId = m_pCoMgr->GetRunningCoroutineId();
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
