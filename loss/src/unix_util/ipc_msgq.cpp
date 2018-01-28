#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sstream>

#include "ipc_msgq.h"

namespace loss
{

int CIpcMsgQueue::Initialize(int iKey, bool bCreate)
{
	int iFlag = 0666;
	iFlag |= bCreate ? IPC_CREAT : 0;

	if((m_iMsgQId = msgget(iKey, iFlag)) == -1) 
	{
        m_sErrMsg.clear();
        m_sErrMsg.append("msgget() Failed: ");
        m_sErrMsg.append(strerror(errno));
		return -1;
	}

	// Set QBytes
	struct msqid_ds buf;
    int iRetCode = msgctl(m_iMsgQId, IPC_STAT, (struct msqid_ds *)&buf);
    if(iRetCode == -1)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("msgctl(IPC_STAT) Failed: ");
        m_sErrMsg.append(strerror(errno));
        return -1;
    }

    buf.msg_qbytes = QBYTES_NUM;
    iRetCode = msgctl(m_iMsgQId, IPC_SET, (struct msqid_ds *)&buf);
    if(iRetCode == -1)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("msgctl(IPC_SET) Failed: ");
        m_sErrMsg.append(strerror(errno));
        return -1;
    }

    m_iKey      = iKey;
	m_iFlag     = iFlag;
	m_bInit     = true;

	return 0;
}

int CIpcMsgQueue::PutMsg(long lType, const char* pBuf, int iBufLen, bool bWait /* = false */)
{
    if(!m_bInit)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("Not Init");
        return -1;
    }

    if(pBuf == NULL || iBufLen <= 0 || iBufLen >= (int)MAX_MSG_SIZE)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("Invalid Buffer,Length: ");
        std::ostringstream os;
        os << iBufLen;
        m_sErrMsg.append(os.str());
        return -1;
    }

    m_stMsgBuf.lType = lType;
    memcpy(m_stMsgBuf.sBuf, pBuf, iBufLen);

    int msgflg = bWait ? 0 : IPC_NOWAIT; 
    if((msgsnd(m_iMsgQId, (struct msgbuf*)&m_stMsgBuf, iBufLen, msgflg)) ==-1)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("msgsnd() Failed: ");
        m_sErrMsg.append(strerror(errno));
        return -1;
    }

    return 0;
}

int CIpcMsgQueue::GetMsg(long lType, char* pBuf, int& iBufLen, bool bWait /* = false */)
{
    if(!m_bInit)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("not init");
        return -1;
    }

    int msgflg = bWait ? 0 : IPC_NOWAIT; 
	m_stMsgBuf.lType = lType;
	int iRetCode = msgrcv(m_iMsgQId, (struct msgbuf*)&m_stMsgBuf, MAX_MSG_SIZE, lType, msgflg);
	if(iRetCode < 0)
	{
        if(errno != ENOMSG)
        {
            m_sErrMsg.clear();
            m_sErrMsg.append("msgrcv() Failed: ");
            m_sErrMsg.append(strerror(errno));
        }

		return -1;
	}

	if(iBufLen < iRetCode)                      // 空间不足
	{
        m_sErrMsg.clear();
        m_sErrMsg.append("msgrcv() Failed: Not Enough Buffer! ");
        std::ostringstream os;
        os << "MsgLen[" << iRetCode << "] RecvBufLen[" << iBufLen << "]";
        m_sErrMsg.append(os.str());
		return -1;
	}

	memcpy(pBuf, m_stMsgBuf.sBuf, iRetCode);
	iBufLen = iRetCode;

	return 0;
}

int CIpcMsgQueue::GetAllMsg( char* pBuf, int& iBufLen, long & lType, bool bWait /* = false */)
{
    if(!m_bInit)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("Not Init");
        return -1;
    }

    int msgflg = bWait ? 0 : IPC_NOWAIT; 
	int iRetCode = msgrcv(m_iMsgQId, (struct msgbuf*)&m_stMsgBuf, MAX_MSG_SIZE, 0, msgflg);
	if(iRetCode < 0)
	{
        if(errno != ENOMSG)
        {
            m_sErrMsg.clear();
            m_sErrMsg.append("msgrcv() Failed: ");
            m_sErrMsg.append(strerror(errno));
        }

		return -1;
	}

	if(iBufLen < iRetCode)                      // 空间不足
	{
        m_sErrMsg.clear();
        m_sErrMsg.append("msgrcv() Failed: Not Enough Buffer! ");
        std::ostringstream os;
        os << "MsgLen[" << iRetCode << "] RecvBufLen[" << iBufLen << "]";
        m_sErrMsg.append(os.str());
		return -1;
	}
	
	lType = m_stMsgBuf.lType ;
	memcpy(pBuf, m_stMsgBuf.sBuf, iRetCode);
	iBufLen = iRetCode;

	return 0;
}


int CIpcMsgQueue::PeekMsg(long lType)
{
    if(!m_bInit)
    {
        m_sErrMsg.clear();
        m_sErrMsg.append("Not Init");
        return -1;
    }

	int iRetCode = msgrcv(m_iMsgQId, NULL, 0, lType,  IPC_NOWAIT);
	if(-1 == iRetCode)
	{
		if(errno == E2BIG)
			return 0;
	}

	return -1;
}

int CIpcMsgQueue::GetStat(int& iCBytes, int& iQNum)
{
    if(!m_bInit)
    {
        return -1;
    }

	struct msqid_ds buf;
    int iRetCode = msgctl(m_iMsgQId, IPC_STAT, (struct msqid_ds *)&buf);
    if(iRetCode == -1)
    {
        
        m_sErrMsg.clear();
        m_sErrMsg.append("msgctl(IPC_STAT) Failed:");
        m_sErrMsg.append(strerror(errno));
        return -1;
    }

	iCBytes = buf.__msg_cbytes; 
	iQNum = buf.msg_qnum;

	return 0;
}

}
