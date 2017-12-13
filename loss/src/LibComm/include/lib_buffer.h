#ifndef __LIB_BUFFER_H__
#define __LIB_BUFFER_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "lib_err.h"

//
namespace LIB_COMM {

class LibBuffer {
 public:
  enum
  {
    SIZE_DEFAULT = 1024,
    SIZE_NORMAL  = 4096,
    SIZE_LARGE   = 10240
  };


 public:
  LibBuffer(uint32_t size = SIZE_DEFAULT)
      :buffer_(NULL), size_(0), length_(0) {
        Init(size);
      }

  ~LibBuffer() {
    Close();
  }

  void Clear() {
    memset(buffer_, 0, size_);
    set_length(0);
  }

  int Init(uint32_t size) {
    size = (size != 0) ? size : SIZE_DEFAULT;
    if (size <= size_) {
      Clear();
      return SUCCESS;
    }

    Close();

    set_size(size);
    buffer_ = (char*)malloc(size);

    Clear();

    return SUCCESS;
  }

  int Append(const char* format, va_list arg) {
    int ret = vsnprintf(buffer_ + length_, size_ - length_ - 1,
                        format, arg);
    if (ret < 0) { return ret; }

    length_ += ret;
    if (length_ >= size_) {
      length_ = size_ - 1;
    }

    return SUCCESS;
  }

  int Append(const char* format, ...) {
    va_list arg;
    va_start(arg, format);

    Append(format, arg);

    va_end(arg);

    return SUCCESS;
  }

  uint32_t size() const { return size_; }
  uint32_t length() const { return length_; }
  char* buffer() const { return buffer_; }

 private:
  void set_size(uint32_t size) { size_ = size; }
  void set_length(uint32_t length) { length_ = length; }
  void set_buffer(char* buffer) { buffer_ = buffer; }

  void Close() {
    if (buffer_ != NULL) {
      free(buffer_);
      set_buffer(NULL);
    }

    set_size(0);
    set_length(0);
  }

 protected:
  char*    buffer_;
  uint32_t size_;
  uint32_t length_;

 private:
  LibBuffer(const LibBuffer& );
  LibBuffer & operator = (const LibBuffer &);
};
/////
}
#endif 
