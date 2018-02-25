/*******************************************************************************
 * Project:  Starship
 * @file     StepIoTimeout.hpp
 * @brief    IO超时回调步骤
 * @author   
 * @date:    2015年10月31日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_SYS_STEP_STEPIOTIMEOUT_HPP_
#define SRC_STEP_SYS_STEP_STEPIOTIMEOUT_HPP_

#include "step/Step.hpp"

namespace oss
{

struct tagIoWatcherData;

/**
 * @brief IO超时回调步骤
 * @note 当发生正常连接（连接成功后曾经发送过合法数据包）IO超时事件时，创建一个StepIoTimeout，
 * 若该步骤正常回调，则重置连接超时，若该步骤超时，则关闭连接，销毁连接资源。该步骤实现的是服务
 * 端发起的心跳机制，心跳时间间隔就是IO超时时间。
 */
class StepIoTimeout: public Step
{
public:
    StepIoTimeout(const tagMsgShell& stMsgShell, struct ev_timer* pWatcher, const std::string& sCoName);
    virtual ~StepIoTimeout();

    //协程专用函数数 
    virtual void CorFunc(); 
private:
    tagMsgShell m_stMsgShell;
    struct ev_timer* watcher;       ///< 指向IO定时器，分配和析构均不在类体里
};

} /* namespace oss */

#endif /* SRC_STEP_SYS_STEP_STEPIOTIMEOUT_HPP_ */
