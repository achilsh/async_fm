#ifndef _SRC_COSTEP_HPP_
#define _SRC_COSTEP_HPP_

#include <string>
#include "LibCoroutine/include/CoroutineOp.hpp"
using namespace LibCoroutine;

namespace oss
{
 
class CoStep: public Coroutiner
{
    public:
     CoStep(const std::string& sCoName);
     virtual ~CoStep();
    
     /**
      * @brief: CorFunc
      *  由业务的子类来实现，
      *  该接口已经被协程调用
      */ 
     virtual void CorFunc() = 0;

     std::string GetCoName();
    protected:
     //该接口主要是用于业务在协程执行完后的其他业务操作
     virtual void AfterFuncWork() = 0;

    private:
     std::string m_sCoName;
};

/** ***/
}
#endif

