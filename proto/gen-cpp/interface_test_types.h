/**
 * Autogenerated by Thrift Compiler (0.9.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef interface_test_TYPES_H
#define interface_test_TYPES_H

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>


namespace Test {


class ping {
 public:

  static const char* ascii_fingerprint; // = "3F5FC93B338687BC7235B1AB103F47B3";
  static const uint8_t binary_fingerprint[16]; // = {0x3F,0x5F,0xC9,0x3B,0x33,0x86,0x87,0xBC,0x72,0x35,0xB1,0xAB,0x10,0x3F,0x47,0xB3};

  ping() : a(0), b() {
  }

  virtual ~ping() throw() {}

  int32_t a;
  std::string b;

  void __set_a(const int32_t val) {
    a = val;
  }

  void __set_b(const std::string& val) {
    b = val;
  }

  bool operator == (const ping & rhs) const
  {
    if (!(a == rhs.a))
      return false;
    if (!(b == rhs.b))
      return false;
    return true;
  }
  bool operator != (const ping &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ping & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(ping &a, ping &b);


class pang {
 public:

  static const char* ascii_fingerprint; // = "4086F12A5C2D615560236565C542F3C3";
  static const uint8_t binary_fingerprint[16]; // = {0x40,0x86,0xF1,0x2A,0x5C,0x2D,0x61,0x55,0x60,0x23,0x65,0x65,0xC5,0x42,0xF3,0xC3};

  pang() : retcode(0), a(0), b() {
  }

  virtual ~pang() throw() {}

  int32_t retcode;
  int32_t a;
  std::string b;

  void __set_retcode(const int32_t val) {
    retcode = val;
  }

  void __set_a(const int32_t val) {
    a = val;
  }

  void __set_b(const std::string& val) {
    b = val;
  }

  bool operator == (const pang & rhs) const
  {
    if (!(retcode == rhs.retcode))
      return false;
    if (!(a == rhs.a))
      return false;
    if (!(b == rhs.b))
      return false;
    return true;
  }
  bool operator != (const pang &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const pang & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(pang &a, pang &b);

} // namespace

#endif
