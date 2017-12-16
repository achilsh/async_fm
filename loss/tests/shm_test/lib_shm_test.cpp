#include "lib_shm.h"
#include <iostream>
#include <stdlib.h>

#define SHM_KEY  (0x201712)

using namespace std;
using namespace LIB_SHM;

//just test init, getvalue, setvalue, delkey op..

void Usage(const char* cBin) {
  std::cout << cBin << " 0 \n" << cBin << " 1" << std::endl;
  std::cout << "0: w op \n" << "1: r op " << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    Usage(argv[0]);
    return 0;
  }

  std::string sTestKeyStr = "test_key_str";
  std::string sTestKeyInt = "test_key_int";

  if (::atoi(argv[1]) == 0) {
    LibShm shmTest(LIB_SHM::OP_W);
    if (false == shmTest.Init(SHM_KEY, 10000)) {
      std::cout << "w op init shm failed, errmsg: " << shmTest.GetErrMsg() << std::endl;
      return 0;
    }
    //
    std::string sSetVal = "yes is my test  111 bbb cc ";
    if (false == shmTest.SetValue(sTestKeyStr, sSetVal)) {
      std::cout << "set string value failed, err msg: " << shmTest.GetErrMsg() << std::endl;
      return 0;
    }
    if (false == shmTest.SetValue<int>(sTestKeyInt , 100)) {
      std::cout << "set int value failed, err msg: " << shmTest.GetErrMsg() << std::endl;
      return 0;
    }
  } else if (::atoi(argv[1]) == 1) {
    LibShm shmTest(LIB_SHM::OP_R);
    if (false == shmTest.Init(SHM_KEY, 10000)) {
      std::cout << "r op init shm failed, err msg: " << shmTest.GetErrMsg() << std::endl;
      return 0;
    }
    std::string sGetStrVal;
    if (false == shmTest.GetValue(sTestKeyStr, sGetStrVal)) {
      std::cout << "get value from shm failed, errmsg: " << shmTest.GetErrMsg() << std::endl;
      return 0;
    }
    std::cout << "val:[" << sGetStrVal <<"][done] " << std::endl;
    int32_t iGetValInt = 0;
    if (false == shmTest.GetValue(sTestKeyInt,iGetValInt)) {
      std::cout << "get value from shm failed, errMsg: " << shmTest.GetErrMsg() << std::endl;
      return 0;
    }
    std::cout << "getInt: " << iGetValInt << std::endl;
  } else {
    Usage(argv[0]);
    return 0;
  }
  return 0;
}
