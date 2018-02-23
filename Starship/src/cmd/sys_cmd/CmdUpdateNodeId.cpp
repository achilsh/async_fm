/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdUpdateNodeId.cpp
 * @brief 
 * @author   
 * @date:    2015年9月18日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdUpdateNodeId.hpp"

namespace oss
{

CmdUpdateNodeId::CmdUpdateNodeId()
{
}

CmdUpdateNodeId::~CmdUpdateNodeId()
{
}

bool CmdUpdateNodeId::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    loss::CJsonObject oNode;
    if (oNode.Parse(oInMsgBody.body()))
    {
        int iNodeId = 0;
        oNode.Get("node_id", iNodeId);
        GetLabor()->SetNodeId(iNodeId);
        return(true);
    }
    return(false);
}

} /* namespace oss */
