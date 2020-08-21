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

#include <memory>

#include "protobag/BagIndexBuilder.hpp"
#include "protobag/Entry.hpp"
#include "protobag/archive/Archive.hpp"

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

class WriteSession final {
public:
  typedef std::shared_ptr<WriteSession> Ptr;
  ~WriteSession() { Close(); }

  struct Spec {
    archive::Archive::Spec archive_spec;
    bool save_timeseries_index = true;
    bool save_descriptor_index = true;

    static Spec WriteToTempdir() {
      return {
        .archive_spec = archive::Archive::Spec::WriteToTempdir()
      };
    }

    bool ShouldDoIndexing() const {
      return save_timeseries_index || save_descriptor_index;
    }
  };

  static Result<Ptr> Create(const Spec &s=Spec::WriteToTempdir());

  OkOrErr WriteEntry(const Entry &entry, bool use_text_format=false);

  // Explicitly close this session, which writes an index, flushes all data,
  // to disk, and invalidates this WriteSession.
  void Close();

protected:
  Spec _spec;
  archive::Archive::Ptr _archive;
  BagIndexBuilder::UPtr _indexer;
};

} /* namespace protobag */
