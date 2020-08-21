/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "protobag/Utils/PBUtils.hpp"

#include <algorithm>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>


namespace protobag {


// ============================================================================
// Reflection Utils ===========================================================
// ============================================================================

// Based upon https://github.com/protocolbuffers/protobuf/blob/15d0cd714dc24d7c460c70d469ce10a45d6c6c02/src/google/protobuf/descriptor.cc#L174
static const size_t kPBNumCPPTypes = 18;
static const char* const kPBCppTypeToName[kPBNumCPPTypes + 1] = {
  "UNKNOWN",
  "double",    // TYPE_DOUBLE
  "float",     // TYPE_FLOAT
  "int64",     // TYPE_INT64
  "uint64",    // TYPE_UINT64
  "int32",     // TYPE_INT32
  "fixed64",   // TYPE_FIXED64
  "fixed32",   // TYPE_FIXED32
  "bool",      // TYPE_BOOL
  "string",    // TYPE_STRING
  "group",     // TYPE_GROUP
  "message",   // TYPE_MESSAGE
  "bytes",     // TYPE_BYTES
  "uint32",    // TYPE_UINT32
  "enum",      // TYPE_ENUM
  "sfixed32",  // TYPE_SFIXED32
  "sfixed64",  // TYPE_SFIXED64
  "sint32",    // TYPE_SINT32
  "sint64",    // TYPE_SINT64
};
const char * const GetPBCPPTypeName(
  ::google::protobuf::FieldDescriptor::CppType type_id) {
  
  // We want to be resilent to building with future protobuf versions, so
  // disable tautology warning.
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
  
  
  if (type_id < kPBNumCPPTypes + 1) {
    return kPBCppTypeToName[type_id];
  } else {
    return "OUT_OF_BOUNDS_UPDATE_kPBCppTypeToName";
  }
  
  #pragma clang diagnostic pop
}

Result<const ::google::protobuf::FieldDescriptor*> 
GetField(
    const ::google::protobuf::Message *message,
    const std::string &fieldname,
    ::google::protobuf::FieldDescriptor::CppType pb_cpp_type) {

  if (!message) {
    return {.error = "Programming error: need user-allocated output message"};
  }

  using namespace ::google::protobuf;
  const Descriptor* descriptor = message->GetDescriptor();
  if (!descriptor) {
    return {
      .error = fmt::format("Msg {} has no descriptor", message->GetTypeName())
    };
  }

  const FieldDescriptor* field = descriptor->FindFieldByName(fieldname);
  if (!field) {
    return {
      .error = fmt::format(
        "Msg {} has no field {}", message->GetTypeName(), fieldname)
    };
  }

  if (field->cpp_type() != pb_cpp_type) {
    return {
      .error = fmt::format(
        "Wanted field {} on msg {} to be type {}, but {}.{} is of type {}",
        fieldname,
        message->GetTypeName(),
        GetPBCPPTypeName(pb_cpp_type),
        message->GetTypeName(), fieldname,
        GetPBCPPTypeName(field->cpp_type()))
    };
  }

  return {.value = field};
}

struct ctx {
  const ::google::protobuf::Reflection* reflection;
  const ::google::protobuf::FieldDescriptor* field;
};
Result<ctx> GetCTX(
      const ::google::protobuf::Message *message,
      const std::string &fieldname,
      ::google::protobuf::FieldDescriptor::CppType pb_cpp_type) {

  auto maybe_field = GetField(message, fieldname, pb_cpp_type);
  if (!maybe_field.IsOk()) { return {.error = maybe_field.error}; }

  const ::google::protobuf::Reflection* reflection = message->GetReflection();
  if (!reflection) {
    return {
      .error =
        fmt::format("Message {} has no reflection", message->GetTypeName())
    };
  }

  return {.value = ctx{.reflection = reflection, .field = *maybe_field.value}};
}


////
//// GetAttr Impl
////

Result<int32_t> GetAttr_int32(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_INT32);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = maybe_ctx.value->reflection->GetInt32(
      *message,
      maybe_ctx.value->field)
  };
}

Result<int64_t> GetAttr_int64(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_INT64);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = maybe_ctx.value->reflection->GetInt64(
      *message,
      maybe_ctx.value->field)
  };
}

Result<float> GetAttr_float(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_FLOAT);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = maybe_ctx.value->reflection->GetFloat(
      *message,
      maybe_ctx.value->field)
  };
}

Result<double> GetAttr_double(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = maybe_ctx.value->reflection->GetDouble(
      *message,
      maybe_ctx.value->field)
  };
}

Result<bool> GetAttr_bool(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_BOOL);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = maybe_ctx.value->reflection->GetBool(
      *message,
      maybe_ctx.value->field)
  };
}

Result<std::string> GetAttr_string(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_STRING);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = maybe_ctx.value->reflection->GetString(
      *message,
      maybe_ctx.value->field)
  };
}

Result<const ::google::protobuf::Message *> GetAttr_msg(
    const ::google::protobuf::Message *message,
    const std::string &fieldname) {

  auto maybe_ctx = GetCTX(
    message, fieldname, ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
  if (!maybe_ctx.IsOk()) { return {.error = maybe_ctx.error }; }
  return {
    .value = &maybe_ctx.value->reflection->GetMessage(
      *message,
      maybe_ctx.value->field)
  };
}


////
//// GetDeep Impl
////

// "first.second.third" -> "first", "second.third"
std::pair<std::string, std::string> SplitHeadTail(
    const std::string &field_path) {
  size_t pos = field_path.find_first_of(".");
  std::string head = field_path;
  std::string tail;
  if (pos != std::string::npos) {
    head = field_path.substr(0, pos);
    tail = field_path.substr(pos + 1);
  }
  return std::make_pair(head, tail);
}

// "first.second.third" -> "first.second", "third"
std::pair<std::string, std::string> GetPrefixAttr(
    const std::string &field_path) {
  size_t pos = field_path.find_last_of(".");
  std::string prefix;
  std::string attr = field_path;
  if (pos != std::string::npos) {
    prefix = field_path.substr(0, pos);
    attr = field_path.substr(pos + 1);
  }
  return std::make_pair(prefix, attr);
}


Result<const ::google::protobuf::Message *> GetDeep_msg(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  auto head_tail = SplitHeadTail(field_path);
  while (!head_tail.first.empty()) {
    auto maybe_member = GetAttr_msg(message, head_tail.first);
    if (!maybe_member.IsOk()) {
      return {
        .error = fmt::format(
          "Could not get member {} of {}, error: {}",
          head_tail.first,
          message->GetTypeName(),
          maybe_member.error)
      };
    }

    message = *maybe_member.value;
    head_tail = SplitHeadTail(head_tail.second);
  }
  
  return {.value = message};
}

template <typename T, typename GetAttrT>
Result<T> GetDeep_attr(
        const ::google::protobuf::Message *message,
        const std::string &field_path,
        const std::string &err_fname,
        GetAttrT get_attr,
        T ignored) {

  auto prefix_attr = GetPrefixAttr(field_path);
  auto maybe_message = GetDeep_msg(message, prefix_attr.first);
  if (!maybe_message.IsOk()) {
    return {
        .error = fmt::format(
          "{} to {} on {}.{} failed: {}",
          err_fname,
          field_path,
          message->GetTypeName(),
          prefix_attr.first,
          maybe_message.error)
      };
  }

  return get_attr(*maybe_message.value, prefix_attr.second);
}


Result<int32_t> GetDeep_int32(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  return GetDeep_attr(
    message, field_path, "GetDeep_int32", &GetAttr_int32, int32_t());
}

Result<int64_t> GetDeep_int64(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  return GetDeep_attr(
    message, field_path, "GetDeep_int64", &GetAttr_int64, int64_t());
}

Result<float> GetDeep_float(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  return GetDeep_attr(
    message, field_path, "GetDeep_float", &GetAttr_float, float());
}

Result<double> GetDeep_double(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  return GetDeep_attr(
    message, field_path, "GetDeep_double", &GetAttr_double, double());
}

Result<bool> GetDeep_bool(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  return GetDeep_attr(
    message, field_path, "GetDeep_bool", &GetAttr_bool, bool());
}

Result<std::string> GetDeep_string(
    const ::google::protobuf::Message *message,
    const std::string &field_path) {

  return GetDeep_attr(
    message, field_path, "GetDeep_string", &GetAttr_string, std::string());
}





// ============================================================================
// DynamicMessageFactory ======================================================
// ============================================================================

struct DynamicMsgFactory::Impl {
  ::google::protobuf::SimpleDescriptorDatabase db;
  std::shared_ptr<::google::protobuf::DescriptorPool> pool;
  ::google::protobuf::DynamicMessageFactory factory;

  std::list<std::string> known_descriptor_names;

  Impl() {
    pool.reset(new ::google::protobuf::DescriptorPool(&db));
      // NB: we need to call *this* DescriptorPool ctor because 
      // DescriptorPool doesn't expose a setter.
  }
};

void DynamicMsgFactory::LazyInitImpl() {
  if (!_impl) {
    _impl.reset(new Impl());
  }
}

void DynamicMsgFactory::RegisterTypes(
  const ::google::protobuf::FileDescriptorSet &fds) {
  
  for (const auto &fd : fds.file()) { RegisterType(fd); }
}

void DynamicMsgFactory::RegisterType(
        const ::google::protobuf::FileDescriptorProto &fd) {
  LazyInitImpl();
  _impl->db.Add(fd);

  for (const ::google::protobuf::DescriptorProto &d : fd.message_type()) {
    _impl->known_descriptor_names.push_back(fd.package() + "." + d.name());
  }
}

DynamicMsgFactory::MsgPtrOrErr DynamicMsgFactory::LoadFromArray(
                const std::string &type_url,
                const std::byte *data,
                size_t size) {

  if ((data == nullptr) || (size == 0)) {
    return {.error = "Bad array"};
  }

  if (!_impl) {
    return {.error = 
      "This factory has no known types. Use RegisterType() or RegisterTypes()"
    };
  }
  Impl &impl = *_impl;

  const ::google::protobuf::Descriptor *mt = nullptr;
  mt = impl.pool->FindMessageTypeByName(GetMessageTypeName(type_url));
  if (!mt) {
    return {.error = fmt::format("Could not resolve type {}" , type_url)};
  }

  const ::google::protobuf::Message* prototype = impl.factory.GetPrototype(mt);
  if (!prototype) {
    return { .error = fmt::format(
      "protobuf::DynamicMessageFactory failed to create prototype for {}",
      type_url)
    };
  }

  std::unique_ptr<::google::protobuf::Message> mp(prototype->New());
  auto res = PBFactory::LoadFromArray(data, size, mp.get());
  if (!res.IsOk()) {
    return {.error = res.error};
  }

  return {.value = std::move(mp)};
}

std::string DynamicMsgFactory::ToString() const {
  std::stringstream ss;
  ss << "DynamicMsgFactory" << std::endl;

  if (_impl) {
    Impl &impl = *_impl;

    // message types
    {
      std::vector<std::string> typenames(
        impl.known_descriptor_names.begin(),
        impl.known_descriptor_names.end());
      std::sort(typenames.begin(), typenames.end());
      ss << "Factory known types:" << std::endl;
      for (const auto &tn : typenames) {
        ss << tn << std::endl;
      }

      ss << std::endl;
    }

    // filenames
    {
      std::vector<std::string> fnames;
      bool success = impl.db.FindAllFileNames(&fnames);
      if (success) {
        ss << "DB known filenames:" << std::endl;
        std::sort(fnames.begin(), fnames.end());
        for (const auto &fname : fnames) {
          ss << fname << std::endl;
        }
      }
      
      ss << std::endl;
    }
  } else {
    ss << "(no registered types)" << std::endl;
  }

  return ss.str();
}

} /* namespace protobag */