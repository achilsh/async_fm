/**
 * @file: Processor_single.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-18
 */
#ifndef __PROCESSOR_SINGLE_HPP__
#define __PROCESSOR_SINGLE_HPP__

#ifdef __cplusplus
    extern "C" {
#endif 
    #include <sys/file.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <stdint.h>
    #include <string.h>
    #include <errno.h>
    #include <unistd.h>
#ifdef __cplusplus 
    }
#endif

#include <string>

namespace loss {

class RunningProcess {
    public:
     RunningProcess();
     virtual ~RunningProcess();
     bool IsRun(const std::string& sProcLockFile);
     std::string GetErrMsg() const { return m_sErr; }
    private:
     void Close();
    private:
     int32_t  m_iFd;
     std::string m_sErr;
};

}
#endif

