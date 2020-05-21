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

  void Close();

protected:
  Spec _spec;
  archive::Archive::Ptr _archive;
  BagIndexBuilder::UPtr _indexer;
};

} /* namespace protobag */
