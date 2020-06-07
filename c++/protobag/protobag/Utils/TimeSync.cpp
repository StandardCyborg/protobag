#include "protobag/Utils/TimeSync.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <unordered_map>

#include <google/protobuf/util/time_util.h>
#include <fmt/format.h>

#include "protobag/archive/Archive.hpp"
#include "protobag/Utils/IterProducts.hpp"
#include "protobag/Utils/TopicTime.hpp"

using Duration = ::google::protobuf::Duration;
using Timestamp = ::google::protobuf::Timestamp;

namespace protobag {

bool MaybeBundle::IsNotFound() const {
  return error == archive::Archive::ReadStatus::EntryNotFound().error;
}



struct TopicQ {
  std::map<Timestamp, Entry> q;

  void PopMostStale() {
    Timestamp t = MaxTimestamp();
    for (const auto &qe : q) {
      t = std::min(t, qe.first);
    }
    Pop(t);
  }
  
  std::optional<Entry> Pop(const Timestamp &t) {
    auto it = q.find(t);
    if (it == q.end()) {
      return std::nullopt;
    } else {
      Entry entry = std::move(it->second);
      q.erase(it);
      return std::move(entry);
    }
  }

  void Push(const Timestamp &t, Entry &&entry) {
    q.insert({t, entry});
  }
  
  size_t Size() const { return q.size(); }
  bool IsEmpty() const { return q.empty(); }

  std::vector<Timestamp> GetTimestamps() const {
    std::vector<Timestamp> out;
    out.reserve(q.size());
    for (const auto &qe : q) { out.push_back(qe.first); }
    return out;
  }
};


// Given all the timestamps in all queues `all_q_stamps`, examine every
// combination of stamps, and return the bundle of stamps (one from each queue)
// with minimum total duration (and duration no greater than max_slop). If
// there are no qualifying bundles, return the empty list.
std::vector<Timestamp> FindMinCostBundle(
    const std::vector<std::vector<Timestamp>> &all_q_stamps,
    ::google::protobuf::Duration max_slop) {

  // Compute and return the total duration of the stamps indicated at `indices`
  auto TotalDuration = [&](const std::vector<size_t> &indices) -> Duration {
    Timestamp start = MaxTimestamp();
    Timestamp end = MinTimestamp();
    for (size_t qid = 0; qid < all_q_stamps.size(); ++qid) {
      const size_t &tid = indices[qid];
      const Timestamp &t = all_q_stamps[qid][tid];
      start = std::min(start, t);
      end = std::max(end, t);
    }
    return end - start;
  };

  // Collect stamps indicated at `indices`
  auto MakeCandidate = [&](const std::vector<size_t> &indices) {
    std::vector<Timestamp> ts;
    ts.reserve(all_q_stamps.size());
    for (size_t qid = 0; qid < all_q_stamps.size(); ++qid) {
      const size_t &tid = indices[qid];
      const Timestamp &t = all_q_stamps[qid][tid];
      ts.push_back(t);
    }
    return ts;
  };

  // Set up the algo
  std::vector<size_t> q_sizes;
  {
    q_sizes.reserve(all_q_stamps.size());
    for (const auto &q : all_q_stamps) { q_sizes.push_back(q.size()); }
  }

  // Iterate through all combinations of queue timestamps (cross product)
  // and find a bundle of stamps that has the minimum total duration; ignore
  // any bundle with duration greater than max_slop.
  std::vector<Timestamp> best_candidate;
  Duration best_duration;
  {
    best_duration.set_seconds(
      ::google::protobuf::util::TimeUtil::kDurationMaxSeconds);
    best_duration.set_nanos(0);
  }
  
  IterProducts iter_prods(std::move(q_sizes));
  auto current_prod = iter_prods.GetNext();
  while (!current_prod.IsEndOfSequence()) {
    Duration current_dur = TotalDuration(current_prod.indices);
    if (current_dur <= max_slop && current_dur < best_duration) {
      best_candidate = MakeCandidate(current_prod.indices);
    }
  }
  
  return best_candidate;
}


struct MaxSlopTimeSync::Impl {
  MaxSlopTimeSync::Spec spec;
  std::unordered_map<std::string, TopicQ> topic_to_q;
  std::vector<std::string> topics_ordered;

  explicit Impl(const MaxSlopTimeSync::Spec &s) {
    spec = s;
    for (const auto &topic : s.topics) {
      topic_to_q[topic] = {};
      topics_ordered.push_back(topic);
    }
    std::sort(topics_ordered.begin(), topics_ordered.end());
  }

  void Enqueue(Entry &&entry) {
    const auto &maybeTT = entry.GetTopicTime();
    if (!maybeTT.has_value()) {
      return;
    }
    const TopicTime &tt = *maybeTT;

    if (topic_to_q.find(tt.topic()) != topic_to_q.end()) {
      auto &topic_q = topic_to_q[tt.topic()];
      if (topic_q.Size() >= spec.max_queue_size) {
        topic_q.PopMostStale();
      }
      topic_q.Push(tt.timestamp(), std::move(entry));
    }
  }

  MaybeBundle TryGetNext() {
    static const MaybeBundle kNoBundle = MaybeBundle::EndOfSequence();
    if (topic_to_q.empty()) { return kNoBundle; }

    // To create a bundle, each queue must have at least one entry
    for (const auto &tq : topic_to_q) {
      if (tq.second.IsEmpty()) {
        return kNoBundle;
      }
    }
    
    return TryCreateBundle();
  }

  MaybeBundle TryCreateBundle() {
    static const MaybeBundle kNoBundle = MaybeBundle::EndOfSequence();
    
    std::vector<std::vector<Timestamp>> all_q_stamps;
    all_q_stamps.reserve(topic_to_q.size());
    for (const auto &topic : topics_ordered) {
      all_q_stamps.push_back(topic_to_q[topic].GetTimestamps());
    }

    auto maybe_bundle_ts = FindMinCostBundle(all_q_stamps, spec.max_slop);
    if (maybe_bundle_ts.empty()) {
      return kNoBundle;
    } else {

      EntryBundle bundle;
      for (size_t qid = 0; qid < maybe_bundle_ts.size(); ++qid) {
        const auto &topic = topics_ordered[qid];
        auto &q = topic_to_q[topic];
        const Timestamp &q_t = maybe_bundle_ts[qid];

        auto maybe_entry = q.Pop(q_t);
        if (!maybe_entry.has_value()) {
          return MaybeBundle::Err(fmt::format(
            ("Programming error: tried to find entry at time {} for "
              "queue {} but entry was missing"),
            ::google::protobuf::util::TimeUtil::ToString(q_t),
            topic));
        }
        bundle.push_back(std::move(*maybe_entry));
      }
      return MaybeBundle::Ok(std::move(bundle));

    }
  }
};

Result<TimeSync::Ptr> MaxSlopTimeSync::Create(
    const ReadSession::Ptr &rs,
    const Spec &spec) {

  if (!rs) {
    return {.error = "Null read session; nothing to read"};
  }

  auto *sync = new MaxSlopTimeSync();
  TimeSync::Ptr p(sync);

  sync->_read_sess = rs;
  sync->_spec = spec;
  sync->_impl.reset(new Impl(spec));

  return {.value = p};
}

MaybeBundle MaxSlopTimeSync::GetNext() {
  if (!_impl) {
    return MaybeBundle::Err("Programming error: impl not initialized");
  }

  auto maybe_next_bundle = _impl->TryGetNext();
  if (maybe_next_bundle.IsOk()) {
    return maybe_next_bundle;
  } else {

    if (!_read_sess) {
      return MaybeBundle::Err("Programming error: null read session");
    }
    ReadSession &rs = *_read_sess; 
    
    bool reading = true;
    while (reading) {
      auto maybe_next_entry = rs.GetNext();
      if (!maybe_next_entry.IsOk()) { 
        reading = false;
        return MaybeBundle::Err(maybe_next_entry.error);
      }

      _impl->Enqueue(std::move(*maybe_next_entry.value));
      auto maybe_next_bundle = _impl->TryGetNext();
      if (maybe_next_bundle.IsOk()) {
        return maybe_next_bundle;
      } // else continue reading; maybe we'll get a bundle next time
    }

    return MaybeBundle::EndOfSequence();
  }
}

} /* namespace protobag */
