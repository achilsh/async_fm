#include "redispool_test.h"
#include <unistd.h>
#include <memory>
#include <sstream>

RedisPoolTest::RedisPoolTest(): m_iThreadNums(10), 
    m_iRedisPoolNums(5)
{
    m_RedisInterface = std::make_shared<FS_RedisInterface>(
        m_iRedisPoolNums, 100, "127.0.0.1",6379, 2);
    std::cout << "redis pool item capacity nums: " <<  m_iRedisPoolNums << std::endl;

}

RedisPoolTest::~RedisPoolTest()
{
}

void RedisPoolTest::StartPool()
{
    for (int i = 0; i < m_iThreadNums; ++i)
    {
        std::ostringstream os;
        os << i << "_key";

        m_threadVec.push_back(
            std::thread(&RedisPoolTest::RunRedisTest, 
            (this),  os.str()));
    }
    
    for (auto& oneThread: m_threadVec)
    {
        oneThread.join();
    }
}

void RedisPoolTest::RunRedisTest(const std::string& sKey)
{
    std::string sField = "field_hset_field";
    std::string sFieldVal = "field_hset_val";
    bool bret =  m_RedisInterface->HSet(sKey,sField, sFieldVal);

    //::sleep(1);
    std::cout << "key: " << sKey << ", field: " << sField << ", field val: " << sFieldVal << std::endl;
    if (bret == false)
    {
        std::cout << "hset fail" <<  std::endl;
    }
}


int main()
{
    RedisPoolTest test;
    test.StartPool();
    return 0;
}


