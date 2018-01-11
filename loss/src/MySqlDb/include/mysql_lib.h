#ifndef _H_MYSQL_LIB_H_
#define _H_MYSQL_LIB_H_

#include <stdint.h>
#include "lib_common.h"
#include "lib_string.h"
#include "mysql.h"

namespace MYSQLUSE {

class MysqlSql {
public:
    typedef enum Cmp{
        EQ = 1,         // =
        NE = 2,         // !=
        LT = 3,         // <=
        GT = 4,         // >=
        LE = 5,         // <
        GE = 6          // >
    }CMP;

    explicit MysqlSql(uint32_t length) : length_(length) {
      sql_.reserve(length);
    }
    virtual ~MysqlSql()  {
    }
    
    void set_length(uint32_t length) {
      Clear();
      sql_.reserve(length);
      length_ = length;
    }
    MysqlSql& Where() {
      sql_ += " where ";
      return *this;
    }
    MysqlSql& BracketsBegin() {
      sql_ += " ( ";
      return *this;
    }

    MysqlSql& BracketsEnd() {
      sql_ += " ) ";
      return *this;
    }

    MysqlSql& And() {
      sql_ += " and ";
      return *this;
    }
    MysqlSql& Or() {
      sql_ += " or ";
      return *this;
    }

    template<class T>
    MysqlSql& Condition(const std::string& field,
                        const T& value,
                        CMP cmp = EQ) {
      sql_ += field + Compare(cmp) + LibString::type2str(value);
      return *this;
    }

    MysqlSql& Condition(const std::string& field,
                                  const std::string& value,
                                  MysqlSql::CMP cmp)  {
      sql_ += field + Compare(cmp) + "'" + value + "'";
      return *this;
    }

    MysqlSql& Condition(const std::string& condition) {
      sql_ += condition;
      return *this;
    }
    MysqlSql& In(const std::string& field, const std::string& value) {
      sql_ += field + " IN(" + value + ") ";
      return *this;
    }
    MysqlSql& Limit(const std::string& limit) {
      sql_ += std::string(" limit ") + limit;
      return *this;
    }
    MysqlSql& Limit(const int64_t begin, const int64_t num) {
      std::string limit = LibString::type2str(begin) + "," +
          LibString::type2str(num);
      return Limit(limit);
    }
    MysqlSql& Limit(const int64_t num) {
      return Limit(0, num);
    }
    virtual void Clear() {
      sql_.clear();
    }
    const std::string& sql() const  { return sql_; }
protected:
    void set_table(const std::string& table) { table_ = table; }
    std::string Compare(CMP cmp) {
      switch(cmp) {
        case EQ: {
          return "=";
        }
        case NE: {
          return "!=";
        }
        case LT: {
          return "<=";
        }
        case GT: {
          return ">=";
        }
        case LE: {
          return "<";
        }
        case GE: {
          return ">";
        }
        default: {
          return "=";
        }
      }
    }
protected:
    std::string sql_;
private:
    uint32_t    length_;
    std::string table_;
};

class MysqlSelect : public MysqlSql {
public:
    explicit MysqlSelect(uint32_t length = SIZE_K): MysqlSql(length) {
      sql_ = "select ";
    }
    ~MysqlSelect()  {
    }

    MysqlSelect& Field(const std::string& field) {
      sql_ += field + ",";
      uint32_t size = field_index_.size();
      field_index_[field] = size;

      return *this;
    }

    // do not include space, and use "," separate
    MysqlSelect& MultiField(const std::string& multi_field) {
      std::vector<std::string> vec;
      LibString::str2vec(multi_field, vec);

      for (uint32_t i = 0; i < vec.size(); i++) {
        Field(vec[i]);
      }

      return *this;
    }
    MysqlSelect& From(const std::string& table) {
      sql_.erase(sql_.end() - 1); //pop last ','
      sql_ += " from " + table;

      set_table(table);
      return *this;
    }
    MysqlSelect& OrderBy(const std::string& orderby) {
      sql_ += std::string(" order by ") + orderby;
      return *this;
    }

    MysqlSelect& OrderByDesc(const std::string& orderby) {
      sql_ += std::string(" order by ") + orderby + " desc";
      return *this;
    }

    int32_t FieldIndex(const std::string& field) {
      std::map<std::string, int32_t>::iterator it = field_index_.find(field);
      if (it != field_index_.end()) { return it->second; }
      return -1;
    }

    virtual void Clear() {
      MysqlSql::Clear();
      field_index_.clear();
    }
private:
    std::map<std::string, int32_t> field_index_;
};

class MysqlInsert : public MysqlSql {
public:
    explicit MysqlInsert(const std::string& table,
                         uint32_t length = SIZE_K) : MysqlSql(length) {
      sql_ = std::string("insert into ") + table;
    }
    virtual ~MysqlInsert() { }
    
    template<class T>
    MysqlInsert& Field(const std::string& field,
                       const T& value) {
      field_ += field + ",";
      value_ += LibString::type2str<T>(value) + ",";
      return *this;
    }

    MysqlInsert& Field(const std::string& field,
                       const std::string& value) {
      field_ += field + "," ;
      value_ += std::string("'") + value + "',";
      return *this;
    }

    void End()  {
      field_.erase(field_.end() - 1);
      value_.erase(value_.end() - 1);
      sql_ += std::string(" (") + field_ + ") values (" + value_ + ")";
    }
    virtual void Clear() {
      MysqlSql::Clear();
      field_ = "";
      value_ = "";
    }
private:
    std::string field_;
    std::string value_;
};

class MysqlReplace : public MysqlSql {
public:
    explicit MysqlReplace(const std::string& table,
                          uint32_t length = SIZE_K) : MysqlSql(length) {
      sql_ = std::string("replace into ") + table;
    }
    ~MysqlReplace() { }

    template<class T>
    MysqlReplace& Field(const std::string& field,
                        const T& value) {
      field_ += field + ",";
      value_ += LibString::type2str(value) + ",";
      return *this;
    }

    MysqlReplace& Field(const std::string& field, 
                        const std::string& value) {
      field_ += field + "," ;
      value_ += std::string("'") + value + "',";
      return *this;
    }

    //
    void End() {
      field_.erase(field_.end() - 1);
      value_.erase(value_.end() - 1);
      sql_ += std::string(" (") + field_ + ") values (" + value_ + ")";
    }

    virtual void Clear() {
      MysqlSql::Clear();
      field_ = "";
      value_ = "";
    }
private:
    std::string field_;
    std::string value_;
};

//////////////////
class MysqlUpdate : public MysqlSql {
public:
    explicit MysqlUpdate(const std::string& table,
                         uint32_t length = SIZE_K): MysqlSql(length) {
      sql_ = std::string("update ") + table + " set ";
      set_first_field(true);
    }
    ~MysqlUpdate() { }

    template<class T>
    MysqlUpdate& Field(const std::string& field,const T& value) {
      if (!first_field()) { sql_ += ","; }
      Condition<T>(field, value, MysqlSql::EQ);
      set_first_field(false);
      return *this;
    }

    MysqlUpdate& Field(const std::string& field, const std::string& value) {
      if (!first_field()) { sql_ += ","; }
      Condition<std::string>(field, value, MysqlSql::EQ);
      set_first_field(false);
      return *this;
    }
    //
    MysqlUpdate& Field(const std::string& field) {
      if (!first_field()) { sql_ += ","; }
      sql_ += field;
      set_first_field(false);
      return *this;
    }
    virtual void Clear() {
      MysqlSql::Clear();
      set_first_field(true);
    }
    void set_first_field(bool first_field) { first_field_ = first_field; }
    bool first_field() { return first_field_; } 
private:
    bool first_field_;
};

class MysqlDelete : public MysqlSql {
public:
    explicit MysqlDelete(const std::string& table,
                         uint32_t length = SIZE_K): MysqlSql(length) {
      sql_ = std::string("delete from ") + table + " ";
    }

    ~MysqlDelete()  {
      MysqlSql::Clear();
    }
};

class MysqlRes {
public:
    friend class MysqlOp;

    MysqlRes() 
        : res_(NULL),row_(NULL),num_rows_(0),num_fields_(0), field_index_(0) {
        }
    ~MysqlRes()  {
      Clear();
    }

    int32_t FetchRow()  {
      if (res_ == NULL) { return ERR_MYSQL_STORE_RESULT; };
      row_ = mysql_fetch_row(res_);
      if (row_ == NULL) { return ERR_MYSQL_FETCH_ROW; }
      lengths_ = mysql_fetch_lengths(res_);
      field_index_ = 0;
      return SUCCESS;
    }
    std::string FieldStr(uint32_t index) {
      if ((NULL == (*(row_ + index))) || (index >= num_fields_)) { return ""; }
      return std::string((char *)(*(row_ + index)),(size_t)lengths_[index]);
    }
    std::string FieldStr() {
      return FieldStr(field_index_++);
    }
    const char* Field(uint32_t index)  {
      if ((NULL == (*(row_ + index))) || (index >= num_fields_)) { return ""; }
      return (char *)(*(row_ + index));
    }
    //
    const char* Field() {
      return Field(field_index_++);
    }
protected:
    int32_t set_res(MYSQL_RES* res) {
      if (res == NULL) { return ERR_MYSQL_STORE_RESULT; }
      if (res_ != NULL) { mysql_free_result(res_); }

      res_ = res;
      set_num_fields(mysql_num_fields(res));
      set_num_rows(mysql_num_rows(res));

      return SUCCESS;
    }
private:
    void Clear() {
      if (res_ != NULL) { mysql_free_result(res_); }
      res_ = NULL;
      set_row(NULL);
      set_num_rows(0);
      set_num_fields(0);
    }
public:
    uint32_t num_rows() { return num_rows_; } 
    uint32_t num_fields() {return num_fields_;} 
    uint32_t field_index() {return field_index_;}

    void set_num_rows(uint32_t num_rows) { num_rows_ = num_rows; } 
    void set_num_fields(uint32_t num_fields) { num_fields_ = num_fields; } 
    void set_field_index(uint32_t field_index) { field_index_ = field_index; }
private:
    void set_row(MYSQL_ROW row) { row_ = row; }
private:
    MYSQL_RES*      res_;
    MYSQL_ROW       row_;
    uint32_t    num_rows_;
    uint32_t    num_fields_;
    uint32_t    field_index_;
    unsigned long* lengths_;
};

class MysqlOp : public LibApi {
public:
    MysqlOp() {
      res_           = NULL;
      connected_     = false;
      charset_       = "utf8";
      escape_        = NULL;
      escape_length_ = SIZE_K;
      CreateEscape();
    }

    MysqlOp(const std::string& host, const std::string& user,
            const std::string& password, int32_t port = 3306) {
      new(this) MysqlOp();
      host_     = host;
      user_     = user;
      password_ = password;
      port_     = port;

    }
    //
    ~MysqlOp() {
      ClearEscape();
      Clear();
      Close();
    }

    //int32_t Init(const MylibIni& ini, const std::string& section = "mysql"); 
    int32_t Connect() {
      MYSQL* mysql = mysql_init(&mysql_);
      IF_RET((mysql == NULL), ERR_MYSQL_INIT);

      my_bool reconn = 1;
      mysql_options(&mysql_, MYSQL_OPT_RECONNECT, (const char *)&reconn);

      uint32_t connect_tm = connect_timeout();
      mysql_options(&mysql_, MYSQL_OPT_CONNECT_TIMEOUT,
                    (const char*)&connect_tm);

      uint32_t read_tm = read_timeout();
      mysql_options(&mysql_, MYSQL_OPT_READ_TIMEOUT, (const char *)&read_tm);

      uint32_t write_tm = write_timeout();
      mysql_options(&mysql_, MYSQL_OPT_WRITE_TIMEOUT,
                    (const char*)&write_tm);

      mysql = mysql_real_connect(&mysql_,
                                 host_.c_str(),
                                 user_.c_str(),
                                 password_.c_str(),
                                 NULL,
                                 port_,
                                 NULL,
                                 CLIENT_FOUND_ROWS|CLIENT_MULTI_RESULTS);//|CLIENT_MULTI_STATEMENTS
      if (mysql == NULL) {
        RecordErrmsg("connect failed||ret:%d||errmsg:%s",
                     mysql_errno(&mysql_), mysql_error(&mysql_));

        mysql_close(&mysql_);
        return ERR_MYSQL_REAL_CONNECT;
      }

      IFNOT_RET(CreateEscape(), SUCCESS);

      connected_ = true;
      CharsetSet();

      return SUCCESS;
    }

    int32_t Query(const std::string& sql)  {
      Clear();

      if (!connected_) {
        IFNOT_RET(Connect(), SUCCESS);
      }

      if (0 != mysql_ping(&mysql_)) {
        Close();
        IFNOT_RET(Connect(), SUCCESS);
      }

      int32_t ret = mysql_real_query(&mysql_, sql.c_str(), sql.length());
      if (ret != 0) {
        RecordErrmsg("real query failed||ret:%d||errmsg:%s||sql:%s",
                     mysql_errno(&mysql_), mysql_error(&mysql_),
                     sql.c_str());
        return ERR_MYSQL_REAL_QUERY;
      }
#ifdef MYSQL_DEBUG
      printf("sql:[%s]\n", sql.c_str());
#endif

      res_ = mysql_store_result(&mysql_);
      if (res_ == NULL && mysql_field_count(&mysql_) != 0) {
        RecordErrmsg("store failed||ret:%d||errmsg:%s",
                     mysql_errno(&mysql_), mysql_error(&mysql_));
        return ERR_MYSQL_STORE_RESULT;
      }

      return SUCCESS;
    }
    int32_t Query(const std::string& sql,MysqlRes& res) {
      IFNOT_RET(Query(sql), SUCCESS);
      int32_t ret = 0;
      if (SUCCESS != (ret = res.set_res(res_))) {
        RecordErrmsg("res.set_res failed:%d||%s",ret,sql.c_str());
        return ret;
      }
      // IFNOT_RET(res.set_res(res_), SUCCESS);
      set_res(NULL);
      return SUCCESS;
    }

    int32_t Query(const MysqlSql* sql)  {
      return Query(sql->sql());
    }
    int32_t Query(const MysqlSql* sql, MysqlRes& res) {
      return Query(sql->sql(), res);
    }
    uint64_t InsertId() {
      return mysql_insert_id(&mysql_);
    }
    uint64_t AffectedRows() {
      return mysql_affected_rows(&mysql_);
    }
    int32_t EscapeString(const std::string& src, std::string& dst) {
      if (!connected_) {
        IFNOT_RET(Connect(), 0);
      }

      size_t src_len = src.size();//strlen(src.c_str());
      if (src_len == 0) {
        return SUCCESS;
      }

      uint32_t dst_len = src_len * 2 + 1;

      if (dst_len >= escape_length_) {
        escape_length_ = dst_len;
        IFNOT_RET(CreateEscape(), SUCCESS);
      }

      memset(escape_, 0, escape_length_);
      mysql_real_escape_string(&mysql_, escape_, src.c_str(), src_len);
      dst = escape_;

      return SUCCESS;
    }
private:
    void Clear() {
      if (res_ != NULL) {
        mysql_free_result(res_);
        res_ = NULL;
      }
    }
    int32_t Close() {
      if (connected_) {
        mysql_close(&mysql_);
        connected_ = false;
      }
      return SUCCESS;
    }
    void ClearEscape() {
      if (escape_ != NULL) {
        free(escape_);
        escape_ = NULL;
      }
    }
    int32_t CreateEscape() {
      ClearEscape();

      escape_ = (char*)malloc(escape_length_);
      if (escape_ == NULL) { return ERR_SYS_MALLOC; }
      memset(escape_, 0, escape_length_);

      return SUCCESS;
    }

public:
    std::string& host() { return host_; }
    int32_t port() { return port_; }
    std::string& user() { return user_; }
    std::string& password() { return password_; }
    std::string& charset() { return charset_; } 
    MYSQL& mysql() { return mysql_; }
    bool connected() { return connected_; }
    MYSQL_RES* res() { return res_; } 

    void set_host(const std::string& host)  { host_ = host; }
    void set_port(int32_t port)  { port_ = port; }
    void set_user(const std::string& user) { user_ = user; }
    void set_password(const std::string& password) { password_ = password; }
    void set_charset(const std::string& charset) { charset_ = charset; }
private:
    void set_connected(bool connected)  { connected_ = connected; }
    void set_res(MYSQL_RES* res)  { res_ = res; }
private:
    void CharsetSet() {
      if (!charset_.empty()) {
        mysql_set_character_set(&mysql_, charset_.c_str());
      }
    }
private:
    std::string     host_;
    int32_t         port_;
    std::string     user_;
    std::string     password_;
    std::string     charset_;

    MYSQL           mysql_;
    bool            connected_;

    MYSQL_RES*      res_;
    char*           escape_;
    uint32_t        escape_length_;
};

class MTransaction {
public:
    MTransaction() { 
      commited_ = false;
      mysql_ = NULL;
    }
    //
    MTransaction(MysqlOp& mysql) {
      //set_mysql(mysql);
      mysql_->Query("begin;");
    }
    virtual ~MTransaction() {
      if (!commited()) {
        mysql_->Query("rollback;");
      }
    }
    //
    void set_mysql(MysqlOp& mysql) {
      mysql_ = &mysql;
      set_commited(false);
    }
    //
    void commit() {
      mysql_->Query("commit;");
      set_commited(true);
    }
public:
    bool commited() { return commited_; }
    void set_commited(bool commited) { commited_ = commited; }
private:
    bool commited_;
    MysqlOp*  mysql_;
};

//////////////////////
//////////////////////
#define MYSQL_GET_ONE_ROW(mysql, res, ss) \
    do {\
      int ret = mysql.Query(ss.str(),res); \
      if (mysql.Query(ss.str(),res) != 0) \
      { \
        LOG_ERROR("mysql Query fail!||ret:%d|msg:%s", ret, mysql.Errmsg()); \
        RETURN(-1); \
      } \
      if (res.num_rows() != 1) \
      { \
        LOG_ERROR("rows not 1:num_rows:%u", res.num_rows()); \
        RETURN(-2); \
      } \
      ret = res.FetchRow(); \
      if (ret != 0) \
      { \
        LOG_ERROR("mysql FetchRow fail!ret:%d",ret); \
        RETURN(-3); \
      } \
    } while(0)
/////////////////////////////////////////////////////////////////
}
//
#endif 
