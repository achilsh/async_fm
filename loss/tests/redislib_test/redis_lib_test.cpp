#include "lib_redis.h"
#include <string>

using namespace  LIB_REDIS;

int32_t sub_ret_proc(const std::string& cs1, const std::string& cs2) {
  std::cout << "changel: " << cs1 << " ,mesg: " <<  cs2 << std::endl;
  return 0;
}

int main(int argc, char **argv) {
  Client RedisClient;
  int iRet = RedisClient.Init("127.0.0.1",6379, "",0);
  if (iRet != 0) {
    std::cout << "redis client init failed" << std::endl;
  }

  RedisHash HashTest(RedisClient);
  bool bRet = HashTest.hSet("h_test","name","jack");
  if (false == bRet) {
    std::cout << "hset fail" << std::endl;
    return -1;
  }

  std::string name_val;
  bRet = HashTest.hGet("h_test","name", name_val);
  std::cout << "get h_test name value: " << name_val << std::endl;
  if (argc < 2) {
    std::cout << "usage: " << "bin sub/pub" << std::endl;
    return 0;
  }

  std::string ch = "a_test";
  if (std::string(argv[1]) == "0") {
    std::cout << "is sub proc" << std::endl;
    //asyn sub test:
    AsyncClient asynTest("127.0.0.1",6379,"", 0);
    RedisSubscriber subTest(asynTest);
    subTest.Subscribe(ch, &sub_ret_proc);
  } else if (std::string(argv[1]) == "1") {
     RedisPublisher pubTest(RedisClient);
     pubTest.Publish(ch,"wo is pub");
  } else {
    std::cout << "1 is pub, 0 is sub" << std::endl;
    return -1;
  }
  return 0;
}


