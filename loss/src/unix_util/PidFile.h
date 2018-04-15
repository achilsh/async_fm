/**
 * @file: PidFile.h
 * @brief: 用于保存当前进程pid到文件的操作, 该类用于服务进程内部调用.
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-15
 */
#ifndef __PID_FILE_H__
#define __PID_FILE_H__
#include <string>

namespace loss
{
class Pidfile 
{
    public:
     Pidfile(const std::string& sFileName);
     virtual ~Pidfile();
     bool SaveCurPidInFile();
     bool RemovePidFile();
     const std::string& GetErrMsg() const 
     {
         return m_sErrMsg;
     }
    private:
     std::string m_sFileName;
     std::string m_sErrMsg;
};

}

#endif

