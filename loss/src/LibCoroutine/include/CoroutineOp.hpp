#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>

#if __APPLE__ && __MACH__
    #include <sys/ucontext.h>
#else
    #include <ucontext.h>
#endif

namespace LibCoroutine 
{

#define COROUTINE_DEAD    (0)
#define COROUTINE_READY   (1)
#define COROUTINE_RUNNING (2)
#define COROUTINE_SUSPEND (3)

    class Coroutiner;

    /*----------------------
     * 协程管理类的定义
     * ----------------------*/
    class CoroutinerMgr
    {
     public:
      CoroutinerMgr();
      virtual ~CoroutinerMgr();

      /**
       * @brief: AddNewCoroutine 
       *  向协程管理器添加新创建的协程实例
       *  客户端直接调用
       * @param pCo, 需要添加的
       *
       * @return: true 添加成功，false 添加失败
       */
      bool  AddNewCoroutine(Coroutiner *pCo);

      /**
       * @brief: GetRunningCoroutineId 
       *
       * 获取协程管理器中正在调度的协程实例id
       *
       * @return: 具体正在运行的协程id
       */

      int32_t GetRunningCoroutineId()
      {
          return m_RunningId;
      }

      /**
       * @brief: GetCoStatus 
       * 获取某个协程的当前状态
       * 安全
       * @param iCoId
       *
       * @return 
       */
      int32_t GetCoStatus(const int32_t iCoId);

      /**
       * @brief: CoStatusDead
       *  确定某个协程的状态是否dead,协程已经无效
       * @param iCoId
       *
       * @return: true => is dead, false => other workeable status
       */
      bool CoStatusDead(const int32_t iCoId);

      /**
       * @brief: YieldCurrentCo 
       * 释放当前调度管理器中正在运行的协程实例,
       * 使当前的运行协程释放cpu,挂起
       *
       * @return: false, 失败；true, 成功
       */
      bool YieldCurrentCo();

      /**
       * @brief: ResumeCo 
       *
       * 调度某个协程，使其被唤醒起来工作
       *
       * @param iCorId: 唤醒的协程id
       *
       * @return 
       */
      bool ResumeCo(int32_t iCorId);

      char *GetStack()
      {
          return m_Stack;
      }

      ucontext_t* GetMainCtx()
      {
          return &m_Main;
      }

      Coroutiner* GetCoByCoId(const int32_t iCoId);
      void DeleteCo(const int32_t iCoId);

      std::string GetErrMsg()
      {
          return m_sErr;
      }
     public:
      static const int32_t STACK_SIZE = 1024*1024;
      static const int32_t MAX_NUM_CO  = 16;

     private:
      /**
       * @brief: CreateCoMgr 
       *
       * @return 
       */
      bool CreateCoMgr();

      /**
       * @brief: DestroyCoMgr 
       *
       * @return 
       */
      bool DestroyCoMgr();


      void ClearErr()
      {
        std::string sEmpty;
        m_sErr.swap(sEmpty);
      }

     private:
      char m_Stack[STACK_SIZE];
      ucontext_t m_Main;              // 正在running的协程在执行完后需切换到的上下文，由于是非对称协程，所以该上下文用来接管协程结束后的程序控制权
      int32_t m_Nco;                  // 调度器中已保存的协程数量
      int32_t m_Cap;                  // 调度器中协程的最大容量
      int32_t m_RunningId;            // 调度器中正在running的协程id
      std::vector<Coroutiner*> m_vCo; // 连续内存空间，用于存储所有协程任务
      bool m_Init;
      std::string m_sErr;
    };


    /*-----------------------------------
     * 创建协程的工厂基类定义
     *-----------------------------------*/
    class CoCreateFactory 
    {
     public:
      /**
       * @brief: GetNewCoroutine
       *  该方法由后续子类重载，不同子类创建
       *  不同类型的协程（协程的运行方法和方法参数 不同）
       *
       * @return: 返回创建的协程实例
       */
      virtual Coroutiner* GetNewCoroutine() = 0;
      CoCreateFactory() {}
      virtual ~CoCreateFactory() {}
    };

    /*-----------------------------------
     * 协程类定义,具体协程运行方法和参数，
     * 可由该类来派生
     *-----------------------------------*/
    class Coroutiner
    {
     public:
      Coroutiner();
      virtual ~Coroutiner();

      static void GloblCorFunc(uint32_t low32, uint32_t hi32);
      virtual void CorFunc() = 0;

      int GetCoStatus();

      bool ResumeCoReadyToRunning(CoroutinerMgr* pMgr);
      bool ResumeCoSuspendToRunning(CoroutinerMgr* pMgr);

      bool SaveStack(char *pTop);
      bool CreateCo();

      int32_t  GetId()
      {
          return m_RunId;
      }
      void SetId(const int32_t iId)
      {
          m_RunId = iId;
      }

      std::string GetErrMsg() 
      {
          return m_sErrMsg;
      }

     public:
      void SetMgr(CoroutinerMgr* pMgr)
      {
          m_pCoMgr = pMgr;
      }
      ptrdiff_t GetCoCap() 
      {
          return m_Cap;
      }
      void SetStatus(int32_t iStatus)
      {
          m_Status = iStatus;
      }
      int32_t GetStatus()
      {
          return m_Status;
      }

      ucontext_t* GetCoCtx() 
      {
          return &m_Ctx;
      }

      bool YieldCurCoInCo();

     protected:
      void FreeRes();
      //用于协程实例在逻辑处理完后的其他额外操作，
      //满足业务自身需求
      virtual void AfterFuncWork() = 0;

      //
      void ClearErrMsg()
      {
          std::string sTmp;
          m_sErrMsg.swap(sTmp);
      }

     protected:
      ucontext_t      m_Ctx; // 协程上下文
      ptrdiff_t       m_Cap;  // 协程栈的最大容量
      ptrdiff_t       m_Size; // 协程栈的当前容量
      int32_t         m_Status; // 协程状态
      char            *m_pStack;  // 协程栈
      int32_t         m_RunId;  //
      std::string     m_sErrMsg;
      CoroutinerMgr   *m_pCoMgr;
    };
}
