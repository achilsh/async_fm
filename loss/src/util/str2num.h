/**
 * @file: str2num.h
 * @brief: 设置安全函数，实现字符串转化数字接口,
 *          类似于atoi(): char -> int 
 *          atol(): char -> long 
 *          atoll(): char -> longlong
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-15
 */

#ifndef __STR_TO_NUM_H__
#define __STR_TO_NUM_H__
#include <stdint.h>

namespace loss
{

    class SafeStrToNum
    {
        public:
         /**
          * @brief: StrToULL(): 字符串数字转化数字
          *     实现字符型值("0", (1 << sizeof(uint64)*8) -1) 转化为数字类型
          * @param str(in): 字符串数字
          * @param out: 数字
          *
          * @return true: succ, false: error
          */
         static bool StrToULL(const char *str, uint64_t *out);

         /**
          * @brief: StrToLL(): 字符串数字转化数字
          *     实现字符型值("-(1 << sizeof(uint64)*8)/2", (1 << sizeof(uint64)*8)/2) 转化为数字类型
          * @param str(in): 字符串数字
          * @param out: 数字
          *
          * @return true: succ, false: error
          */
         static bool StrToLL(const char *str, int64_t *out);

         /**
          * @brief: StrToUL(): 字符串数字转化数字
          *     实现字符型值("0", (1<< sizeof(uint32_t)*8) -1) 转化为数字字类型
          * @param str(in): 字符串数字
          * @param out: 数字
          *
          * @return true: succ, false: error
          */
         static bool StrToUL(const char *str, uint32_t *out);

         /**
          * @brief: StrToL(): 字符串数字转化数字
          *     实现字符型值("-(1<< sizeof(int32_t)*8)", (1<< sizeof(int32_t)*8) -1) 转化为字符类型
          * @param str(in): 字符串数字
          * @param out: 数字
          *
          * @return true: succ, false: error
          */
         static bool StrToL(const char *str, int32_t *out);

         /**
          * @brief: StrToD():  字符串数字转化数字
          *  实现double大小的数字字符转化为数字
          * @param str(in): 数字字符
          * @param out: 数字
          *
          * @return true: succ, false: error
          */
         static bool StrToD(const char *str, double *out);
         
         /**
          * @brief: StrToF(): 字符串数字转化数字
          *  实现float 大小的数字字符转化为数字
          * @param str(in): 数字字符
          * @param out: 数字
          *
          * @return true: succ, false: error
          */
         static bool StrToF(const char* str, float *out);
    };
}

#endif
