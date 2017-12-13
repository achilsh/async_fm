#ifndef __LIB_ERR_H__
#define __LIB_ERR_H__

namespace LIB_COMM {
  enum {
    SUCCESS = 0,
    /**********************************************************************/
    /* -101 ~ -999: system and mylib error
     * */
    /* mylib system
     * */
    /**********************************************************************/
    ERR_SYS_MALLOC                  = -101,
    ERR_SYS_STAT                    = -102,
    ERR_SYS_FOPEN                   = -103,
    ERR_SYS_OPEN                    = -104,
    ERR_SYS_FREAD                   = -105,
    /**********************************************************************/
    /* mysql                                                              */
    /**********************************************************************/
    ERR_MYSQL_INIT                  = -1201,
    ERR_MYSQL_PING                  = -1202,
    ERR_MYSQL_REAL_CONNECT          = -1203,
    ERR_MYSQL_REAL_QUERY            = -1204,
    ERR_MYSQL_STORE_RESULT          = -1205,
    ERR_MYSQL_FETCH_ROW             = -1206,
    ERR_MYSQL_NUM_ROWS              = -1207,
    ERR_MYSQL_AFFECTD_ROW           = -1208,
    ERR_MYSQL_FIELD_MUST_INCLUDE    = -1221,
    //
  };
}
#endif 
