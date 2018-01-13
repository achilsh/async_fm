/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepNodeNotice.cpp
 * @brief    告知对端己方Worker进程信息
 * @author   
 * @date:    2015年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepNodeNotice.hpp"
#include <iostream>
using namespace std;


namespace oss
{
StepNodeNotice::StepNodeNotice(const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
    : m_iTimeoutNum(0), m_stMsgShell(stMsgShell)
{
    if (!m_jsonData.Parse(oInMsgBody.body()))
    {
        LOG4CPLUS_DEBUG_FMT(GetLogger(), "StepNodeNotice::Start!  m_jsonData.Parse fail");
    }
}

StepNodeNotice::~StepNodeNotice()
{
}

E_CMD_STATUS StepNodeNotice::Emit(
                int iErrno,
                const std::string& strErrMsg,
                const std::string& strErrShow)
{
    string node_type = "";
    string node_ip = "";
    int    node_port = 0;
    int    worker_num = 0;
    int    i,j;
    char   strIdentify[50] = {0};
    for (i = 0;i<m_jsonData["node_arry_reg"].GetArraySize();i++)
    {
        if (
            m_jsonData["node_arry_reg"][i].Get("node_type",node_type)
            &&m_jsonData["node_arry_reg"][i].Get("node_ip",node_ip)
            &&m_jsonData["node_arry_reg"][i].Get("node_port",node_port)
            &&m_jsonData["node_arry_reg"][i].Get("worker_num",worker_num)
            )
        {
            for(j = 0;j<worker_num;j++)
            {
                memset(strIdentify,0,50);
                sprintf(strIdentify,"%s:%d:%d",node_ip.c_str(),node_port,j);
                Step::AddNodeIdentify(node_type,string(strIdentify));
                LOG4CPLUS_DEBUG_FMT(GetLogger(), "Step::AddNodeIdentify(%s,%s)",node_type.c_str(),strIdentify);
            }
        }
    }
    
    for (i = 0;i<m_jsonData["node_arry_exit"].GetArraySize();i++)
    {
        if (
            m_jsonData["node_arry_exit"][i].Get("node_type",node_type)
            &&m_jsonData["node_arry_exit"][i].Get("node_ip",node_ip)
            &&m_jsonData["node_arry_exit"][i].Get("node_port",node_port)
            &&m_jsonData["node_arry_exit"][i].Get("worker_num",worker_num)
            )
        {
            for(j = 0;j<worker_num;j++)
            {
                memset(strIdentify,0,50);
                sprintf(strIdentify,"%s:%d:%d",node_ip.c_str(),node_port,j);
                Step::DelNodeIdentify(node_type,string(strIdentify));
                LOG4CPLUS_DEBUG_FMT(GetLogger(), "Step::DelNodeIdentify(%s,%s)",node_type.c_str(),strIdentify);
            }
        }
    }
    
    return(STATUS_CMD_COMPLETED);
}

E_CMD_STATUS StepNodeNotice::Callback(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
     return(STATUS_CMD_COMPLETED);
}

E_CMD_STATUS StepNodeNotice::Timeout()
{
    ++m_iTimeoutNum;
    if (m_iTimeoutNum <= 3)
    {
        return(STATUS_CMD_FAULT);
    }
    else
    {
        LOG4_ERROR("timeout %d times!", m_iTimeoutNum);
        return(STATUS_CMD_FAULT);
    }
}

} /* namespace oss */
