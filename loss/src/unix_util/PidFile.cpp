#include "PidFile.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include "util/str2num.h"

namespace loss
{
    Pidfile::Pidfile(const std::string& sFileName):m_sFileName(sFileName), m_sErrMsg("")
    {
    }
    Pidfile::~Pidfile()
    {
        RemovePidFile();
    }
    bool Pidfile::SaveCurPidInFile()
    {
        if (m_sFileName.empty())
        {
            m_sErrMsg = "pid file name is param";
            return false;
        }
        const char *pFile = m_sFileName.c_str();
        FILE *fp = NULL;
        if (access(pFile, F_OK) == 0)
        {
            if ((fp = fopen(pFile, "r")) != NULL)
            {
                char buffer[1024] = {0};
                if (fgets(buffer, sizeof(buffer),fp) != NULL)
                {
                    unsigned int pid;
                    if (SafeStrToNum::StrToUL(buffer, &pid) && kill((pid_t)pid, 0) == 0)
                    {
                        m_sErrMsg = "this pid file contained the following pid";
                    }
                }
                fclose(fp);
            }
        }
        /* Create the pid file first with a temporary name, then
         * atomically move the file to the real name to avoid a race with
         * another process opening the file to read the pid, but finding
         * it empty.
         */ 
        char tmp_pid_file[1024] = {0};
        snprintf(tmp_pid_file, sizeof(tmp_pid_file), "%s.tmp", pFile);
        if ((fp = fopen(tmp_pid_file, "w")) == NULL)
        {
            m_sErrMsg += "open tmp pid file fail";
            return false;
        }
        fprintf(fp, "%ld\n", (long)getpid());
        if (fclose(fp) == -1)
        {
            m_sErrMsg += "close pid file fail";
            return false;
        }
        if (rename(tmp_pid_file, pFile) != 0)
        {
            m_sErrMsg += "rename tmp pid file to pid file fail";
            return false;
        }
        return true;
    }

    bool Pidfile::RemovePidFile()
    {
        if (m_sFileName.empty())
        {
            return false;
        }
        if (unlink(m_sFileName.c_str()) != 0)
        {
            m_sErrMsg = "remove pid file fail";
            return false;
        }
        return true;
    }
}
