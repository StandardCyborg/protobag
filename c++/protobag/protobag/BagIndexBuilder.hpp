#pragma once

#include <memory>
#include <string>
#include <vector>

#include "protobag/Entry.hpp"

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

class BagIndexBuilder final {
public:
  typedef std::unique_ptr<BagIndexBuilder> UPtr;
  BagIndexBuilder();
  ~BagIndexBuilder();

  uint64_t GetNextFilenum(const std::string &topic);

  void Observe(const Entry &entry, const std::string &entryname="");

  static BagIndex Complete(UPtr &&builder);
    // NB: Destroys builder!  TODO give an example use case in class docstring to explain ~~~~~

protected:
  BagIndex _index;

  struct TopicTimePQ;
  std::unique_ptr<TopicTimePQ> _ttq;

  BagIndex_TopicStats &GetMutableStats(const std::string &topic);
};

} /* namespace protobag */