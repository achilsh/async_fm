// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: hello_test.proto

#ifndef PROTOBUF_hello_5ftest_2eproto__INCLUDED
#define PROTOBUF_hello_5ftest_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_hello_5ftest_2eproto();
void protobuf_AssignDesc_hello_5ftest_2eproto();
void protobuf_ShutdownFile_hello_5ftest_2eproto();

class hellotest;

// ===================================================================

class hellotest : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:hellotest) */ {
 public:
  hellotest();
  virtual ~hellotest();

  hellotest(const hellotest& from);

  inline hellotest& operator=(const hellotest& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const hellotest& default_instance();

  void Swap(hellotest* other);

  // implements Message ----------------------------------------------

  inline hellotest* New() const { return New(NULL); }

  hellotest* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const hellotest& from);
  void MergeFrom(const hellotest& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const {
    return InternalSerializeWithCachedSizesToArray(false, output);
  }
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(hellotest* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional bytes strData = 1;
  bool has_strdata() const;
  void clear_strdata();
  static const int kStrDataFieldNumber = 1;
  const ::std::string& strdata() const;
  void set_strdata(const ::std::string& value);
  void set_strdata(const char* value);
  void set_strdata(const void* value, size_t size);
  ::std::string* mutable_strdata();
  ::std::string* release_strdata();
  void set_allocated_strdata(::std::string* strdata);

  // @@protoc_insertion_point(class_scope:hellotest)
 private:
  inline void set_has_strdata();
  inline void clear_has_strdata();

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr strdata_;
  friend void  protobuf_AddDesc_hello_5ftest_2eproto();
  friend void protobuf_AssignDesc_hello_5ftest_2eproto();
  friend void protobuf_ShutdownFile_hello_5ftest_2eproto();

  void InitAsDefaultInstance();
  static hellotest* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// hellotest

// optional bytes strData = 1;
inline bool hellotest::has_strdata() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void hellotest::set_has_strdata() {
  _has_bits_[0] |= 0x00000001u;
}
inline void hellotest::clear_has_strdata() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void hellotest::clear_strdata() {
  strdata_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_strdata();
}
inline const ::std::string& hellotest::strdata() const {
  // @@protoc_insertion_point(field_get:hellotest.strData)
  return strdata_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void hellotest::set_strdata(const ::std::string& value) {
  set_has_strdata();
  strdata_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:hellotest.strData)
}
inline void hellotest::set_strdata(const char* value) {
  set_has_strdata();
  strdata_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:hellotest.strData)
}
inline void hellotest::set_strdata(const void* value, size_t size) {
  set_has_strdata();
  strdata_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:hellotest.strData)
}
inline ::std::string* hellotest::mutable_strdata() {
  set_has_strdata();
  // @@protoc_insertion_point(field_mutable:hellotest.strData)
  return strdata_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* hellotest::release_strdata() {
  // @@protoc_insertion_point(field_release:hellotest.strData)
  clear_has_strdata();
  return strdata_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void hellotest::set_allocated_strdata(::std::string* strdata) {
  if (strdata != NULL) {
    set_has_strdata();
  } else {
    clear_has_strdata();
  }
  strdata_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), strdata);
  // @@protoc_insertion_point(field_set_allocated:hellotest.strData)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_hello_5ftest_2eproto__INCLUDED
