#include "mysql_lib.h"
#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;

using namespace MYSQLUSE;

int main(int argc, const char** argv) {
  //1.测试各种语句.
  const std::string vsFieldName = "name,age,money";
  const std::string stbName = "test.tb_hzy_app_svr_test";
  //1.2 insert;
  MysqlInsert insertSql(stbName);
  insertSql.Field<std::string>("name", "achilsh");
  insertSql.Field<int>("age", 19);
  insertSql.Field<double>("money", 100.12);
  insertSql.End();
  std::cout <<"insert sql: " << insertSql.sql() << std::endl;
  
  //1.1 select 
  MysqlSelect selectSql;
  selectSql.MultiField(vsFieldName);
  selectSql.From(stbName);
  selectSql.Where();
  selectSql.Condition<std::string>("name","achilsh", MysqlSql::EQ);
  selectSql.And();
  selectSql.Condition("age", 18, MysqlSql::EQ);
  selectSql.OrderBy("money");
  selectSql.Limit(2);
  std::cout << "select sql: " << selectSql.sql() << std::endl;
  //1.3 update
  MysqlUpdate updateSql(stbName);
  updateSql.Field("age",18);
  updateSql.Field("money",20000.012);
  updateSql.Where();
  updateSql.set_first_field(true);
  updateSql.Field<std::string>("name","achilsh");
  std::cout << "update sql: " << updateSql.sql() << std::endl;
  //1.4 delete
  MysqlDelete deleteSql(stbName);
  deleteSql.Where();
  deleteSql.Condition<std::string>("name","jack");
  std::cout << "delete sql: " << deleteSql.sql() << std::endl;
  //2. 测试各种语句的执行
  std::string sHost = "192.168.1.159";
  std::string sUser = "root";
  std::string sPasswd = "huazhiye";
  uint32_t  uiPort = 3306;
  //2.1 insert 操作:
  MysqlOp sqlTest(sHost, sUser, sPasswd, uiPort);
  int iRet = sqlTest.Connect();
  if (iRet != 0) {
    std::cout << "connect failed" << std::endl;
    return 0;
  }
  std::string sInsertSql = insertSql.sql();
  iRet = sqlTest.Query(sInsertSql);
  if (iRet != 0) {
    std::cout << "insert failed, sql: " << sInsertSql << std::endl;
    return 0;
  }

  // 2.2 select 
  {
    std::string sSelectSeql = selectSql.sql();
    MysqlRes retSelectSql;
    iRet =  sqlTest.Query(sSelectSeql, retSelectSql);
    if (iRet != 0) {
      std::cout << "select failed, sql: " << sSelectSeql << std::endl;
      return 0;
    }
    std::cout <<"begin to query, sql: " << sSelectSeql  << std::endl;

    int iRowNum = retSelectSql.num_rows();
    std::cout <<"query ret nums: " << iRowNum << std::endl;
    for (int i = 0; i < iRowNum; ++i) {
      iRet =  retSelectSql.FetchRow();
      if (iRet != 0) {
        std::cout << "get sql flow err" << std::endl;
        continue;
      }
      std::string sNameVal = retSelectSql.Field();
      int iAgeVal = strtoul(retSelectSql.Field(), NULL, 10); 
      double dMoneyVal = atof(retSelectSql.Field());
      std::cout << "name: " << sNameVal << ",age: " << iAgeVal <<
          "money: " << dMoneyVal << std::endl;
    }
  }
  // 2.3 update
  std::string sSqlUpdate = updateSql.sql() ;
  iRet = sqlTest.Query(sSqlUpdate);
  if (iRet != 0) {
    std::cout << "update failed, sql: " << sSqlUpdate <<std::endl;
    return 0;
  }

  { //select after update.
    std::string sSelectSeql = selectSql.sql();
    MysqlRes retSelectSql;
    iRet =  sqlTest.Query(sSelectSeql, retSelectSql);
    if (iRet != 0) {
      std::cout << "select failed, sql: " << sSelectSeql << std::endl;
      return 0;
    }
    std::cout <<"begin to query, sql: " << sSelectSeql  << std::endl;

    int iRowNum = retSelectSql.num_rows();
    std::cout <<"query ret nums: " << iRowNum << std::endl;
    for (int i = 0; i < iRowNum; ++i) {
      iRet =  retSelectSql.FetchRow();
      if (iRet != 0) {
        std::cout << "get sql flow err" << std::endl;
        continue;
      }
      std::string sNameVal = retSelectSql.Field();
      int iAgeVal = strtoul(retSelectSql.Field(), NULL, 10); 
      double dMoneyVal = atof(retSelectSql.Field());
      std::cout << "name: " << sNameVal << ",age: " << iAgeVal <<
          "money: " << dMoneyVal << std::endl;
    }

  }
  //2.4 delete 
  std::string sSqlDel = deleteSql.sql();
  iRet = sqlTest.Query(sSqlDel);
  if (iRet != 0) {
    std::cout <<"del failed, sql:" << sSqlDel << std::endl;
    return 0;
  }



  return 0;
}

