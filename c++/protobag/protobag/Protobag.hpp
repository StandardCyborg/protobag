#pragma once

#include <string>

#include "protobag/ReadSession.hpp"
#include "protobag/WriteSession.hpp"
#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

int foo();

class Protobag final {
public:
  Protobag() = default;
  explicit Protobag(const std::string &p) : path(p) { }
  
  std::string path;

  Result<WriteSession::Ptr> StartWriteSession(WriteSession::Spec s={}) const {
    s.archive_spec.path = path;
    return WriteSession::Create(s);
  }

  Result<ReadSession::Ptr> ReadEntries(const Selection &sel) const {
    return ReadSession::Create({
      .archive_spec = {
        .path = path,
        .mode = "read",
      },
      .selection = sel
    });
  }

  BagIndex GetIndex() const;
};

} /* namespace protobag */
