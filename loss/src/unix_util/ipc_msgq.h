#ifndef _BOSS_CATEGORY_UTIL_IPC_MSG_Q_H_
#define _BOSS_CATEGORY_UTIL_IPC_MSG_Q_H_

#include <stdint.h>
#include <string>

namespace loss
{

const uint32_t MAX_MSG_SIZE = 1024 * 1024;                  // MsgQ消息的最大值, 1M

const unsigned long QBYTES_NUM = 10 * 1024 * 1024;          // max number of bytes allowed on queue, 10M

class CIpcMsgQueue
{
public:
    CIpcMsgQueue() : m_iMsgQId(0), m_iKey(0), m_iFlag(0), m_bInit(false)
    {
    }

    ~CIpcMsgQueue()
    {
    }

public:
	int Initialize(int iKey, bool bCreate);

	int PutMsg(long lType, const char* pBuf, int iBufLen, bool bWait = false);

	int GetMsg(long lType, char* pBuf, int& iBufLen, bool bWait = false);
	
	int GetAllMsg( char* pBuf, int& iBufLen, long & lType, bool bWait = false);

	int PeekMsg(long lType);

	int GetStat(int& iCBytes, int& iQNum);

    std::string GetErrMsg() { return m_sErrMsg; }

private:
	typedef struct tagMsgBuf
	{
        long   lType;
        char   sBuf[MAX_MSG_SIZE];
	} MsgBuf_T;

private:
	int         m_iMsgQId;          // Msg id

	int         m_iKey;             // Msg Key

	int         m_iFlag;            // Msg Flag

	bool        m_bInit;

	MsgBuf_T    m_stMsgBuf;

    std::string  m_sErrMsg;

};

} // namespace loss 

#endif  

