#ifndef __SRC_STEPCONN_REMOTENODE_HPP__
#define __SRC_STEPCONN_REMOTENODE_HPP__

#include "protocol/oss_sys.pb.h"
#include "step/Step.hpp"

namespace oss
{
class Step;

class StepConnRemoteNode: public Step
{
 public:
    StepConnRemoteNode(const tagMsgShell& stMsgShell,
                       const MsgHead& oInMsgHead,
                       const MsgBody& oInMsgBody,
                       const std::string& sCoName);
    virtual  ~StepConnRemoteNode();

    virtual void CorFunc();     //协程专用函数数
 private:

    bool UpDatePeerNodeInfo(const std::string& strNodeType,const std::string& strNodeInfo);
    int m_iTimeoutNum;          // 超时次数
};

///////
}
#endif
