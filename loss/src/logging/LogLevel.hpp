/*******************************************************************************
* Project:  DataAnalysis
* File:     LogLevel.hpp
* Description: 定义日志错误级别
* Author:         
* Created date:  2010-12-15
* Modify history:
*******************************************************************************/

#ifndef LOGLEVEL_HPP_
#define LOGLEVEL_HPP_

#include <string>

namespace loss
{

enum LogLev
{
    FATAL = 0,          //致命错误
    CRITICAL = 1,       //严重错误
    ERROR = 2,          //一般错误
    NOTICE = 3,         //关键提示消息
    WARNING = 4,        //警告
    INFO = 5,           //一般提示消息
    DEBUG_MSG = 6,      //调试消息
    LEV_MAX             //日志错误级别数
};

const std::string LogLevMsg[LEV_MAX] =
{
    "FATAL",            //致命错误
    "CRITICAL",         //严重错误
    "ERROR",            //一般错误
    "NOTICE",           //关键提示消息
    "WARNING",          //警告
    "INFO",             //一般提示消息
    "DEBUG"             //调试消息
};

const int DEFAULT_LOG_LEVEL = INFO;

}

#endif /* LOGLEVEL_HPP_ */
