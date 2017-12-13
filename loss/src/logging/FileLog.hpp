/*******************************************************************************
* Project:  DataAnalysis
* File:     Log.hpp
* Description: 文件日志类，用于写日志文件
* Author:         
* Created date:  2010-12-20
* Modify history:
*******************************************************************************/

#ifndef LOG_HPP_
#define LOG_HPP_

#include <cstdio>
#include <cstdarg>
#include <string>
#include <iostream>
#include "LogBase.hpp"
#include "util/UnixTime.hpp"

namespace loss
{

class CLog : public loss::CLogBase
{
public:
    explicit CLog(
            const std::string strLogFile,
            int iLogLev = loss::DEFAULT_LOG_LEVEL,
            unsigned int uiMaxFileSize = loss::gc_uiMaxLogFileSize,
            unsigned int uiMaxRollFileIndex = loss::gc_uiMaxRollLogFileIndex);
    virtual ~CLog()
    {
        fclose(m_fp);
    }

    int SetLogLevel(int iLev)
    {
        m_iLogLevel = iLev;
        return(0);
    }

    virtual int WriteLog(int iLev = INFO, const char* szLogStr = "info", ...);

private:
    int OpenLogFile(const std::string strLogFile);
    void ReOpen();
    void RollOver();
    int Vappend(int iLev, const char* szLogStr, va_list ap);

    FILE* m_fp;
    int m_iLogLevel;
    unsigned int m_uiMaxFileSize;       // 日志文件大小
    unsigned int m_uiMaxRollFileIndex;  // 滚动日志文件数量
    std::string m_strLogFileBase;       // 日志文件基本名（如 log/program_name.log）
};


}

#endif /* LOG_HPP_ */
