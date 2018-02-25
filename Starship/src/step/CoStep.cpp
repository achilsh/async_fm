#include "CoStep.hpp"

namespace oss
{

CoStep::CoStep(const std::string& sCoName):m_sCoName(sCoName)
{
}

CoStep::~CoStep()
{
}

std::string CoStep::GetCoName()
{
    return m_sCoName;
}

///
}
