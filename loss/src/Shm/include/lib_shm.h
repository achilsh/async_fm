/**
 * @file: lib_shm.h
 * @brief: 对共享内存操作的封装，底层采用hashmap 存储共享内存数据
 *        支持多进程写，多进程读模式
 * @author:  wusheng Hu
 * @version: 0x01
 * @date: 2017-12-15
 */

#ifndef _LIB_SHM_H_
#define _LIB_SHM_H_

#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
//
#include <string>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include "qlibc/qlibc.h"

#define SHM_VALUE_MD5LEN    (16)
#define SHM_WOP_FLOCK_PATH  "/tmp/shm_wflock_"

namespace LIB_SHM {
  //shm mode:
  enum ShmUseMode {
    OP_W = 0,
    OP_R = 1,
  };

  enum ShmErrCode {
    ERR_OK = 0, 
    ERR_SHMPARAM = 1,
    ERR_SHMGET = 2,
    ERR_SHMCREAT = 3,
    ERR_SHMLOCK = 4,
    ERR_SHMATNULL = 5,
    ERR_SHMWAUTH = 6,
    ERR_SHMPUT = 7,
    ERR_SHMGETNOEXIST = 8,
    ERR_SHMMD5CHECKSUM = 9,
    ERR_SHMDELKEY  = 10,
  };
   
  class ShmFLock {
    public:
     ShmFLock(const std::string& sShmKey);
     virtual ~ShmFLock();
     //static const std::string m_fLockPrefix;
     bool m_Init;
     std::string GetErrMsg() { return sErrMsg; }
    private:
     std::string sErrMsg;
     int32_t m_openFd;
  };
  //const std::string ShmFLock::m_fLockPrefix = "/tmp/shm_wflock_";
  
  class LibShm {
    public:
     LibShm(const enum ShmUseMode& shmMode = OP_R);
     virtual ~LibShm();

     bool Init(key_t shmKey, size_t shmSize);
     template<class T>
     bool SetValue(const std::string& sKey, const T& tData);
     
     bool GetValue(const std::string& sKey, std::string& sData);
     bool GetValue(const std::string& sKey, int32_t&  tData);
     bool GetValue(const std::string& sKey, uint32_t&  tData);
     bool GetValue(const std::string& sKey, int64_t&  tData);
     bool GetValue(const std::string& sKey, uint64_t&  tData);
     bool GetValue(const std::string& sKey, double&  tData);
     bool GetValue(const std::string& sKey, bool&  tData);

     bool DelKey(const std::string& sKey);
     bool ShmRm();

     std::string GetErrMsg() const { return m_sErrMsg; }
     int GetErrNo() const { return m_iErrCode; }
    
    private:
     void SetErrMsg(const std::string& sErrMsg) {
       m_sErrMsg.assign(sErrMsg);
     }
     void SetErrNo(const int iErrNo) { 
       m_iErrCode = iErrNo;
     }
      
    private:
     std::string m_sErrMsg;
     int m_iErrCode;
     int m_shmMode;
     key_t m_shmKey;
     size_t m_shmSize;
     int32_t m_shmId;
     void* m_shmAddr;
    
     bool m_WOpFirst;
     qhasharr_t * m_TbHash;
  };

  //
  template<class T>
  bool LibShm::SetValue(const std::string& sKey, const T& tData) {
    if (m_shmMode != OP_W) {
      SetErrNo(ERR_SHMWAUTH);
      SetErrMsg("only w op can do set,check mode");
      return false;
    }
    if (sKey.empty() || m_TbHash == NULL) {
      SetErrNo(ERR_SHMPARAM);
      SetErrMsg("set key empty or hash addr empty");
      return false;
    }
    // calc md5 for tData;
    std::stringstream ios;
    ios << tData;
    std::string sData = ios.str();

    char cValMd5[SHM_VALUE_MD5LEN] = {0};
    qhashmd5(sData.c_str(), sData.size(), cValMd5);

    sData.append(cValMd5,SHM_VALUE_MD5LEN);
    if (false == qhasharr_put(m_TbHash, sKey.c_str(),
                              sData.c_str(), sData.size())) {
      SetErrNo(ERR_SHMPUT); 
      SetErrMsg("set key value to hash failed");
      return false;
    }
    return true;
  }

///
}

#endif
