#ifndef _Free_Lock_Queue_h_
#define _Free_Lock_Queue_h_

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <utility>

#include <atomic>


namespace loss
{

template<typename T>
class FreeLockQue
{
 public:
  enum StatusNode
  {
      EM_QNODE_EMPTY        = 0,
      EM_QNODE_ENQUEING     = 1,
      EM_QNODE_ENQUEABLE    = 2,
      EM_QNODE_DEQUEING     = 3,
  };
 public:
  FreeLockQue(int iBufCapacity = 129)
      : m_pQue(NULL), m_pFlagsQue(NULL), m_iQueMaxLen(iBufCapacity)
        , m_iQueCurNums(0), m_iHeadIndex(0), m_iTailIndex(0)
    {
        InitQue();
    }

  virtual ~FreeLockQue()
  {
      delete [] m_pQue; 
      delete [] m_pFlagsQue;
  }

  bool EnQue(const T& tNode)
  {
      if (m_iQueCurNums >= m_iQueMaxLen)
      {
          return false;
      }

      int iTailIndexCur = m_iTailIndex;
      std::atomic<int>* pTailFlagIndexCur = m_pFlagsQue + iTailIndexCur;

      while(Cas(pTailFlagIndexCur, EM_QNODE_EMPTY, EM_QNODE_ENQUEING) == false)
      {
          iTailIndexCur = m_iTailIndex;
          pTailFlagIndexCur = m_pFlagsQue + iTailIndexCur;
      }

      int iTailIndexNew = (iTailIndexCur + 1) & (m_iQueMaxLen - 1);
      Cas(&m_iTailIndex, iTailIndexCur, iTailIndexNew);

      *(m_pQue + iTailIndexCur) = std::move(tNode);
      FetchAdd(pTailFlagIndexCur, EM_QNODE_ENQUEING);
      FetchAdd(&m_iQueCurNums, 1);

      return true;
  }

  bool DeQue(T* tpNode)
  {
      if (IsEmpty())
      {
          return false;
      }

      int iHeadIndexCur = m_iHeadIndex.load();
      std::atomic<int>* pHeadFlagIndexCur = m_pFlagsQue + iHeadIndexCur;

      while(Cas(pHeadFlagIndexCur, EM_QNODE_ENQUEABLE, EM_QNODE_DEQUEING) == false)
      {
          iHeadIndexCur = m_iHeadIndex.load();
          pHeadFlagIndexCur = m_pFlagsQue + iHeadIndexCur;

          if (IsEmpty())
          {
              return false;
          }
      }

      int iHeadIndexNew = (iHeadIndexCur +1) & (m_iQueMaxLen - 1);
      Cas(&m_iHeadIndex, iHeadIndexCur, iHeadIndexNew);

      if (tpNode)
      {
          *tpNode = std::move(*(m_pQue + iHeadIndexCur));      
      }

      FetchSub(pHeadFlagIndexCur, EM_QNODE_DEQUEING);
      FetchSub(&m_iQueCurNums, 1);

      return true;
  }

  bool IsEmpty() const
  {
      return m_iQueCurNums <= 0;
  }

  int Size() const
  {
      return m_iQueCurNums;
  }


 private:
  bool InitQue()
  {
      if (Power2Exponent(m_iQueMaxLen) == false)
      {
          return false;
      }

      m_pFlagsQue = new(std::nothrow) std::atomic<int>[m_iQueMaxLen];
      if (m_pFlagsQue == NULL)
      {
          return false;
      }
      memset(m_pFlagsQue, (int)EM_QNODE_EMPTY, sizeof(int)* m_iQueMaxLen);
      //memset(m_pFlagsQue, 0, m_iQueMaxLen);

      m_pQue = reinterpret_cast<T*>(new(std::nothrow) char[ m_iQueMaxLen*sizeof(T) ]); 
      if (m_pQue == NULL)
      {
          return false;
      }
      memset(m_pQue,0,m_iQueMaxLen*sizeof(T));
      return true;
  }

  bool Power2Exponent(int& iOrigin)
  {
      if (iOrigin <= 0)
      {
          return false;
      }

      do 
      {
          if(!(iOrigin & (iOrigin -1)))
          {
              break;
          }

          iOrigin++;

      } while(1);

      return true;
  }

  bool Cas(std::atomic<int>* pReg, int iOldVal, int iNewVal)
  {
      return std::atomic_compare_exchange_weak(pReg, &iOldVal, iNewVal);
  }

  bool FetchAdd(std::atomic<int>* pReg, int iAdd)
  {
      return std::atomic_fetch_add(pReg, iAdd);
  }

  bool FetchSub(std::atomic<int>* pReg, int iSub)
  {
      return std::atomic_fetch_sub(pReg, iSub);
  }

 private:
  T* m_pQue;
  // 0: empty, 1: enque-ing, 2: enque-able, 3: deque-ing
  std::atomic<int>* m_pFlagsQue;      // flag for each que node 
  int m_iQueMaxLen;             // default node size for ring que 
  std::atomic<int> m_iQueCurNums;     // ring que item nums
  std::atomic<int> m_iHeadIndex;      // index for head node
  std::atomic<int> m_iTailIndex;      // index for tail node
};

}

#endif
