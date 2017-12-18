#include "lib_shm.h"

namespace LIB_SHM  {

  ShmFLock::ShmFLock(const std::string& sShmKey):m_Init(false), m_openFd(0) {
    //std::string sKey = m_fLockPrefix + sShmKey;
    std::string sKey = SHM_WOP_FLOCK_PATH  + sShmKey;
    m_openFd = ::open(sKey.c_str(), O_CREAT|O_RDWR, 0600);
    if (m_openFd < 0) {
      sErrMsg = strerror(errno);
     sErrMsg  += "," + sKey;
      return ;
    }

    int32_t iRetLock = ::lockf(m_openFd, F_TLOCK, 0);
    if (iRetLock < 0) {
      ::close(m_openFd); m_openFd = 0;
      return ;
    }
    m_Init = true;
  }

  ShmFLock::~ShmFLock() {
    if (m_Init == true && m_openFd >0) {
      ::lockf(m_openFd,F_ULOCK,0);
      ::close(m_openFd);
    }
  }


  LibShm::LibShm(const enum ShmUseMode& shmMode): m_shmMode(shmMode) {
    m_sErrMsg.clear(); m_iErrCode = ERR_OK;
    m_shmKey = 0; m_shmSize = 0; m_shmId = 0; m_shmAddr = NULL;
    m_TbHash = NULL; m_WOpFirst = false;
  }

  LibShm::~LibShm() {
    if (m_shmAddr != NULL) {
      ::shmdt(m_shmAddr); m_shmAddr = NULL;
    }
    //
    if (m_TbHash != NULL) {
      m_TbHash->free(m_TbHash);
      m_TbHash = NULL;
    }
    //
  }

  bool LibShm::Init(key_t shmKey, size_t shmSize) {
    if (shmKey == 0 || shmSize ==0) {
      SetErrNo(ERR_SHMPARAM);
      SetErrMsg("shm key or shm size invalid");
      return false;
    }
    m_shmKey = shmKey;
    m_shmSize = shmSize;

    if (m_shmMode == OP_W) {
      m_shmId = ::shmget(m_shmKey,m_shmSize,0);
      if (m_shmId < 0) {
        if (ENOENT == errno) {
          std::stringstream ios ;
          ios << m_shmKey;
          ShmFLock shmLock(ios.str());
          if (shmLock.m_Init == true) {
            m_shmId = ::shmget(m_shmKey,m_shmSize,0);
            if (m_shmId > 0) {
              goto SHMRET;
              //return true;
            }
            m_shmId = ::shmget(m_shmKey,m_shmSize,IPC_CREAT|IPC_EXCL|0644);
            if (m_shmId < 0) {
              SetErrNo(ERR_SHMCREAT);
              SetErrMsg("create shm failed");
              return false;
            } else {
              m_WOpFirst = true;
              goto SHMRET;
              //return true;
            }
          } else {
            SetErrMsg(shmLock.GetErrMsg());
            SetErrNo(ERR_SHMLOCK); 
            return false;
          }
        } else {
          SetErrNo(ERR_SHMGET);
          std::string errMsg = strerror(errno);
          SetErrMsg(errMsg);
          return false;
        }
      }
SHMRET:
      m_shmAddr = ::shmat(m_shmId, NULL, 0);
      if (m_shmAddr == NULL) {
        SetErrNo(ERR_SHMATNULL);
        SetErrMsg("w op shmat failed");
        return false;
      }
    } else if (m_shmMode == OP_R) {
      m_shmId = ::shmget(m_shmKey,m_shmSize,0);
      if (m_shmId < 0) {
        SetErrNo(ERR_SHMGET);
        SetErrMsg("r op shmget failed");
        return false;
      }
      m_shmAddr = ::shmat(m_shmId, NULL,SHM_RDONLY);
      if (m_shmAddr == NULL) {
        SetErrNo(ERR_SHMATNULL);
        SetErrMsg("r op shmat failed");
        return false;
      }
    } else {
      SetErrNo(ERR_SHMPARAM);
      SetErrMsg("shm op not w and r,check class construct params");
      return false;
    }

    if (m_WOpFirst == true) {
      m_TbHash = qhasharr(m_shmAddr , m_shmSize);
    } else {
      m_TbHash = qhasharr(m_shmAddr, 0);
    }
    return true;
  }

bool LibShm::DelKey(const std::string& sKey) {
  if (sKey.empty() || m_TbHash == NULL) {
    SetErrNo(ERR_SHMPARAM);
    SetErrMsg("del key empty or hash addr empty");
    return false;
  }

  if (m_shmMode != OP_W) {
    SetErrNo(ERR_SHMWAUTH);
    SetErrMsg("del key just w op, check shm mode");
    return false;
  }

  if (false == qhasharr_remove(m_TbHash, sKey.c_str())) {
    if (errno != ENOENT) {
      SetErrNo(ERR_SHMDELKEY);
      SetErrMsg("del key failed");
      return false;
    }
    return true;
  }
  return true;
}

bool LibShm::ShmRm() {
  if (m_shmId > 0 && m_shmMode == OP_W) {
    ::shmctl(m_shmId, IPC_RMID, 0);
    return true;
  }
  return true;
}

bool LibShm::GetValue(const std::string& sKey, std::string& sData) {
  if (sKey.empty() || m_TbHash == NULL) {
    SetErrNo(ERR_SHMPARAM);
    SetErrMsg("get key empty or hash addr empty");
    return false;
  }

  size_t  stLen = 0;
  char *cRetValBuf = NULL;
  cRetValBuf =  (char*)qhasharr_get(m_TbHash, sKey.c_str(), &stLen); 
  if (NULL == cRetValBuf) {
    if (errno == ENOENT) {
      SetErrNo(ERR_SHMGETNOEXIST);
      SetErrMsg("key not exist");
      return false;
    }
    return false;
  }

  sData.assign(cRetValBuf, stLen);
  if (cRetValBuf != NULL) {
    free(cRetValBuf); cRetValBuf = NULL;
  }

  if (stLen < SHM_VALUE_MD5LEN) {
    SetErrNo(ERR_SHMPARAM);
    SetErrMsg("get data len less than md5 len");
    return false;
  }

  char cValMd5[SHM_VALUE_MD5LEN] = {0};
  qhashmd5(sData.c_str(), sData.size() - SHM_VALUE_MD5LEN, cValMd5);
  if (0 != ::memcmp(cValMd5, sData.c_str() + sData.size() - SHM_VALUE_MD5LEN, SHM_VALUE_MD5LEN)) {
    SetErrNo(ERR_SHMMD5CHECKSUM);
    SetErrMsg("md5 checksum not eq");
    return false;
  }
  sData.resize(sData.size() - SHM_VALUE_MD5LEN);

  return true;
}

//
}
