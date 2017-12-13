/*******************************************************************************
* Project:  DataAnalysis
* File:     Log.cpp
* Description: 文件日志类，用于写日志文件
* Author:         
* Created date:  2010-12-20
* Modify history:
*******************************************************************************/


#include "FileLog.hpp"


loss::CLog::CLog(const std::string strLogFile, int iLogLev,
        unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex)
    : m_iLogLevel(iLogLev), m_uiMaxFileSize(uiMaxFileSize),
      m_uiMaxRollFileIndex(uiMaxRollFileIndex), m_strLogFileBase(strLogFile)
{
    m_fp = NULL;
    OpenLogFile(strLogFile);
}

int loss::CLog::OpenLogFile(const std::string strLogFile)
{
    m_fp = fopen(strLogFile.c_str(), "a+" );
    if(NULL == m_fp)
    {
        std::cerr << "Can not open file: " << strLogFile << std::endl;
        return -1;
    }
    return 0;
}

int loss::CLog::WriteLog(int iLev, const char* szLogStr, ...)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(NULL == m_fp)
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    va_list ap;
    va_start(ap, szLogStr);
    Vappend(iLev, szLogStr, ap);
    va_end(ap);

    fprintf(m_fp, "\n");

    fflush(m_fp);

    return 0;
}

void loss::CLog::ReOpen()
{
    if (NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
    m_fp = fopen(m_strLogFileBase.c_str(), "a+");
}

void loss::CLog::RollOver()
{
    if (NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
    std::stringstream ssOldestFile(
            std::stringstream::in | std::stringstream::out);
    ssOldestFile << m_strLogFileBase << "." << m_uiMaxRollFileIndex;
    remove(ssOldestFile.str().c_str());

    for (int i = m_uiMaxRollFileIndex - 1; i >= 1; --i)
    {
        std::stringstream ssSrcFileName(
                std::stringstream::in | std::stringstream::out);
        std::stringstream ssDestFileName(
                std::stringstream::in | std::stringstream::out);

        ssSrcFileName << m_strLogFileBase << "." << i;
        ssDestFileName << m_strLogFileBase << "." << (i + 1);
        remove(ssDestFileName.str().c_str());
        rename(ssSrcFileName.str().c_str(), ssDestFileName.str().c_str());
    }
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss << m_strLogFileBase << ".1";
    std::string strBackupFile = ss.str();
    rename(m_strLogFileBase.c_str(), strBackupFile.c_str());
}

int loss::CLog::Vappend(int iLev, const char* szLogStr, va_list ap)
{
    long file_size = -1;
    if (NULL != m_fp)
    {
        file_size = ftell(m_fp);
    }
    //    if (0 == fstat(m_fd, &sb))
    //    {
    //        file_size = sb.st_size;
    //    }
    if (file_size < 0)
    {
        ReOpen();
    }
    else if (file_size >= m_uiMaxFileSize)
    {
        RollOver();
        ReOpen();
    }
    if (NULL == m_fp)
    {
        return -1;
    }
    fprintf(m_fp, "[%s] [%s]\t", loss::GetCurrentTime(20).c_str(),
       loss::LogLevMsg[iLev].c_str());
    vfprintf(m_fp, szLogStr, ap);
    return 0;
}


