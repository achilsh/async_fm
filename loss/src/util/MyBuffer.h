/**
 * @file: MyBuffer.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2019-03-03
 */

#pragma once

#include <algorithm>
#include "MySlice.h"

namespace loss 
{

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |   (is content)   |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

class MyBuffer 
{
    public:
      static const size_t kCheapPrependSize;
      static const size_t kInitialSize;
      
      explicit MyBuffer(size_t initSize = kInitialSize, 
                        size_t reservedPrependSize = kCheapPrependSize)
          : capacity_(reservedPrependSize + initSize)
          , read_index_(reservedPrependSize)
          , write_index_(reservedPrependSize)
          , reserved_prepend_size_(reservedPrependSize) {
          buffer_ = new char[capacity_];
          assert(length() == 0);
          assert(WritableBytes() == initSize);
          assert(PrependableBytes() == reservedPrependSize);
      }

      ~MyBuffer() 
      {
          delete [] buffer_;
          buffer_   = nullptr;
          capacity_ = 0;
      }


      void Swap(MyBuffer& rhs) 
      {
         std::swap( buffer_,                rhs.buffer_ );
         std::swap( capacity_,              rhs.capacity_ );
         std::swap( read_index_,            rhs.read_index_ );
         std::swap( write_index_,           rhs.write_index_ );
         std::swap( reserved_prepend_size_, rhs.reserved_prepend_size_ );
      }

      /**
       * @brief: Skip 
       *   advances the reading index of MyBuffer to iLen,
       *   将已读的标示向前增加iLen
       *
       * @param iLen
       */
      void Skip(size_t iLen) 
      {
          if (iLen < length()) 
          {
              read_index_ += iLen;
          } 
          else 
          {
              Reset();
          }
      }


      /**
       * @brief: Retrieve 
       *   将已读的标示向前增加iLen
       *   advance the reading index of MyBuffer to iLen
       *   is the same of Skip(ilen);
       *
       * @param iLen
       */
      void Retrieve(size_t iLen) 
      {
          this->Skip(iLen);
      }


      /**
       * @brief: Truncate 
       *        discards all but the first iLen unread bytes from 
       *        MyBuffear. set unread bytes to  iLen: 
       *        write_index_ - read_index_ is iLen.
       *        if iLen gt the length of MyBuffer, do nothing.
       *
       * @param iLen
       */
      void Truncate(size_t iLen) 
      {
          if (iLen == 0) 
          {
              read_index_  = reserved_prepend_size_;
              write_index_ = reserved_prepend_size_;
          }
          else if (write_index_ > read_index_ + iLen)
          {
              write_index_ = read_index_ + iLen;
          }
      }


      /**
       * @brief: Reset
       *  resets MyBuffer to empty.
       */
      void Reset() 
      {
          Truncate(0);
      }


      /**
       * @brief: Reserve 
       *        increase the capacity of MyBuffer to a value that's
       *        gt iLen, eg: if iLen gt the current capacity, new
       *        storage is allocated, otherwise doesn't nothing.
       * @param iLen
       */
      void Reserve(size_t iLen) 
      {
          if (capacity_  >= iLen + reserved_prepend_size_)
          {
              return ;
          }

          Grow(iLen + reserved_prepend_size_);
      }


      /**
       * @brief: EnsureWritableBytes 
       *         make sure there is enough mem space to append data with length
       *         iLen.
       * @param iLen
       */
      void EnsureWritableBytes(size_t iLen) 
      {
          if (WritableBytes() < iLen) 
          {
              Grow(iLen);
          }
          assert(WritableBytes() >= iLen);
      }

      
      /**
       * @brief: ToText 
       *     appends char '\0' to buffer to convert the underlying data to 
       *     a c-style string text. It will not change the length of MyBuffer.
       */
      void ToText() 
      {
          AppendInt8('\0');
          UnwriteBytes(1);
      }
#define evppbswap_64(x)                            \
      ((((x) & 0xff00000000000000ull) >> 56)       \
       | (((x) & 0x00ff000000000000ull) >> 40)     \
       | (((x) & 0x0000ff0000000000ull) >> 24)     \
       | (((x) & 0x000000ff00000000ull) >> 8)      \
       | (((x) & 0x00000000ff000000ull) << 8)      \
       | (((x) & 0x0000000000ff0000ull) << 24)     \
       | (((x) & 0x000000000000ff00ull) << 40)     \
       | (((x) & 0x00000000000000ffull) << 56))


    public:
      void Write(const void* pData, size_t iLen) 
      {
          EnsureWritableBytes(iLen);
          memcpy(WriteBegin(), pData, iLen);
          assert(write_index_ + iLen <= capacity_);
          write_index_ += iLen;
      }
      
      void Append(const Slice& strData) 
      {
          Write(strData.data(), strData.size());
      }
      
      //eg: output_buffer_.Append(static_cast<const char*>(data) + nwritten, remaining)
      void Append(const char* pData, size_t iLen) 
      {
          Write(pData, iLen);
      }

      void Append(const void* pData, size_t iLen) 
      {
          Write(pData, iLen);
      }

      // Append int64_t/int32_t/int16_t with network endian
      void AppendInt64(int64_t x) 
      {
          int64_t be = evppbswap_64(x);
          Write(&be, sizeof be);
      }

      void AppendInt32(int32_t x) 
      {
          int32_t be32 = htonl(x);
          Write(&be32, sizeof be32);
      }

      void AppendInt16(int16_t x) 
      {
          int16_t be16 = htons(x);
          Write(&be16, sizeof be16);
      }

      void AppendInt8(int8_t x) 
      {
          Write(&x, sizeof x);
      }

      // Prepend int64_t/int32_t/int16_t with network endian
      // from little endian to network endian
      void PrependInt64(int64_t x) 
      {
          int64_t be = evppbswap_64(x);
          Prepend(&be, sizeof be);
      }

      void PrependInt32(int32_t x) 
      {
          int32_t be32 = htonl(x);
          Prepend(&be32, sizeof be32);
      }

      void PrependInt16(int16_t x) 
      {
          int16_t be16 = htons(x);
          Prepend(&be16, sizeof be16);
      }

      void PrependInt8(int8_t x) 
      {
          Prepend(&x, sizeof x);
      }


      /**
       * @brief: Prepend
       *         Insert content into the front of reading index.
       *
       * @param d
       * @param len
       */
      void Prepend(const void* /*restrict*/ d, size_t len) 
      {
          assert(len <= PrependableBytes());
          read_index_ -= len;
          const char* p = static_cast<const char*>(d);
          memcpy(begin() + read_index_, p, len);
      }


      void UnwriteBytes(size_t n) 
      {
          assert(n <= length());
          write_index_ -= n;
      }

      /**
       * @brief: WriteBytes, modity write index with size n,
       *    generally after read data with MyBuffer, then 
       *    call this interface to modity write_index_
       *
       * @param n
       */
      void WriteBytes(size_t n) 
      {
          assert(n <= WritableBytes());
          write_index_ += n;
      }

    public:
      //read
      /**
       * @brief: ReadInt64 
       *        get int from MyBuffer, then transfer int from
       *        network endian to little endian.
       *
       * @return 
       */
      int64_t ReadInt64() 
      {
          int64_t result = PeekInt64();
          Skip(sizeof result);
          return result;
      }

      int32_t ReadInt32() 
      {
          int32_t result = PeekInt32();
          Skip(sizeof result);
          return result;
      }

      int16_t ReadInt16() 
      {
          int16_t result = PeekInt16();
          Skip(sizeof result);
          return result;
      }

      int8_t ReadInt8() 
      {
          int8_t result = PeekInt8();
          Skip(sizeof result);
          return result;
      }

      Slice ToSlice() const 
      {
          return Slice(data(), length());
      }

      std::string ToString() const 
      {
          return std::string(data(), length());
      }

      /**
       * @brief: Shrink,在原来的基础上增加 reserve 长度数据
       *
       * @param reserve
       */
      void Shrink(size_t reserve) 
      {
          MyBuffer other(length() + reserve);
          other.Append(ToSlice());
          Swap(other);
      }


      /**
       * @brief: ReadFromFD, 从fd上读取数据并存入MyBuffer中，
       *         常用于读事件就绪后数据读取,eg: 
       *         input_buffer_.ReadFromFD(chan_->fd(), &serrno);
       *
       * @param fd
       * @param saved_errno
       *
       * @return 
       */
      ssize_t ReadFromFD(int fd, int* saved_errno);

      /**
       * @brief: Next 
       *         从MyBuffer对象中获取长度为iLen数据，
       *         同时更新MyBuffer的已读标识read_index_
       *
       * @param iLen
       *
       * @return 
       */
      Slice Next(size_t iLen) 
      {
          if (iLen < length()) 
          {
              Slice result(data(), iLen);
              read_index_ += iLen;
              return result;
          }

          return NextAll();
      }


      /**
       * @brief: NextAll,从MyBuffer中获取所有未读的数据,
       *         并将已读标识和未读标识设置初始状态
       *
       * @return 
       */
      Slice NextAll() 
      {
          Slice result(data(), length());
          Reset();
          return result;
      }

      /**
       * @brief: NextString,将已读数据索引向前迁移len字节 
       *
       * @param len
       *
       * @return, 返回迁移的数据 
       */
      std::string NextString(size_t len) 
      {
          Slice s = Next(len);
          return std::string(s.data(), s.size());
      }
        
      /**
       * @brief: NextAllString, 读取MyBuffer的所有未读数据，
       *         读取之后重置MyBuffer, eg: std::string s = msg->NextAllString();
       *
       * @return 返回所有未读数据 
       */
      std::string NextAllString() 
      {
          Slice s = NextAll();
          return std::string(s.data(), s.size());
      }

      /**
       * @brief: ReadByte,从MyBuffer中读取一字节，
       *          如果没有字节可读，则返回'\0'
       *          否则读取一字节，并更新MyBuffer中的已读标示
       *
       * @return 
       */
      char ReadByte() 
      {
          assert(length() >= 1);

          if (length() == 0) 
          {
              return '\0';
          }

          return buffer_[read_index_++];
      }


      /**
       * @brief: UnreadBytes ,更新已读标示
       *
       * @param n
       */
      void UnreadBytes(size_t n) 
      {
          assert(n < read_index_);
          read_index_ -= n;
      }

      //仅仅是查看下MyBuffer已读后数据，返回的数据是
      //将网路高端字节序列转为本地字节序
      int64_t PeekInt64() const 
      {
          assert(length() >= sizeof(int64_t));
          int64_t be64 = 0;
          ::memcpy(&be64, data(), sizeof be64);
          return evppbswap_64(be64);
      }

      int32_t PeekInt32() const 
      {
          assert(length() >= sizeof(int32_t));
          int32_t be32 = 0;
          ::memcpy(&be32, data(), sizeof be32);
          return ntohl(be32);
      }

      int16_t PeekInt16() const 
      {
          assert(length() >= sizeof(int16_t));
          int16_t be16 = 0;
          ::memcpy(&be16, data(), sizeof be16);
          return ntohs(be16);
      }

      int8_t PeekInt8() const 
      {
          assert(length() >= sizeof(int8_t));
          int8_t x = *data();
          return x;
      }

    public:

      /**
       * @brief: data,返回未读取数据的起始地址 
       *    eg:  n = ::send(fd_, output_buffer_.data(), output_buffer_.length(), MSG_NOSIGNAL) 
       *         output_buffer_.Next(n)
       *
       * @return 
       */
      const char* data() const 
      {
          return buffer_ + read_index_;
      }

      /**
       * @brief: WriteBegin,返回可写的地址空间 
       *         eg: ret = ::recvfrom(thread->fd(), (char*)recv_msg->WriteBegin(), recv_buf_size_, 0, recv_msg->mutable_remote_addr(), &addr_len)
       *             recv_msg->WriteBytes(ret);
       *
       * @return 
       */
      char* WriteBegin() 
      {
          return begin() + write_index_;
      }

      const char* WriteBegin() const 
      {
          return begin() + write_index_;
      }

      /**
       * @brief: length, 获取MyBuffer未读数据数据的长度 
       *
       * @return 
       */
      size_t length() const 
      {
          assert(write_index_ >= read_index_);
          return write_index_ - read_index_;
      }

      size_t size() const 
      {
          return length();
      }

      size_t capacity() const 
      {
          return capacity_;
      }


      /**
       * @brief: WritableBytes,获取用于可写的空间大小 
       *
       * @return 
       */
      size_t WritableBytes() const 
      {
          assert(capacity_ >= write_index_);
          return capacity_ - write_index_;
      }


      /**
       * @brief: PrependableBytes,返回可以向前append的空间的大小
       *         ,实际是空间已读的标示
       *
       * @return 
       */
      size_t PrependableBytes() const 
      {
          return read_index_;
      }


      /**
       * @brief: FindCRLF,从未读取的数据中查找\r\n,
       *       查询到，返回第一个\r\n的地址，未找到返回nullptr.
       *
       * @return 
       */
      const char* FindCRLF() const 
      {
          const char* crlf = std::search(data(), WriteBegin(), kCRLF, kCRLF + 2);
          return crlf == WriteBegin() ? nullptr : crlf;
      }


      /**
       * @brief: FindCRLF, 从初始位置start的未读空间中查找\r\n字符，
       *                找到则返回第一个\r\n位置，否则返回nullptr.
       *
       * @param start
       *
       * @return 
       */
      const char* FindCRLF(const char* start) const 
      {
          assert(data() <= start);
          assert(start <= WriteBegin());
          const char* crlf = std::search(start, WriteBegin(), kCRLF, kCRLF + 2);
          return crlf == WriteBegin() ? nullptr : crlf;
      }


      /**
       * @brief: FindEOL,从未读空间中查找\n字符 
       *
       * @return 
       */
      const char* FindEOL() const 
      {
          const void* eol = memchr(data(), '\n', length());
          return static_cast<const char*>(eol);
      }

      /**
       * @brief: FindEOL,从start起始的未读数据空间查找\n 字符 
       *
       * @param start
       *
       * @return 
       */
      const char* FindEOL(const char* start) const 
      {
          assert(data() <= start);
          assert(start <= WriteBegin());
          const void* eol = memchr(start, '\n', WriteBegin() - start);
          return static_cast<const char*>(eol);
      }

    private:
      char* begin() 
      {
          return buffer_;
      }

      const char* begin() const 
      {
          return buffer_;
      }

      void Grow(size_t len) 
      {
          if (WritableBytes() + PrependableBytes() < len + reserved_prepend_size_) 
          {
              //grow the capacity
              size_t n = (capacity_ << 1) + len;
              size_t m = length();
              char* d = new char[n];
              memcpy(d + reserved_prepend_size_, begin() + read_index_, m);
              write_index_ = m + reserved_prepend_size_;
              read_index_ = reserved_prepend_size_;
              capacity_ = n;
              delete[] buffer_;
              buffer_ = d;
          } 
          else
          {
              // move readable data to the front, make space inside buffer
              assert(reserved_prepend_size_ < read_index_);
              size_t readable = length();
              memmove(begin() + reserved_prepend_size_, begin() + read_index_, length());
              read_index_ = reserved_prepend_size_;
              write_index_ = read_index_ + readable;
              assert(readable == length());
              assert(WritableBytes() >= len);
          }
      }

    private:
      char* buffer_;
      size_t capacity_;
      size_t read_index_;
      size_t write_index_;
      size_t reserved_prepend_size_;
      static const char kCRLF[];
};

}
