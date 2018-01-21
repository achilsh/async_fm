#include "TestSingleton.h"
#include <iostream>
namespace im 
{
    TestSingletonIm::TestSingletonIm() 
    {
    }
    TestSingletonIm::~TestSingletonIm() 
    {
        std::cout << "call obj deconstruct func" << std::endl;
    }

    void TestSingletonIm::SetVal(const std::string& sVal)
    {
        m_sVal = sVal;
    }

    std::string TestSingletonIm::GetVal() const
    {
        return m_sVal;
    }
    TestGG::TestGG() 
    {
    }
    TestGG::~TestGG() 
    {
    }
    int TestGG::m_X;
}
