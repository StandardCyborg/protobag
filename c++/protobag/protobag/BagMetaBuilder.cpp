#include "protobag/BagMetaBuilder.hpp"

#include <queue>
#include <tuple>

#include <google/protobuf/util/time_util.h>


namespace protobag {

BagMetaBuilder::BagMetaBuilder() {

  auto &start = *_meta.mutable_start();
  start.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMaxSeconds);
  start.set_nanos(0);

  auto &end = *_meta.mutable_end();
  end.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMinSeconds);
  end.set_nanos(0);

  _meta.set_protobag_version("TODO"); // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`

}

BagMetaBuilder::~BagMetaBuilder() {
  // NB: must declare here for TopicTimePQ PImpl pattern to work with unique_ptr
}

struct BagMetaBuilder::TopicTimePQ {
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




BagMeta_TopicStats &BagMetaBuilder::GetMutableStats(const std::string &topic) {
  auto &topic_to_stats = *_meta.mutable_topic_to_stats();
  if (!topic_to_stats.contains(topic)) {
    auto &stats = topic_to_stats[topic];
    stats.set_n_messages(0);
  }
  return topic_to_stats[topic];
}

uint64_t BagMetaBuilder::GetNextFilenum(const std::string &topic) {
  const auto &stats = GetMutableStats(topic);
  return stats.n_messages() + 1;
}


void BagMetaBuilder::Observe(const Entry &entry, const std::string &entryname) {
  
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
    *_meta.mutable_start() = std::min(_meta.start(), t);
    *_meta.mutable_end() = std::max(_meta.end(), t);
  }

}

BagMeta BagMetaBuilder::Complete(UPtr &&builder) {
  BagMeta meta;

  if (!builder) { return meta; }

  // Steal meta and time-ordered entries to avoid large copies
  meta = std::move(builder->_meta);
  if (builder->_ttq) {
    auto ttq = std::move(builder->_ttq);
    ttq->MoveOrderedTTsTo(*meta.mutable_time_ordered_entries());
  }
  return meta;
}


} /* namespace protobag */