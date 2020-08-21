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
#include <queue>
#include <string>

#include "protobag/Entry.hpp"
#include "protobag/archive/Archive.hpp"
#include "protobag/Utils/Result.hpp"

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

class ReadSession final {
public:
  typedef std::shared_ptr<ReadSession> Ptr;

  struct Spec {
    archive::Archive::Spec archive_spec;
    Selection selection;
    bool unpack_stamped_messages;

    // NB: for now we *only* support time-ordered reads for stamped entries. 
    // Non-stamped are not ordered.

    static Spec ReadAllFromPath(const std::string &path) {
      Selection sel;
      sel.mutable_select_all(); // Creating an ALL means "SELECT *"
      return {
        .archive_spec = {
          .mode="read",
          .path=path,
        },
        .selection = sel,
        .unpack_stamped_messages = true,
      };
    }
  };

  static Result<Ptr> Create(const Spec &s={});

  MaybeEntry GetNext();


  // Utilities
  
  // Read just the index from `path`
  static Result<BagIndex> GetIndex(const std::string &path);

  // Get a list of all the topics from `path` (if the archive at `path`
  // has any time-series data).  NB: Ignores the protobag index.
  static Result<std::vector<std::string>> GetAllTopics(const std::string &path);

protected:
  Spec _spec;
  archive::Archive::Ptr _archive;

  bool _started = false;
  struct ReadPlan {
    std::queue<std::string> entries_to_read;
    bool require_all = true;
    bool raw_mode = false;
  };
  ReadPlan _plan;

  static MaybeEntry ReadEntryFrom(
    archive::Archive::Ptr archive,
    const std::string &entryname,
    bool raw_mode = false,
    bool unpack_stamped = true);
  
  static Result<BagIndex> ReadLatestIndex(archive::Archive::Ptr archive);

  static Result<ReadPlan> GetEntriesToRead(
    archive::Archive::Ptr archive,
    const Selection &sel);
};

} /* namespace protobag */
