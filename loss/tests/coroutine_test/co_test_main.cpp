#include "CoroutineOp.hpp"
#include "util/LibStdCout.h"
#include <stdio.h>

using namespace LibCoroutine;

class CoOneParam: public Coroutiner
{
 public:
  CoOneParam() {} 
  virtual ~CoOneParam() {}
  
  virtual void CorFunc();


  void SetOne(const int one)
  {
      m_One = one;
  }
 protected:
  void AfterFuncWork();
 private:
  int m_One;
};

void CoOneParam::CorFunc() 
{
    for (int iCnt = 0; iCnt < 5; ++iCnt)
    {
        DEBUG_LOG("co id: %d, val: %d, next to yield", GetId(), iCnt + m_One);
        YieldCurCoInCo();
        DEBUG_LOG("co id: %d, awake again do work", GetId());
    }
}

void CoOneParam::AfterFuncWork()
{
    DEBUG_LOG("fun done, other works, co id: %d", GetId());
}

//--------------------------------------------
class CoOneParamCreate: public  CoCreateFactory
{
 public:
  CoOneParamCreate() {}
  virtual ~CoOneParamCreate() {}
  virtual Coroutiner* GetNewCoroutine();
};

Coroutiner* CoOneParamCreate::GetNewCoroutine()
{
    CoOneParam* pCo = new  CoOneParam();
    pCo->SetOne(100);
    return pCo;
}

////--------------------
class CoTwoParam: public Coroutiner
{
 public:
  CoTwoParam(){}
  virtual ~CoTwoParam() {}
  virtual void CorFunc();

  void SetTwo(int x, int y)
  {
      m_one = x;
      m_two = y;
  }
 protected:
    virtual void AfterFuncWork();
 private:
  int m_one;
  int m_two;
};

void CoTwoParam::CorFunc() 
{
    for (int iCnt = 0; iCnt < 5; ++iCnt)
    {
        DEBUG_LOG("co id: %d, val: %d, next to yield", GetId(), iCnt + m_one + m_two);
        YieldCurCoInCo();
        DEBUG_LOG("co id: %d, awake again do work", GetId());
    }
}

void CoTwoParam::AfterFuncWork()
{
    DEBUG_LOG("fun done, other works, co id: %d", GetId());
}

class CoTwoParamCreate: public  CoCreateFactory
{
 public:
    CoTwoParamCreate() {}
    virtual ~CoTwoParamCreate() {}
    virtual Coroutiner* GetNewCoroutine();
};

Coroutiner* CoTwoParamCreate::GetNewCoroutine()
{
    CoTwoParam* pCo = new CoTwoParam();
    pCo->SetTwo(200, 300);
    return pCo;
}


////////////////////////

int main()
{
    CoCreateFactory *pCoCreate1 = new CoOneParamCreate();
    Coroutiner* pCo1 = pCoCreate1->GetNewCoroutine();
    
    CoroutinerMgr coMgr;
    if (false == coMgr.AddNewCoroutine(pCo1))
    {
        delete pCo1;
        delete pCoCreate1;
        return 0;
    }
    int iCoIdOne = pCo1->GetId();

    CoCreateFactory* pCoCreate2  = new CoTwoParamCreate();
    Coroutiner* pCo2 = pCoCreate2->GetNewCoroutine();
    if (false == coMgr.AddNewCoroutine(pCo2))
    {
        delete pCoCreate2;
        delete pCo2;
        return 0;
    }

    int iCoIdTwo =  pCo2->GetId();

    
    int iStatus_One = coMgr.GetCoStatus(iCoIdOne);
    int iStatus_Two = coMgr.GetCoStatus(iCoIdTwo);
    
    while(iStatus_One && iStatus_Two)
    {
        coMgr.ResumeCo(pCo2->GetId());
        coMgr.ResumeCo(pCo1->GetId());

        iStatus_One = coMgr.GetCoStatus(iCoIdOne);
        iStatus_Two = coMgr.GetCoStatus(iCoIdTwo);
    }
    
    DEBUG_LOG("main end!");
    
    delete pCoCreate2;
    delete pCoCreate1;
}


