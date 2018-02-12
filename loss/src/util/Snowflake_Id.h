/**
 * @file: Snowflake_Id.h
 * @brief: 实现Snowflake c++ 版的全局id
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-02-11
 */

#ifndef _SRC_SNOWFLAKE_ID_H_
#define _SRC_SNOWFLAKE_ID_H_

#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

#include "util/LibSingleton.h"

namespace loss 
{

/**
* @brief: 
*
* 1. 41位的时间序列（精确到毫秒，41位的长度可以使用69年）
* 2. 10位的机器标识（10位的长度最多支持部署1024个节点） 
* 3. 12位的计数顺序号（12位的计数顺序号支持每个节点每毫秒产生4096个ID序号）\
*    最高位是符号位，始终为0。
*
* 4. 建议用单例模式来访问id 生成器
*/
class GlobalIDCreator
{
    public:
     GlobalIDCreator();
     virtual ~GlobalIDCreator();
     
     void SetMechineId(int32_t iMechineId)
     {
         m_iMId = iMechineId;
     }
       
     /**
      * @brief: GetGlobalID 
      *
      * 对外提供全局id生成接口
      * @return: 64位的id
      */
     uint64_t GetGlobalID();
    private:
     uint64_t GetTime();
     int32_t m_iMId;          /**< 实例运行环境机器id */
     int32_t m_iSeqPerMS;     /**< 实例运行每毫秒计数 */
};

/**---------------- 接口实现部分---------------------**/
GlobalIDCreator::GlobalIDCreator(): m_iMId(0), m_iSeqPerMS(0),
{
}

GlobalIDCreator::~GlobalIDCreator()
{
}

uint64_t GlobalIDCreator::GetTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t uiLTime = tv.tv_usec;
    uiLTime /= 1000;
    uiLTime += (tv.tv_sec * 1000);
    return uiLTime;
}

uint64_t GlobalIDCreator::GetGlobalID()
{
    uint64_t  uiLId = 0;
    uint64_t  gTime = GetTime();
    uiLId |= gTime << 22;
    uiLId |= m_iMId& 0x3ff << 12;

    uiLId |= m_iSeqPerMS++ & 0xfff;
    if (m_iSeqPerMS == 0x1000)
    {
        m_iSeqPerMS = 0;
    }
    return uiLId;
}

//-----------//
}

/*** 业务只需关注下面两个接口 ******／
 * 1 接口:SetGLobalMechineID(x),  设置一个机器id: int32_t 类型
 * 2 接口:GLOBAL_ID, 获取一个全局id, 返回一个int64_t  类型数字
 */

#define SetGLobalMechineID(mechine_id)    SINGLETON(loss::GlobalIDCreator)->SetMechineId(mechine_id)
#define GLOBAL_ID     SINGLETON(loss::GlobalIDCreator)->GetGlobalID()

#endif
