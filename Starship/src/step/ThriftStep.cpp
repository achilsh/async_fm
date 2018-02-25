#include "ThriftStep.hpp"

namespace oss
{

ThriftStep::ThriftStep(const oss::tagMsgShell& stMsgShell, unsigned int iSeq, 
                       const std::string& sName,
                       const std::string& sCoName = "thrift_step") 
    :Step(sCoName), m_stMsgShell(stMsgShell), m_iSeq(iSeq), m_sName(sName)
{
}

ThriftStep::ThriftStep(const std::string& sCoName) :Step(sCoName)
{
}

ThriftStep::~ThriftStep()
{
}

bool ThriftStep::SendTo(const Thrift2Pb& oThriftMsg)
{
    return (GetLabor()->SendTo(m_stMsgShell, oThriftMsg));
}

////
}
