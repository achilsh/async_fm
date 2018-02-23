/**
 * @file: Method.hpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-22
 */

#ifndef SRC_CMD_METHOD_HPP_
#define SRC_CMD_METHOD_HPP_ 

#include "protocol/thrift2pb.pb.h"
#include "Cmd.hpp"
#include "step/ThriftStep.hpp"

namespace oss
{

class Method: public Cmd
{
     public:
      Method();
      virtual ~Method();
      virtual bool Init()
      {
          return true;
      }

      /**
       * @brief 从Cmd基类继承的命令处理入口
       * @note 在Module类中不需要实现此版本AnyMessage
       */
      virtual bool AnyMessage(
          const tagMsgShell& stMsgShell,
          const MsgHead& oInMsgHead,
          const MsgBody& oInMsgBody)
      {
          return(false);
      }

      /**
       * @brief thrift 服务模块处理入口
       * @param stMsgShell 来源消息外壳
       * @param oInThriftMsg接收到的thrift 数据包
       * @return 是否处理成功
       */
      virtual bool AnyMessage(
          const tagMsgShell& stMsgShell,
          const Thrift2Pb& oInThriftMsg, 
          uint32_t uiPacketLen, const uint8_t *pPacketBuf ) = 0;
    
      /**< 打包thrift协议的数据, 结果填充pb 回应报文 */
      template<typename T>
      bool PacketThriftData(Thrift2Pb& oInThriftMsg, const T& tData);

      /**< 解析pb 中请求的报文数据, 获取请求参数 */
      template<typename T>
      bool GetThriftParams(T& tParam, const Thrift2Pb& oInThriftMsg, 
                           uint32_t uiPacketLen, const uint8_t *pPacketBuf ); 

      /**< 向pb数据写入thrift的接口名 */
      void SetThriftInterfaceName(const std::string& sName, Thrift2Pb& oInThriftMsg)
      {
          oInThriftMsg.set_thrift_interface_name(sName);
      }
      
      /**< 向pb 数据写入thrift的 seq */
      void SetThriftSeq(const int iSeq, Thrift2Pb& oInThriftMsg)
      {
          oInThriftMsg.set_thrift_seq(iSeq);
      }

    public:
      const std::string& GetMethodName() const
      {
        return m_strMethodName;
      }

      void SetMethodName(const std::string& sMethodName)
      {
          m_strMethodName = sMethodName;
      }

      /**< 把thrift 返回数据打包成pb, 并把打包数据发送出去 */
    protected:
     template<typename T> bool SendAck(const tagMsgShell& stMsgShell,
                                Thrift2Pb& oInThriftMsg, const T& tRet);

    private:
      std::string m_strMethodName;
};
////
}

#include "Method_Template.hpp"

#endif

