#include "protobag/BagIndexBuilder.hpp"

#include <queue>
#include <tuple>

#include <google/protobuf/util/time_util.h>


namespace protobag {

BagIndexBuilder::BagIndexBuilder() {

  auto &start = *_index.mutable_start();
  start.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMaxSeconds);
  start.set_nanos(0);

  auto &end = *_index.mutable_end();
  end.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMinSeconds);
  end.set_nanos(0);

  _index.set_protobag_version("TODO"); // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`

}

BagIndexBuilder::~BagIndexBuilder() {
  // NB: must declare here for TopicTimePQ PImpl pattern to work with unique_ptr
}

struct BagIndexBuilder::TopicTimePQ {
  std::priority_queue<TopicTime, std::vector<TopicTime>, std::greater<TopicTime>> observed;

  void Observe(const TopicTime &tt) {
    observed.push(tt);
  }

  template <typename RepeatedPtrFieldT>
  void MoveOrderedTTsTo(RepeatedPtrFieldT &repeated_field) {
    repeated_field.Reserve(observed.size());
    while (!observed.empty()) {
      auto top = observed.top();
      observed.pop();
      repeated_field.Add(std::move(top));
    }
  }
};




BagIndex_TopicStats &BagIndexBuilder::GetMutableStats(const std::string &topic) {
  auto &topic_to_stats = *_index.mutable_topic_to_stats();
  if (!topic_to_stats.contains(topic)) {
    auto &stats = topic_to_stats[topic];
    stats.set_n_messages(0);
  }
  return topic_to_stats[topic];
}

uint64_t BagIndexBuilder::GetNextFilenum(const std::string &topic) {
  const auto &stats = GetMutableStats(topic);
  return stats.n_messages() + 1;
}


void BagIndexBuilder::Observe(const Entry &entry, const std::string &entryname) {
  
  if (_do_timeseries_indexing) {
    {
      auto &stats = GetMutableStats(entry.topic);
      stats.set_n_messages(stats.n_messages() + 1);
    }

    {
      TopicTime tt;
      tt.set_topic(entry.topic);
      *tt.mutable_timestamp() = entry.stamped_msg.timestamp();
      tt.set_entryname(entryname);

      if (!_ttq) {
        _ttq.reset(new TopicTimePQ());
      }
      _ttq->Observe(tt);
    }

    {
      const auto &t = entry.stamped_msg.timestamp();
      *_index.mutable_start() = std::min(_index.start(), t);
      *_index.mutable_end() = std::max(_index.end(), t);
    }
  }

  if (_do_descriptor_indexing) {
    
  }

}

BagIndex BagIndexBuilder::Complete(UPtr &&builder) {
  BagIndex index;

  index.set_protobag_version("TODO GET REAL VERSION");

  if (!builder) { return index; }

  // Steal meta and time-ordered entries to avoid large copies
  index = std::move(builder->_index);
  if (builder._do_timeseries_indexing) {
    if (builder->_ttq) {
      auto ttq = std::move(builder->_ttq);
      ttq->MoveOrderedTTsTo(*index.mutable_time_ordered_entries());
    }
  }
  return index;
}


} /* namespace protobag */