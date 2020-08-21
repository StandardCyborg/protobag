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
#include <string>

#include "protobag/Entry.hpp"

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

/**
 * BagIndexBuilder fulfills the observer pattern and builds an "index" of
 * `Entry`s written to a protobag.  Current indexing features:
 *  * Timeseries Indexing: the topics and timestamps of Stamped `Entry`s are
 *     indexed to facilitate time-ordered playback (e.g. entries could be
 *     written out-of-order).  Also collects other stats.
 *  * Descriptor Indexing: the ::google::protobuf::Descriptor data for each
 *     message is saved so that messages can be decoded even when the
 *     user lacks protoc-generated code for the messages.  FMI see
 *     `protobag::DynamicMsgFactory`.
 */
class BagIndexBuilder final {
public:
  typedef std::unique_ptr<BagIndexBuilder> UPtr;
  BagIndexBuilder();
  ~BagIndexBuilder();

  void DoTimeseriesIndexing(bool v) { _do_timeseries_indexing = v; }
  void DoDescriptorIndexing(bool v) { _do_descriptor_indexing = v; }
  bool IsTimeseriesIndexing() const { return _do_timeseries_indexing; }
  bool IsDescriptorIndexing() const { return _do_descriptor_indexing; }

  void Observe(const Entry &entry, const std::string &final_entryname="");

  // Completes the indexing for `builder` and returns a file `BagIndex`.  This
  // process moves some resources directly to `BagIndex` from `builder`, so 
  // the given `builder` instance is consumed.
  static BagIndex Complete(UPtr &&builder);

protected:
  BagIndex _index;

  bool _do_timeseries_indexing = true;
  bool _do_descriptor_indexing = true;

  struct TopicTimeOrderer;
  std::unique_ptr<TopicTimeOrderer> _tto;

  struct DescriptorIndexer;
  std::unique_ptr<DescriptorIndexer> _desc_idx;

  BagIndex_TopicStats &GetMutableStats(const std::string &topic);
};

} /* namespace protobag */