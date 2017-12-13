#ifndef __LIB_API_H__
#define __LIB_API_H__

#include <stdint.h>
#include "lib_err.h"
#include "lib_macro.h"
#include "lib_buffer.h"

namespace LIB_COMM {

//////
class LibApi {
public:
    enum
    {
        TM_CONN  = 3,
        TM_READ  = 10,
        TM_WRITE = 10
    };

    LibApi() {
        set_errcode(SUCCESS);
        set_connect_timeout(TM_CONN);
        set_read_timeout(TM_READ);
        set_write_timeout(TM_WRITE);
        set_need_debug(false);
    }

    virtual ~LibApi() {
    }
    //
    int32_t  errcode() const { return errcode_; }
    uint32_t connect_timeout() const { return connect_timeout_; }
    uint32_t read_timeout() const { return read_timeout_; }
    uint32_t write_timeout() const { return write_timeout_; }
    bool need_debug() const { return need_debug_; }
    //
    void set_errcode(int32_t errcode) { errcode_ = errcode; }
    void set_connect_timeout(uint32_t connect_timeout) {
        connect_timeout_ = connect_timeout;
    }
    void set_read_timeout(uint32_t read_timeout) {
        read_timeout_ = read_timeout;
    }
    void set_write_timeout(uint32_t write_timeout) {
        write_timeout_ = write_timeout;
    }
    void set_need_debug(bool need_debug) { need_debug_ = need_debug; }

    void ClearErrmsg(int32_t errcode = SUCCESS) {
        set_errcode(errcode);
        buffer_error_.Clear();
        buffer_debug_.Clear();
    }

    int Resize(uint32_t size) {
        buffer_error_.Init(size);
        buffer_debug_.Init(size);

        return SUCCESS;
    }

    int RecordErrmsg(const char* format, ...) {
        va_list arg;
        va_start(arg, format);

        buffer_error_.Append(format, arg);

        va_end(arg);

        return SUCCESS;
    }

    int RecordDebugmsg(const char* format, ...) {
        if(!need_debug()) { return SUCCESS; }

        va_list arg;
        va_start(arg, format);

        buffer_debug_.Append(format, arg);

        va_end(arg);

        return SUCCESS;
    }

    char* Errmsg() {
        return buffer_error_.buffer();
    }

    char* Debugmsg() {
        return buffer_debug_.buffer();
    }

private:
    bool        need_debug_;
    int32_t     errcode_;
    uint32_t    connect_timeout_;
    uint32_t    read_timeout_;
    uint32_t    write_timeout_;
    LibBuffer   buffer_error_;
    LibBuffer   buffer_debug_;
};
/////

}
#endif 
