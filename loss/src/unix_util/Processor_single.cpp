#include "Processor_single.hpp"
#include <sstream>

namespace loss {

RunningProcess::RunningProcess(): m_iFd(0) {
}

RunningProcess::~RunningProcess() {
    Close();
}

bool RunningProcess::IsRun(const std::string& sLockFile) {
    std::ostringstream os;
       
    m_iFd = ::open(sLockFile.c_str(), O_CREAT, 400);
    if (m_iFd < 0) {
        m_sErr.clear();
        os << "open file: " << sLockFile 
            << ",err: " <<::strerror(errno);
        m_sErr = os.str();
        return false;
    }
    if (::flock(m_iFd, LOCK_EX|LOCK_NB) < 0) {
        os.str("");
        os << "flock err: " << ::strerror(errno);
        m_sErr = os.str();
        return false;
    }
    return true;
}

void RunningProcess::Close() {
    if (m_iFd) {
        ::close(m_iFd); m_iFd = 0;
    }
}
///////
}
