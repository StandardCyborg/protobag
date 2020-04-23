#pragma once

#include <memory>

#include "protobag/BagMetaBuilder.hpp"
#include "protobag/Entry.hpp"
#include "protobag/archive/Archive.hpp"

#include "ProtobagMsg.pb.h"

namespace protobag {

class WriteSession final {
public:
  typedef std::shared_ptr<WriteSession> Ptr;
  ~WriteSession() { Close(); }

  struct Spec {
    archive::Archive::Spec archive_spec;
    bool save_meta_index = true;

    static Spec WriteToTempdir() {
      return {
        .archive_spec = archive::Archive::Spec::WriteToTempdir()
      };
    }
  };

  static Result<Ptr> Create(const Spec &s=Spec::WriteToTempdir());

  OkOrErr WriteEntry(const Entry &entry);

  void Close();

protected:
  Spec _spec;
  archive::Archive::Ptr _archive;
  BagMetaBuilder::UPtr _indexer;
};

} /* namespace protobag */
