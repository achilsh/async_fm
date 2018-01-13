/*******************************************************************************
* Project:  Starship
* @file     ClientMsgHead.hpp
* @brief    数据包头定义
* @author   
* @date:    2015年10月8日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_PROTOCOL_MSGHEAD_HPP_
#define SRC_PROTOCOL_MSGHEAD_HPP_

#pragma pack(1)

/**
 * @brief 与客户端通信消息头
 */
struct tagClientMsgHead
{
    unsigned char version;                  ///< 协议版本号（1字节）
    unsigned char encript;                  ///< 压缩加密算法（1字节）
    unsigned short cmd;                     ///< 命令字/功能号（2字节）
    unsigned short checksum;                ///< 校验码（2字节）
    unsigned int body_len;                  ///< 消息体长度（4字节）
    unsigned int seq;                       ///< 序列号（4字节）

    tagClientMsgHead() : version(0), encript(0), cmd(0), checksum(0), body_len(0), seq(0)
    {
    }
};

#pragma pack()


#endif /* SRC_PROTOCOL_MSGHEAD_H_ */
