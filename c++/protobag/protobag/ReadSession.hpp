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

    // NB: for now we *only* support time-ordered reads for stamped entries. non-stamped are not ordered

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

  // TODO: begin() end() interface? see do-while loop in demo
  // TODO TypeResolver halper ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  // Utilities
  
  // Read just the index from `path`
  static Result<BagIndex> GetIndex(const std::string &path);

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

  // maybe move these and make public ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
  // static Result<BagIndex> GetReindexed(archive::Archive::Ptr archive);
  
  

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
