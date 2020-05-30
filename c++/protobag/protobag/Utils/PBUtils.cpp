#include "protobag/Utils/PBUtils.hpp"

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>

namespace protobag {

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