/*******************************************************************************
 * Project:  loss
 * @file     RedisCmd.hpp
 * @brief    redis命令生成
 * @author   
 * @date:    2015年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_DBI_REDIS_REDISCMD_HPP_
#define SRC_DBI_REDIS_REDISCMD_HPP_

#include <string>
#include <vector>

namespace loss
{

class RedisCmd
{
public:
	RedisCmd();
	virtual ~RedisCmd();

	/**
	 * @brief 设置redis命令
	 */
    void SetCmd(const std::string& strCmd);

    /**
     * @brief 设置redis命令参数
     * @note redis命令后面的key也认为是参数之一
     */
    void Append(const std::string& strArgument, bool bIsBinaryArg = false);

	/**
	 * @brief 设置用户hash定位redis节点的字符串
	 * @note 所设置的字符串不是redis的key，key和参数一起通过Append()添加
	 */
	void SetHashKey(const std::string& strHashKey);
	std::string ToString() const;

	const std::string& GetErrMsg() const
	{
	    return(m_strErr);
	}

    void SetHost(const std::string& sHost)
    {
        m_sHost = sHost;
    }

    void SetPort(uint32_t uiPort)
    {
        m_uiPort = uiPort;
    }

public:
	const std::string& GetCmd() const
	{
	    return(m_strCmd);
	}

	const std::string& GetHashKey() const
	{
	    return(m_strHashKey);
	}

	const std::vector<std::pair<std::string, bool> >& GetCmdArguments() const
    {
	    return(m_vecCmdArguments);
    }
    
    const std::string& GetHost() const 
    {
        return m_sHost;
    }
    const uint32_t GetPort() const
    {
        return m_uiPort;
    }
private:
	std::string m_strErr;
	std::string m_strHashKey;
	std::string m_strCmd;
	std::vector<std::pair<std::string, bool> > m_vecCmdArguments;
    std::string m_sHost;
    uint32_t    m_uiPort;
};

} /* namespace loss */

#endif /* SRC_DBI_REDIS_REDISCMD_HPP_ */
