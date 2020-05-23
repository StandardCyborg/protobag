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

  // uint64_t GetNextFilenum(const std::string &topic);

  void Observe(const Entry &entry, const std::string &final_entryname="");

  static BagIndex Complete(UPtr &&builder);
    // NB: Destroys builder!  TODO give an example use case in class docstring to explain ~~~~~

  void DoTimeseriesIndexing(bool v) { _do_timeseries_indexing = v; }
  void DoDescriptorIndexing(bool v) { _do_descriptor_indexing = v; }
  bool IsTimeseriesIndexing() const { return _do_timeseries_indexing; }
  bool IsDescriptorIndexing() const { return _do_descriptor_indexing; }

protected:
  BagIndex _index;

  bool _do_timeseries_indexing = true;
  bool _do_descriptor_indexing = true;

  struct TopicTimePQ;
  std::unique_ptr<TopicTimePQ> _ttq;

  struct DescriptorIndexer;
  std::unique_ptr<DescriptorIndexer> _desc_idx;

  BagIndex_TopicStats &GetMutableStats(const std::string &topic);
};

} /* namespace protobag */