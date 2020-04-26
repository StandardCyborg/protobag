#pragma once

#include <memory>
#include <string>
#include <vector>

#include "protobag/Entry.hpp"

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

class BagMetaBuilder final {
public:
  typedef std::unique_ptr<BagMetaBuilder> UPtr;
  BagMetaBuilder();
  ~BagMetaBuilder();

  uint64_t GetNextFilenum(const std::string &topic);

  void Observe(const Entry &entry, const std::string &entryname="");

  static BagMeta Complete(UPtr &&builder);
    // NB: Destroys builder!  TODO give an example use case in class docstring to explain ~~~~~

protected:
  BagMeta _meta;

  struct TopicTimePQ;
  std::unique_ptr<TopicTimePQ> _ttq;

  BagMeta_TopicStats &GetMutableStats(const std::string &topic);
};

} /* namespace protobag */