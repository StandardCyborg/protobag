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

#pragma once

#include <string>

#include "protobag/ReadSession.hpp"
#include "protobag/WriteSession.hpp"
#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

class Protobag final {
public:
  Protobag() = default;
  explicit Protobag(const std::string &p) : path(p) { }
  
  std::string path;

  Result<WriteSession::Ptr> StartWriteSession(WriteSession::Spec s={}) const {
    s.archive_spec.path = path;
    if (s.archive_spec.mode.empty()) {
      s.archive_spec.mode = "write";
    }
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
