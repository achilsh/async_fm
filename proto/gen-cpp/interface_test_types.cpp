/**
 * Autogenerated by Thrift Compiler (0.9.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "interface_test_types.h"

#include <algorithm>

namespace Test {

const char* ping::ascii_fingerprint = "3F5FC93B338687BC7235B1AB103F47B3";
const uint8_t ping::binary_fingerprint[16] = {0x3F,0x5F,0xC9,0x3B,0x33,0x86,0x87,0xBC,0x72,0x35,0xB1,0xAB,0x10,0x3F,0x47,0xB3};

uint32_t ping::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;

  bool isset_a = false;
  bool isset_b = false;

  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->a);
          isset_a = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->b);
          isset_b = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  if (!isset_a)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_b)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  return xfer;
}

uint32_t ping::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("ping");

  xfer += oprot->writeFieldBegin("a", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->a);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("b", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString(this->b);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(ping &a, ping &b) {
  using ::std::swap;
  swap(a.a, b.a);
  swap(a.b, b.b);
}

const char* pang::ascii_fingerprint = "4086F12A5C2D615560236565C542F3C3";
const uint8_t pang::binary_fingerprint[16] = {0x40,0x86,0xF1,0x2A,0x5C,0x2D,0x61,0x55,0x60,0x23,0x65,0x65,0xC5,0x42,0xF3,0xC3};

uint32_t pang::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;

  bool isset_retcode = false;
  bool isset_a = false;
  bool isset_b = false;

  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->retcode);
          isset_retcode = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->a);
          isset_a = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->b);
          isset_b = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  if (!isset_retcode)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_a)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_b)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  return xfer;
}

uint32_t pang::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("pang");

  xfer += oprot->writeFieldBegin("retcode", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->retcode);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("a", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->a);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("b", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->b);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(pang &a, pang &b) {
  using ::std::swap;
  swap(a.retcode, b.retcode);
  swap(a.a, b.a);
  swap(a.b, b.b);
}

} // namespace
