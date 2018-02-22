/**
 * @file: CmdHelloWorld.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00000001
 * @date: 2017-11-14
 */
#ifndef PLUGINS_CMDHELLOWORLD_H_
#define PLUGINS_CMDHELLOWORLD_H_

#include "cmd/Cmd.hpp"

#if 0

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 创建函数声明
 * @note 插件代码编译成so后存放到PluginServer的plugin目录，PluginServer加载动态库后调用create()
 * 创建插件类实例。
 */
  oss::Cmd* create();
#ifdef __cplusplus
}
#endif

#endif

class MStatic {
  public:
   static int m_MTest;
};

class CmdHelloWorld: public oss::Cmd 
{
  public:
   CmdHelloWorld();
   virtual ~CmdHelloWorld();
   virtual bool AnyMessage(
       const oss::tagMsgShell& stMsgShell,
       const MsgHead& oInMsgHead,
       const MsgBody& oInMsgBody);
  private:
    //TODO: 增加自己的需要数据成员 
   oss::tagMsgShell m_stMsgShell;
   MsgHead m_oMsgHead;
   MsgBody m_oMsgBody;
};

//
OSS_EXPORT(CmdHelloWorld);

#endif 
