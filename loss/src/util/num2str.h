/**
 * @file: num2str.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-04-25
 */

#ifndef _t_num2str_h_
#define _t_num2str_h_
#include <stdint.h>
namespace loss
{
   char* itoa_u32(uint32_t u, char* pDstBuf);
   char *itoa_32(int32_t i, char* pDstBuf);
   char *itoa_u64(uint64_t i, char *pDstBuf);
   char *itoa_64(int64_t i, char *pDstbuf);
}
#endif

