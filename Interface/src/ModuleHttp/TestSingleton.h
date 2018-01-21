/**
 * @file: TestSingleton.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-20
 */
#ifndef __TEST_SINGLETON_H__
#define __TEST_SINGLETON_H__ 

#include "util/LibSingleton.h"
#include <string>

namespace im 
{
    class TestSingletonIm 
    {
        public:
         TestSingletonIm();
         virtual ~TestSingletonIm();
         void SetVal(const std::string& sVal);
         std::string GetVal() const;

        private:
         std::string m_sVal;
    };

    #define SINGLETEST SINGLETON(TestSingletonIm)
    static int TestGlobalStatic = 15;

    class TestGG 
    {
     public:
      TestGG();
      virtual ~TestGG();
      static int GetX() 
      {
          return m_X;
      }
      static void StaticSetX(const int x) 
      {
          m_X = x;
      }
      void SetX(const int x) 
      {
          m_X = x;
      }
     private:
      static int m_X;
    };
}

#endif

