/*******************************************************************************
* Project:  DataAnalysis
* File:     MysqlDbi.cpp
* Description: MySQL DB操作接口类
* Author:         
* Created date:  2010-12-21
* Modify history: adolphsun 2011-06-16
*******************************************************************************/

#include "MysqlDbi.hpp"

using namespace std;

//将str中的所有old_value子串替换为new_value
void StringReplace(string &str, const string &old_value,const string &new_value)
{
    for(string::size_type pos=0; pos!=string::npos; pos+=new_value.length())
    {
    	if((pos=str.find(old_value, pos)) != string::npos)
        {
            str.replace(pos, old_value.length(), new_value);
        }
        else
        {
                break;
        }
    }
}

loss::CMysqlDbi::CMysqlDbi()
{
	InitVariables();
}

loss::CMysqlDbi::CMysqlDbi(
        const char* szIp,
        const char* szUserName,
        const char* szUserPwd,
        const char* szDbName,
        const char* szCharacterSet,
        unsigned int uiPort)
{
	InitVariables();

	strncpy(m_stDbConfDetail.m_stDbConnInfo.m_szDbCharSet, szCharacterSet, 16);
	strncpy(m_stDbConfDetail.m_stDbConnInfo.m_szDbHost, szIp, 32);
	strncpy(m_stDbConfDetail.m_stDbConnInfo.m_szDbUser, szUserName, 32);
	strncpy(m_stDbConfDetail.m_stDbConnInfo.m_szDbPwd, szUserPwd, 32);
	strncpy(m_stDbConfDetail.m_stDbConnInfo.m_szDbName, szDbName, 32);
	m_stDbConfDetail.m_stDbConnInfo.m_uiDbPort = uiPort;
	m_stDbConfDetail.m_ucDbType = loss::MYSQL_DB;
	m_stDbConfDetail.m_ucAccess = 1;	//直连

    InitDbConn(m_stDbConfDetail);
}

loss::CMysqlDbi::~CMysqlDbi()
{
		FreeResult();
        mysql_close(&m_dbInstance);
}

int loss::CMysqlDbi::InitDbConn(const loss::tagDbConfDetail& stDbConfDetail)
{
	FreeResult();
	InitVariables();

	mysql_close(&m_dbInstance);

    if (!mysql_init(&m_dbInstance))
    {
        m_iErrno = -1;
        m_strError = "no memory, mysql_init failed!";
        return -1;
    }
   //设置连接超时
    unsigned int uiTimeOut=30;
    mysql_options(&m_dbInstance, MYSQL_OPT_CONNECT_TIMEOUT, reinterpret_cast<char *>(&uiTimeOut));

    mysql_options(&m_dbInstance, MYSQL_OPT_COMPRESS, NULL);           //设置传输数据压缩
    mysql_options(&m_dbInstance, MYSQL_OPT_LOCAL_INFILE, NULL);       //设置允许使用LOAD FILE

    //设置自动重连
    bool bBool = 1;
    mysql_options(&m_dbInstance, MYSQL_OPT_RECONNECT, reinterpret_cast<char *>(&bBool));

    if (!mysql_real_connect(&m_dbInstance, stDbConfDetail.m_stDbConnInfo.m_szDbHost,
    		stDbConfDetail.m_stDbConnInfo.m_szDbUser, stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
    		stDbConfDetail.m_stDbConnInfo.m_szDbName, stDbConfDetail.m_stDbConnInfo.m_uiDbPort,
    		NULL, 0))
    {
        m_iErrno = mysql_errno(&m_dbInstance);
        m_strError = mysql_error(&m_dbInstance);
        return -2;
    }
    SetDbConnCharacterSet(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet);

    m_stDbConfDetail = stDbConfDetail;  // 用于丢失连接时重连

    return 0;
}

int loss::CMysqlDbi::MysqlInit()
{
    if (!mysql_init(&m_dbInstance))
    {
        m_iErrno = -1;
        m_strError = "no memory, mysql_init failed!";
        return -1;
    }
    return 0;
}

int loss::CMysqlDbi::SetMysqlOptions(
        enum mysql_option option,
        const char* arg)
{
    return mysql_options(&m_dbInstance, option, arg);
}

int loss::CMysqlDbi::MysqlRealConnect(
        const char* szIp,
        const char* szUserName,
        const char* szUserPwd,
        const char* szDbName,
        unsigned int uiPort,
        const char* szUnixSocket,
        unsigned long ulClientFlag)
{
    if (!mysql_real_connect(&m_dbInstance, szIp, szUserName,
                szUserPwd, szDbName, uiPort, szUnixSocket, ulClientFlag))
    {
        m_iErrno = mysql_errno(&m_dbInstance);
        m_strError = mysql_error(&m_dbInstance);
        return -2;
    }
    return 0;
}

int loss::CMysqlDbi::SetDbConnCharacterSet(const char* szCharacterSet)
{
    int ret = 0;
    ret = mysql_set_character_set(&m_dbInstance, szCharacterSet); //mysql 5.0 lib
    /* mysql 4.0 lib
    char set_character_string[32] = {0};
    sprintf(set_character_string, "set names %s", character_set);
    ret = mysql_real_query(&mysql_, set_character_string, strlen(set_character_string));
    */
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);
    return ret;
}

int loss::CMysqlDbi::EscapeString(
        char* szTo,
        const char* szFrom,
        unsigned long uiLength)
{
    return mysql_real_escape_string(&m_dbInstance, szTo, szFrom, uiLength);
}

int loss::CMysqlDbi::MysqlPing()
{
    return mysql_ping(&m_dbInstance);
}

int loss::CMysqlDbi::SetAutoCommit(my_bool mode)
{
    int ret = 0;
    ret = mysql_autocommit(&m_dbInstance, mode);
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);
    return ret;
}

int loss::CMysqlDbi::Commit()
{
    int ret = 0;
    ret = mysql_commit(&m_dbInstance);
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);
    return ret;
}

int loss::CMysqlDbi::ExecSql(const std::string& strSql)
{
    if (!CleanBeforeExecSql())
    {
    	return m_iQueryResult;
    }

    m_iQueryResult = mysql_real_query(&m_dbInstance, strSql.c_str(), strSql.size());
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);

    /*
     * 2006 (CR_SERVER_GONE_ERROR) : MySQL服务器不可用
     * 2013 (CR_SERVER_LOST) : 查询过程中丢失了与MySQL服务器的连接
     * mysql_ping 这个函数的返回值和文档说明有出入，返回0并不能保证连接是可用的
     */
    if (mysql_errno(&m_dbInstance) == 2006 ||
    		mysql_errno(&m_dbInstance) == 2013)
    {
    	if (InitDbConn(m_stDbConfDetail) != 0)
    	{
    		return m_iErrno;
    	}

    	 m_iQueryResult = mysql_real_query(&m_dbInstance, strSql.c_str(), strSql.size());
    	 m_iErrno = mysql_errno(&m_dbInstance);
    	 m_strError = mysql_error(&m_dbInstance);
    }

    return m_iQueryResult;
}

int loss::CMysqlDbi::ExecSql(
        const std::string& strSql,
        MYSQL_RES* pQueryRes)
{
    ExecSql(strSql);
    if (m_iQueryResult == 0)
    {
        pQueryRes = mysql_store_result(&m_dbInstance);
        m_iErrno = mysql_errno(&m_dbInstance);
        m_strError = mysql_error(&m_dbInstance);

        if (m_iErrno != 0) {
        	return m_iErrno;
        }
    }
    else
    {
        pQueryRes = NULL;
    }

    return m_iQueryResult;
}

int loss::CMysqlDbi::ExecSql(
        const std::string& strSql,
        T_vecResultSet& vecRes)
{
    ExecSql(strSql);
    if (m_iQueryResult == 0)
    {
        m_pQueryRes = mysql_store_result(&m_dbInstance);
        m_iErrno = mysql_errno(&m_dbInstance);
        m_strError = mysql_error(&m_dbInstance);
        if (m_iErrno == 0)
        {
            MYSQL_FIELD* fields = mysql_fetch_fields(m_pQueryRes);
            int num_field = mysql_num_fields(m_pQueryRes);
            while ((m_stRow = mysql_fetch_row(m_pQueryRes)) != NULL)
            {
                T_mapRow mapRow;
                for (int i = 0; i < num_field; i++)
                {
                    if (m_stRow[i] != NULL)
                    {
                        mapRow[fields[i].name] = m_stRow[i];
                    }
                    else
                    {
                        mapRow[fields[i].name] = "";
                    }
                }
                vecRes.push_back(mapRow);
            }
        }
        else
        {
            return m_iErrno;
        }
    }

    return m_iQueryResult;
}

//插入数据记录
int loss::CMysqlDbi::ExecSql(
        const std::string& strSql,
        unsigned long long& ullInsertId)
{
	if (!CleanBeforeExecSql())
	{
		return m_iQueryResult;
	}

    m_iQueryResult = mysql_real_query(&m_dbInstance, strSql.c_str(), strSql.size());
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);

    if (mysql_errno(&m_dbInstance) == 2006 ||
     		mysql_errno(&m_dbInstance) == 2013)
     {
     	if (InitDbConn(m_stDbConfDetail) != 0)
     	{
     		return m_iErrno;
     	}

     	 m_iQueryResult = mysql_real_query(&m_dbInstance, strSql.c_str(), strSql.size());
     	 m_iErrno = mysql_errno(&m_dbInstance);
     	 m_strError = mysql_error(&m_dbInstance);
     }

    ullInsertId = mysql_insert_id(&m_dbInstance);
    return m_iQueryResult;
}

unsigned int loss::CMysqlDbi::AffectRows()
{
	return(mysql_affected_rows(&m_dbInstance));
}

const MYSQL_RES* loss::CMysqlDbi::StoreResult()
{
    if (m_iErrno != 0)  // 查询是失败的
    {
        return NULL;
    }

    if (m_pQueryRes != NULL)
    {
        return m_pQueryRes;
    }

    m_pQueryRes = mysql_store_result(&m_dbInstance);
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);

    return m_pQueryRes;
}

const MYSQL_RES* loss::CMysqlDbi::UseResult()
{
	if (m_iErrno != 0)  // 查询是失败的
	{
	    return NULL;
	}

    if (m_pQueryRes != NULL)
    {
        return m_pQueryRes;
    }

    m_pQueryRes = mysql_use_result(&m_dbInstance);
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);

    return m_pQueryRes;
}

int loss::CMysqlDbi::GetResultSet(T_vecResultSet& vecResultSet)
{
	vecResultSet.clear();
	if (m_iQueryResult == 0)
    {
        m_pQueryRes = mysql_store_result(&m_dbInstance);
        m_iErrno = mysql_errno(&m_dbInstance);
        m_strError = mysql_error(&m_dbInstance);
        if (m_iErrno == 0)
        {
            MYSQL_FIELD* fields = mysql_fetch_fields(m_pQueryRes);
            int num_field = mysql_num_fields(m_pQueryRes);
            while ((m_stRow = mysql_fetch_row(m_pQueryRes)) != NULL)
            {
                T_mapRow mapRow;
                for (int i = 0; i < num_field; i++)
                {
                    if (m_stRow[i] != NULL)
                    {
                        mapRow[fields[i].name] = m_stRow[i];
                    }
                    else
                    {
                        mapRow[fields[i].name] = "";
                    }
                }
                vecResultSet.push_back(mapRow);
            }
        }
        else
        {
            return m_iErrno;
        }
    }

    return m_iQueryResult;
}

int loss::CMysqlDbi::GetResultToFile()
{
	 //查询失败，取不了结果集
	 if (m_iQueryResult != 0)
	 {
		 return m_iQueryResult;
	 }

	  m_pQueryRes = mysql_use_result(&m_dbInstance);
	  m_iErrno = mysql_errno(&m_dbInstance);
	  m_strError = mysql_error(&m_dbInstance);

	  if (m_iErrno == 0)
	  {
		  //默认会清空文件，刚好是需要的
		  ofstream fOut(m_strTempFileName.c_str());
		  if (!fOut.good())
		  {
			  return -1;
		  }

		  MYSQL_FIELD* fields = mysql_fetch_fields(m_pQueryRes);
		  int num_field = mysql_num_fields(m_pQueryRes);
		  if (fields == NULL || num_field == 0)
		  {
			  return -2;
		  }

		  //保存数据字典，用于从文件读数据
		  for (int i = 0; i < num_field; i++)
		  {
			  m_vecDict.push_back(std::string(fields[i].name));
		  }

		  string field;
		  while ((m_stRow = mysql_fetch_row(m_pQueryRes)) != NULL)
		  {
			  for (int i = 0; i < num_field; i++)
			  {
				  if (m_stRow[i] != NULL)
				  {
					  field = m_stRow[i];
					  StringReplace(field, "\n", "");
				  }
				  else
				  {
					  field = "";
				  }

				  fOut << field.size() << " " << field << " ";
			  }
			  fOut << "\n";
		  }
		  fOut.close();

		  return m_iQueryResult;
	  }
	  else
	  {
		  return m_iErrno;
	  }
}

loss::T_mapRow& loss::CMysqlDbi::GetRowFromFile()
{
	m_mapRow.clear();

//	FILE *fp = fopen(m_strTempFileName.c_str(), "r"); //文件打不开怎么办？
//	char lineBuff[1024*10];
//	while (fgets(lineBuff, 10240, fp) != NULL)
//	{
//			int size = 0;
//			string strOut;
//			stringstream string2Fields(lineBuff);
//			for (int i=0; i<4; ++i)
//			{
//					strOut.clear();
//					string2Fields >> size;
//
//					if (size==0)
//					{
//							cout << "该字段为空" << endl;
//					}
//					else
//					{       strOut.resize(size);
//							string2Fields.get();
//							string2Fields.read(const_cast<char *>(strOut.data()), size);
//							cout << strOut << endl;
//					}
//			}
//	}
//
//	fclose(fp);

	return m_mapRow;

}

int loss::CMysqlDbi::GetResultDict(T_vecDict& vecDict)
{
	m_vecDict.clear();

	if (0 == mysql_field_count(&m_dbInstance))	// 无结果集
	{
		return 0;
	}
	//已经取回结果集
	if (m_pQueryRes != NULL)
	{
		vecDict = m_vecDict;
		return 0;
	}

	//上一次查询是失败的
	if (m_iQueryResult != 0)
	{
		vecDict = m_vecDict;
		return m_iQueryResult;
	}

	//从服务器取结果集
	m_pQueryRes = mysql_use_result(&m_dbInstance);
	m_iErrno = mysql_errno(&m_dbInstance);
	m_strError = mysql_error(&m_dbInstance);

	if (m_iErrno == 0)
	{
		MYSQL_FIELD* fields = mysql_fetch_fields(m_pQueryRes);
		int num_field = mysql_num_fields(m_pQueryRes);
		for (int i = 0; i < num_field; i++)
		{
			m_vecDict.push_back(std::string(fields[i].name));
		}

		vecDict = m_vecDict;
		return 0;
	}
	else
	{
		vecDict = m_vecDict;
		return m_iErrno;
	}
}

int loss::CMysqlDbi::GetResultRow(T_mapRow& mapRow)
{
	mapRow.clear();

	if (0 == mysql_field_count(&m_dbInstance))	// 无结果集
	{
		return 0;
	}

    if (m_pQueryRes == NULL)
    {
        return 0;
    }

    m_stRow = mysql_fetch_row(m_pQueryRes);
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);

    if (m_iErrno != 0)
    {
    	return m_iErrno;
    }

    if (m_stRow == NULL)
    {
    	return 0;
    }

    for (size_t i = 0; i < m_vecDict.size(); i++)
    {
        if (m_stRow[i] == NULL)
        {
            mapRow[m_vecDict[i]] = "";
        }
        else
        {
            mapRow[m_vecDict[i]] = m_stRow[i];
        }
    }

    return 0;
}

MYSQL_ROW loss::CMysqlDbi::GetRow()
{
    if (m_pQueryRes == NULL)
    {
        return NULL;
    }
    m_stRow = mysql_fetch_row(m_pQueryRes);
    m_iErrno = mysql_errno(&m_dbInstance);
    m_strError = mysql_error(&m_dbInstance);
    return m_stRow;
}

unsigned long* loss::CMysqlDbi::FetchLengths()
{
    if (m_pQueryRes != NULL)
    {
        return mysql_fetch_lengths(m_pQueryRes);
    }
    return 0;
}

unsigned int loss::CMysqlDbi::FetchFieldNum()
{
    if (m_pQueryRes != NULL)
    {
        return mysql_num_fields(m_pQueryRes);
    }
    return 0;
}

MYSQL_FIELD* loss::CMysqlDbi::FetchFields()
{
    if (m_pQueryRes != NULL)
    {
        return mysql_fetch_fields(m_pQueryRes);
    }
    return NULL;
}

unsigned int loss::CMysqlDbi::FetchFieldCount()
{
    return mysql_field_count(&m_dbInstance);
}

void loss::CMysqlDbi::FreeResult()
{
    if (m_pQueryRes != NULL)
    {
        mysql_free_result(m_pQueryRes);
        m_pQueryRes = NULL;

        //mysql_free_result 其实这个函数是不会发生错误的
        m_iErrno = mysql_errno(&m_dbInstance);
        m_strError = mysql_error(&m_dbInstance);
    }
}

const char* loss::CMysqlDbi::MysqlInfo()
{
    return mysql_info(&m_dbInstance);
}

void loss::CMysqlDbi::InitVariables()
{
	m_iQueryResult = 0;
	m_iErrno = 0;
	m_pQueryRes = NULL;
	mysql_init(&m_dbInstance);
}

bool loss::CMysqlDbi::CleanBeforeExecSql()
{
	//测试数据库连接的可用性
	if (MysqlPing() == CR_SERVER_GONE_ERROR)
	{
		//先关闭连接，再重新建立一个连接
		if (InitDbConn(m_stDbConfDetail) != 0)
		{
			m_iQueryResult = -2;
			return false;
		}
	}

	//查询之前应该释放结果集，否则会发生内存泄漏。
	FreeResult();

	return true;
}
