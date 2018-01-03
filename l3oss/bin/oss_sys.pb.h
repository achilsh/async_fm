// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: oss_sys.proto

#ifndef PROTOBUF_oss_5fsys_2eproto__INCLUDED
#define PROTOBUF_oss_5fsys_2eproto__INCLUDED

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
void protobuf_AddDesc_oss_5fsys_2eproto();
void protobuf_AssignDesc_oss_5fsys_2eproto();
void protobuf_ShutdownFile_oss_5fsys_2eproto();

class ConfigInfo;
class ConnectWorker;
class LogLevel;
class TargetWorker;
class WorkerLoad;

// ===================================================================

class ConfigInfo : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:ConfigInfo) */ {
 public:
  ConfigInfo();
  virtual ~ConfigInfo();

  ConfigInfo(const ConfigInfo& from);

  inline ConfigInfo& operator=(const ConfigInfo& from) {
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
  static const ConfigInfo& default_instance();

  void Swap(ConfigInfo* other);

  // implements Message ----------------------------------------------

  inline ConfigInfo* New() const { return New(NULL); }

  ConfigInfo* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ConfigInfo& from);
  void MergeFrom(const ConfigInfo& from);
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
  void InternalSwap(ConfigInfo* other);
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

  // required string file_name = 1;
  bool has_file_name() const;
  void clear_file_name();
  static const int kFileNameFieldNumber = 1;
  const ::std::string& file_name() const;
  void set_file_name(const ::std::string& value);
  void set_file_name(const char* value);
  void set_file_name(const char* value, size_t size);
  ::std::string* mutable_file_name();
  ::std::string* release_file_name();
  void set_allocated_file_name(::std::string* file_name);

  // required string file_content = 2;
  bool has_file_content() const;
  void clear_file_content();
  static const int kFileContentFieldNumber = 2;
  const ::std::string& file_content() const;
  void set_file_content(const ::std::string& value);
  void set_file_content(const char* value);
  void set_file_content(const char* value, size_t size);
  ::std::string* mutable_file_content();
  ::std::string* release_file_content();
  void set_allocated_file_content(::std::string* file_content);

  // @@protoc_insertion_point(class_scope:ConfigInfo)
 private:
  inline void set_has_file_name();
  inline void clear_has_file_name();
  inline void set_has_file_content();
  inline void clear_has_file_content();

  // helper for ByteSize()
  int RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr file_name_;
  ::google::protobuf::internal::ArenaStringPtr file_content_;
  friend void  protobuf_AddDesc_oss_5fsys_2eproto();
  friend void protobuf_AssignDesc_oss_5fsys_2eproto();
  friend void protobuf_ShutdownFile_oss_5fsys_2eproto();

  void InitAsDefaultInstance();
  static ConfigInfo* default_instance_;
};
// -------------------------------------------------------------------

class WorkerLoad : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:WorkerLoad) */ {
 public:
  WorkerLoad();
  virtual ~WorkerLoad();

  WorkerLoad(const WorkerLoad& from);

  inline WorkerLoad& operator=(const WorkerLoad& from) {
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
  static const WorkerLoad& default_instance();

  void Swap(WorkerLoad* other);

  // implements Message ----------------------------------------------

  inline WorkerLoad* New() const { return New(NULL); }

  WorkerLoad* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const WorkerLoad& from);
  void MergeFrom(const WorkerLoad& from);
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
  void InternalSwap(WorkerLoad* other);
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

  // required int32 pid = 1;
  bool has_pid() const;
  void clear_pid();
  static const int kPidFieldNumber = 1;
  ::google::protobuf::int32 pid() const;
  void set_pid(::google::protobuf::int32 value);

  // required int32 load = 2;
  bool has_load() const;
  void clear_load();
  static const int kLoadFieldNumber = 2;
  ::google::protobuf::int32 load() const;
  void set_load(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:WorkerLoad)
 private:
  inline void set_has_pid();
  inline void clear_has_pid();
  inline void set_has_load();
  inline void clear_has_load();

  // helper for ByteSize()
  int RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int32 pid_;
  ::google::protobuf::int32 load_;
  friend void  protobuf_AddDesc_oss_5fsys_2eproto();
  friend void protobuf_AssignDesc_oss_5fsys_2eproto();
  friend void protobuf_ShutdownFile_oss_5fsys_2eproto();

  void InitAsDefaultInstance();
  static WorkerLoad* default_instance_;
};
// -------------------------------------------------------------------

class ConnectWorker : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:ConnectWorker) */ {
 public:
  ConnectWorker();
  virtual ~ConnectWorker();

  ConnectWorker(const ConnectWorker& from);

  inline ConnectWorker& operator=(const ConnectWorker& from) {
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
  static const ConnectWorker& default_instance();

  void Swap(ConnectWorker* other);

  // implements Message ----------------------------------------------

  inline ConnectWorker* New() const { return New(NULL); }

  ConnectWorker* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ConnectWorker& from);
  void MergeFrom(const ConnectWorker& from);
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
  void InternalSwap(ConnectWorker* other);
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

  // required int32 worker_index = 1;
  bool has_worker_index() const;
  void clear_worker_index();
  static const int kWorkerIndexFieldNumber = 1;
  ::google::protobuf::int32 worker_index() const;
  void set_worker_index(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:ConnectWorker)
 private:
  inline void set_has_worker_index();
  inline void clear_has_worker_index();

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int32 worker_index_;
  friend void  protobuf_AddDesc_oss_5fsys_2eproto();
  friend void protobuf_AssignDesc_oss_5fsys_2eproto();
  friend void protobuf_ShutdownFile_oss_5fsys_2eproto();

  void InitAsDefaultInstance();
  static ConnectWorker* default_instance_;
};
// -------------------------------------------------------------------

class TargetWorker : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:TargetWorker) */ {
 public:
  TargetWorker();
  virtual ~TargetWorker();

  TargetWorker(const TargetWorker& from);

  inline TargetWorker& operator=(const TargetWorker& from) {
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
  static const TargetWorker& default_instance();

  void Swap(TargetWorker* other);

  // implements Message ----------------------------------------------

  inline TargetWorker* New() const { return New(NULL); }

  TargetWorker* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const TargetWorker& from);
  void MergeFrom(const TargetWorker& from);
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
  void InternalSwap(TargetWorker* other);
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

  // required int32 err_no = 1;
  bool has_err_no() const;
  void clear_err_no();
  static const int kErrNoFieldNumber = 1;
  ::google::protobuf::int32 err_no() const;
  void set_err_no(::google::protobuf::int32 value);

  // required string worker_identify = 2;
  bool has_worker_identify() const;
  void clear_worker_identify();
  static const int kWorkerIdentifyFieldNumber = 2;
  const ::std::string& worker_identify() const;
  void set_worker_identify(const ::std::string& value);
  void set_worker_identify(const char* value);
  void set_worker_identify(const char* value, size_t size);
  ::std::string* mutable_worker_identify();
  ::std::string* release_worker_identify();
  void set_allocated_worker_identify(::std::string* worker_identify);

  // required string node_type = 3;
  bool has_node_type() const;
  void clear_node_type();
  static const int kNodeTypeFieldNumber = 3;
  const ::std::string& node_type() const;
  void set_node_type(const ::std::string& value);
  void set_node_type(const char* value);
  void set_node_type(const char* value, size_t size);
  ::std::string* mutable_node_type();
  ::std::string* release_node_type();
  void set_allocated_node_type(::std::string* node_type);

  // optional string err_msg = 4;
  bool has_err_msg() const;
  void clear_err_msg();
  static const int kErrMsgFieldNumber = 4;
  const ::std::string& err_msg() const;
  void set_err_msg(const ::std::string& value);
  void set_err_msg(const char* value);
  void set_err_msg(const char* value, size_t size);
  ::std::string* mutable_err_msg();
  ::std::string* release_err_msg();
  void set_allocated_err_msg(::std::string* err_msg);

  // @@protoc_insertion_point(class_scope:TargetWorker)
 private:
  inline void set_has_err_no();
  inline void clear_has_err_no();
  inline void set_has_worker_identify();
  inline void clear_has_worker_identify();
  inline void set_has_node_type();
  inline void clear_has_node_type();
  inline void set_has_err_msg();
  inline void clear_has_err_msg();

  // helper for ByteSize()
  int RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr worker_identify_;
  ::google::protobuf::internal::ArenaStringPtr node_type_;
  ::google::protobuf::internal::ArenaStringPtr err_msg_;
  ::google::protobuf::int32 err_no_;
  friend void  protobuf_AddDesc_oss_5fsys_2eproto();
  friend void protobuf_AssignDesc_oss_5fsys_2eproto();
  friend void protobuf_ShutdownFile_oss_5fsys_2eproto();

  void InitAsDefaultInstance();
  static TargetWorker* default_instance_;
};
// -------------------------------------------------------------------

class LogLevel : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:LogLevel) */ {
 public:
  LogLevel();
  virtual ~LogLevel();

  LogLevel(const LogLevel& from);

  inline LogLevel& operator=(const LogLevel& from) {
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
  static const LogLevel& default_instance();

  void Swap(LogLevel* other);

  // implements Message ----------------------------------------------

  inline LogLevel* New() const { return New(NULL); }

  LogLevel* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const LogLevel& from);
  void MergeFrom(const LogLevel& from);
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
  void InternalSwap(LogLevel* other);
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

  // required int32 log_level = 1;
  bool has_log_level() const;
  void clear_log_level();
  static const int kLogLevelFieldNumber = 1;
  ::google::protobuf::int32 log_level() const;
  void set_log_level(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:LogLevel)
 private:
  inline void set_has_log_level();
  inline void clear_has_log_level();

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int32 log_level_;
  friend void  protobuf_AddDesc_oss_5fsys_2eproto();
  friend void protobuf_AssignDesc_oss_5fsys_2eproto();
  friend void protobuf_ShutdownFile_oss_5fsys_2eproto();

  void InitAsDefaultInstance();
  static LogLevel* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// ConfigInfo

// required string file_name = 1;
inline bool ConfigInfo::has_file_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void ConfigInfo::set_has_file_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void ConfigInfo::clear_has_file_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void ConfigInfo::clear_file_name() {
  file_name_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_file_name();
}
inline const ::std::string& ConfigInfo::file_name() const {
  // @@protoc_insertion_point(field_get:ConfigInfo.file_name)
  return file_name_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ConfigInfo::set_file_name(const ::std::string& value) {
  set_has_file_name();
  file_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:ConfigInfo.file_name)
}
inline void ConfigInfo::set_file_name(const char* value) {
  set_has_file_name();
  file_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:ConfigInfo.file_name)
}
inline void ConfigInfo::set_file_name(const char* value, size_t size) {
  set_has_file_name();
  file_name_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:ConfigInfo.file_name)
}
inline ::std::string* ConfigInfo::mutable_file_name() {
  set_has_file_name();
  // @@protoc_insertion_point(field_mutable:ConfigInfo.file_name)
  return file_name_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ConfigInfo::release_file_name() {
  // @@protoc_insertion_point(field_release:ConfigInfo.file_name)
  clear_has_file_name();
  return file_name_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ConfigInfo::set_allocated_file_name(::std::string* file_name) {
  if (file_name != NULL) {
    set_has_file_name();
  } else {
    clear_has_file_name();
  }
  file_name_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), file_name);
  // @@protoc_insertion_point(field_set_allocated:ConfigInfo.file_name)
}

// required string file_content = 2;
inline bool ConfigInfo::has_file_content() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void ConfigInfo::set_has_file_content() {
  _has_bits_[0] |= 0x00000002u;
}
inline void ConfigInfo::clear_has_file_content() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void ConfigInfo::clear_file_content() {
  file_content_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_file_content();
}
inline const ::std::string& ConfigInfo::file_content() const {
  // @@protoc_insertion_point(field_get:ConfigInfo.file_content)
  return file_content_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ConfigInfo::set_file_content(const ::std::string& value) {
  set_has_file_content();
  file_content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:ConfigInfo.file_content)
}
inline void ConfigInfo::set_file_content(const char* value) {
  set_has_file_content();
  file_content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:ConfigInfo.file_content)
}
inline void ConfigInfo::set_file_content(const char* value, size_t size) {
  set_has_file_content();
  file_content_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:ConfigInfo.file_content)
}
inline ::std::string* ConfigInfo::mutable_file_content() {
  set_has_file_content();
  // @@protoc_insertion_point(field_mutable:ConfigInfo.file_content)
  return file_content_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* ConfigInfo::release_file_content() {
  // @@protoc_insertion_point(field_release:ConfigInfo.file_content)
  clear_has_file_content();
  return file_content_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void ConfigInfo::set_allocated_file_content(::std::string* file_content) {
  if (file_content != NULL) {
    set_has_file_content();
  } else {
    clear_has_file_content();
  }
  file_content_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), file_content);
  // @@protoc_insertion_point(field_set_allocated:ConfigInfo.file_content)
}

// -------------------------------------------------------------------

// WorkerLoad

// required int32 pid = 1;
inline bool WorkerLoad::has_pid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void WorkerLoad::set_has_pid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void WorkerLoad::clear_has_pid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void WorkerLoad::clear_pid() {
  pid_ = 0;
  clear_has_pid();
}
inline ::google::protobuf::int32 WorkerLoad::pid() const {
  // @@protoc_insertion_point(field_get:WorkerLoad.pid)
  return pid_;
}
inline void WorkerLoad::set_pid(::google::protobuf::int32 value) {
  set_has_pid();
  pid_ = value;
  // @@protoc_insertion_point(field_set:WorkerLoad.pid)
}

// required int32 load = 2;
inline bool WorkerLoad::has_load() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void WorkerLoad::set_has_load() {
  _has_bits_[0] |= 0x00000002u;
}
inline void WorkerLoad::clear_has_load() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void WorkerLoad::clear_load() {
  load_ = 0;
  clear_has_load();
}
inline ::google::protobuf::int32 WorkerLoad::load() const {
  // @@protoc_insertion_point(field_get:WorkerLoad.load)
  return load_;
}
inline void WorkerLoad::set_load(::google::protobuf::int32 value) {
  set_has_load();
  load_ = value;
  // @@protoc_insertion_point(field_set:WorkerLoad.load)
}

// -------------------------------------------------------------------

// ConnectWorker

// required int32 worker_index = 1;
inline bool ConnectWorker::has_worker_index() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void ConnectWorker::set_has_worker_index() {
  _has_bits_[0] |= 0x00000001u;
}
inline void ConnectWorker::clear_has_worker_index() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void ConnectWorker::clear_worker_index() {
  worker_index_ = 0;
  clear_has_worker_index();
}
inline ::google::protobuf::int32 ConnectWorker::worker_index() const {
  // @@protoc_insertion_point(field_get:ConnectWorker.worker_index)
  return worker_index_;
}
inline void ConnectWorker::set_worker_index(::google::protobuf::int32 value) {
  set_has_worker_index();
  worker_index_ = value;
  // @@protoc_insertion_point(field_set:ConnectWorker.worker_index)
}

// -------------------------------------------------------------------

// TargetWorker

// required int32 err_no = 1;
inline bool TargetWorker::has_err_no() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void TargetWorker::set_has_err_no() {
  _has_bits_[0] |= 0x00000001u;
}
inline void TargetWorker::clear_has_err_no() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void TargetWorker::clear_err_no() {
  err_no_ = 0;
  clear_has_err_no();
}
inline ::google::protobuf::int32 TargetWorker::err_no() const {
  // @@protoc_insertion_point(field_get:TargetWorker.err_no)
  return err_no_;
}
inline void TargetWorker::set_err_no(::google::protobuf::int32 value) {
  set_has_err_no();
  err_no_ = value;
  // @@protoc_insertion_point(field_set:TargetWorker.err_no)
}

// required string worker_identify = 2;
inline bool TargetWorker::has_worker_identify() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void TargetWorker::set_has_worker_identify() {
  _has_bits_[0] |= 0x00000002u;
}
inline void TargetWorker::clear_has_worker_identify() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void TargetWorker::clear_worker_identify() {
  worker_identify_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_worker_identify();
}
inline const ::std::string& TargetWorker::worker_identify() const {
  // @@protoc_insertion_point(field_get:TargetWorker.worker_identify)
  return worker_identify_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TargetWorker::set_worker_identify(const ::std::string& value) {
  set_has_worker_identify();
  worker_identify_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:TargetWorker.worker_identify)
}
inline void TargetWorker::set_worker_identify(const char* value) {
  set_has_worker_identify();
  worker_identify_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:TargetWorker.worker_identify)
}
inline void TargetWorker::set_worker_identify(const char* value, size_t size) {
  set_has_worker_identify();
  worker_identify_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:TargetWorker.worker_identify)
}
inline ::std::string* TargetWorker::mutable_worker_identify() {
  set_has_worker_identify();
  // @@protoc_insertion_point(field_mutable:TargetWorker.worker_identify)
  return worker_identify_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* TargetWorker::release_worker_identify() {
  // @@protoc_insertion_point(field_release:TargetWorker.worker_identify)
  clear_has_worker_identify();
  return worker_identify_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TargetWorker::set_allocated_worker_identify(::std::string* worker_identify) {
  if (worker_identify != NULL) {
    set_has_worker_identify();
  } else {
    clear_has_worker_identify();
  }
  worker_identify_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), worker_identify);
  // @@protoc_insertion_point(field_set_allocated:TargetWorker.worker_identify)
}

// required string node_type = 3;
inline bool TargetWorker::has_node_type() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void TargetWorker::set_has_node_type() {
  _has_bits_[0] |= 0x00000004u;
}
inline void TargetWorker::clear_has_node_type() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void TargetWorker::clear_node_type() {
  node_type_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_node_type();
}
inline const ::std::string& TargetWorker::node_type() const {
  // @@protoc_insertion_point(field_get:TargetWorker.node_type)
  return node_type_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TargetWorker::set_node_type(const ::std::string& value) {
  set_has_node_type();
  node_type_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:TargetWorker.node_type)
}
inline void TargetWorker::set_node_type(const char* value) {
  set_has_node_type();
  node_type_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:TargetWorker.node_type)
}
inline void TargetWorker::set_node_type(const char* value, size_t size) {
  set_has_node_type();
  node_type_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:TargetWorker.node_type)
}
inline ::std::string* TargetWorker::mutable_node_type() {
  set_has_node_type();
  // @@protoc_insertion_point(field_mutable:TargetWorker.node_type)
  return node_type_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* TargetWorker::release_node_type() {
  // @@protoc_insertion_point(field_release:TargetWorker.node_type)
  clear_has_node_type();
  return node_type_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TargetWorker::set_allocated_node_type(::std::string* node_type) {
  if (node_type != NULL) {
    set_has_node_type();
  } else {
    clear_has_node_type();
  }
  node_type_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), node_type);
  // @@protoc_insertion_point(field_set_allocated:TargetWorker.node_type)
}

// optional string err_msg = 4;
inline bool TargetWorker::has_err_msg() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void TargetWorker::set_has_err_msg() {
  _has_bits_[0] |= 0x00000008u;
}
inline void TargetWorker::clear_has_err_msg() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void TargetWorker::clear_err_msg() {
  err_msg_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_err_msg();
}
inline const ::std::string& TargetWorker::err_msg() const {
  // @@protoc_insertion_point(field_get:TargetWorker.err_msg)
  return err_msg_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TargetWorker::set_err_msg(const ::std::string& value) {
  set_has_err_msg();
  err_msg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:TargetWorker.err_msg)
}
inline void TargetWorker::set_err_msg(const char* value) {
  set_has_err_msg();
  err_msg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:TargetWorker.err_msg)
}
inline void TargetWorker::set_err_msg(const char* value, size_t size) {
  set_has_err_msg();
  err_msg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:TargetWorker.err_msg)
}
inline ::std::string* TargetWorker::mutable_err_msg() {
  set_has_err_msg();
  // @@protoc_insertion_point(field_mutable:TargetWorker.err_msg)
  return err_msg_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* TargetWorker::release_err_msg() {
  // @@protoc_insertion_point(field_release:TargetWorker.err_msg)
  clear_has_err_msg();
  return err_msg_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TargetWorker::set_allocated_err_msg(::std::string* err_msg) {
  if (err_msg != NULL) {
    set_has_err_msg();
  } else {
    clear_has_err_msg();
  }
  err_msg_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), err_msg);
  // @@protoc_insertion_point(field_set_allocated:TargetWorker.err_msg)
}

// -------------------------------------------------------------------

// LogLevel

// required int32 log_level = 1;
inline bool LogLevel::has_log_level() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void LogLevel::set_has_log_level() {
  _has_bits_[0] |= 0x00000001u;
}
inline void LogLevel::clear_has_log_level() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void LogLevel::clear_log_level() {
  log_level_ = 0;
  clear_has_log_level();
}
inline ::google::protobuf::int32 LogLevel::log_level() const {
  // @@protoc_insertion_point(field_get:LogLevel.log_level)
  return log_level_;
}
inline void LogLevel::set_log_level(::google::protobuf::int32 value) {
  set_has_log_level();
  log_level_ = value;
  // @@protoc_insertion_point(field_set:LogLevel.log_level)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------

// -------------------------------------------------------------------

// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_oss_5fsys_2eproto__INCLUDED
