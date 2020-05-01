#pragma once

#include <string>

#include "protobag/ReadSession.hpp"
#include "protobag/WriteSession.hpp"
#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

int foo();

class Protobag final {
public:
  
  std::string path;

  Result<WriteSession::Ptr> StartWriteSession(WriteSession::Spec s={}) const {
    s.archive_spec.path = path;
    return WriteSession::Create(s);
  }

  Result<ReadSession::Ptr> ReadEntries(ReadSession::Spec s={}) const {
    s.archive_spec.path = path;
    return ReadSession::Create(s);
  }

  BagMeta GetIndex() const;
};

} /* namespace protobag */
