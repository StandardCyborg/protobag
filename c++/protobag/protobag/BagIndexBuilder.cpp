#include "protobag/BagIndexBuilder.hpp"

#include <algorithm>
#include <queue>
#include <tuple>
#include <unordered_set>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/util/time_util.h>

#include "protobag/Utils/TopicTime.hpp"

#ifndef PROTOBAG_VERSION
#define PROTOBAG_VERSION "unknown"
#endif

namespace protobag {


struct BagIndexBuilder::TopicTimeOrderer {
  std::queue<TopicTime> observed;

  void Observe(const TopicTime &tt) {
    observed.push(tt);
  }

  template <typename RepeatedPtrFieldT>
  void MoveOrderedTTsTo(RepeatedPtrFieldT &repeated_field) {
    repeated_field.Reserve(observed.size());
    while (!observed.empty()) {
      auto tt = observed.front();
      observed.pop();
      repeated_field.Add(std::move(tt));
    }
    std::sort(repeated_field.begin(), repeated_field.end());
  }
};



struct BagIndexBuilder::DescriptorIndexer {
  std::unordered_map<
    std::string,
    ::google::protobuf::FileDescriptorSet> 
      type_url_to_fds;

  std::unordered_map<std::string, std::string> entryname_to_type_url;

  void Observe(
        const std::string &entryname,
        const std::string &type_url, 
        const ::google::protobuf::Descriptor *descriptor) {
    if (!descriptor) { return; }
    if (type_url.empty()) { return; }
    if (entryname.empty()) { return; }

    entryname_to_type_url[entryname] = type_url;


    if (type_url_to_fds.find(type_url) != type_url_to_fds.end()) {
      // Don't re-index
      return;
    }


    // Now do a BFS of the file containing `descriptor` and the file's
    // dependencies, being careful not to get caught in a cycle.
    // TODO: collect a smaller set of total descriptor defs that petain
    // only to `descriptor`.
    ::google::protobuf::FileDescriptorSet fds;
    {
      std::queue<const ::google::protobuf::FileDescriptor*> q;
      std::unordered_set<std::string> visited;
      while (!q.empty()) {
        const ::google::protobuf::FileDescriptor *current = q.front();
        q.pop();
        if (!current) { continue; } // BUG! TODO asserts
        
        if (visited.find(current->name()) != visited.end()) {
          continue;
        }

        // Visit this file
        {
          visited.insert(current->name());
            // TODO: can user have two different files with same name?
          
          ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
          current->CopyTo(fd);
        }

        // Enqueue children
        {
          for (int d = 0; d < current->dependency_count(); ++d) {
            q.push(current->dependency(d));
          }
        }
      }
    }

    type_url_to_fds[type_url] = fds;
  }

  void MoveToDescriptorPoolData(BagIndex_DescriptorPoolData &dpd) {
    {
      auto &type_url_to_descriptor = *dpd.mutable_type_url_to_descriptor();
      for (const auto &entry : type_url_to_fds) {
        type_url_to_descriptor[entry.first] = entry.second;
      }
    }

    {
      auto &entryname_to_type_url = *dpd.mutable_entryname_to_type_url();
      for (const auto &entry : entryname_to_type_url) {
        entryname_to_type_url[entry.first] = entry.second;
      }
    }
  }
};



BagIndexBuilder::BagIndexBuilder() {

  auto &start = *_index.mutable_start();
  start.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMaxSeconds);
  start.set_nanos(0);

  auto &end = *_index.mutable_end();
  end.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMinSeconds);
  end.set_nanos(0);

  _index.set_protobag_version(PROTOBAG_VERSION);

}

BagIndexBuilder::~BagIndexBuilder() {
  // NB: must declare here for PImpl pattern to work with unique_ptr
}

BagIndex_TopicStats &BagIndexBuilder::GetMutableStats(const std::string &topic) {
  auto &topic_to_stats = *_index.mutable_topic_to_stats();
  if (!topic_to_stats.contains(topic)) {
    auto &stats = topic_to_stats[topic];
    stats.set_n_messages(0);
  }
  return topic_to_stats[topic];
}

// uint64_t BagIndexBuilder::GetNextFilenum(const std::string &topic) {
//   const auto &stats = GetMutableStats(topic);
//   return stats.n_messages() + 1;
// }


void BagIndexBuilder::Observe(
    const Entry &entry, const std::string &final_entryname) {
  
  const std::string entryname = 
    final_entryname.empty() ? entry.entryname : final_entryname;

  if (_do_timeseries_indexing) {
    const auto &maybe_tt = entry.GetTopicTime();
    if (maybe_tt.has_value()) {
      const TopicTime tt = *maybe_tt;

      {
        auto &stats = GetMutableStats(tt.topic());
        stats.set_n_messages(stats.n_messages() + 1);
      }

      {
        if (!_tto) {
          _tto.reset(new TopicTimeOrderer());
        }
        _tto->Observe(tt);
      }

      {
        const auto &t = tt.timestamp();
        *_index.mutable_start() = std::min(_index.start(), t);
        *_index.mutable_end() = std::max(_index.end(), t);
      }
    }
  }

  if (_do_descriptor_indexing && entry.ctx.has_value()) {
    if (!_desc_idx) {
      _desc_idx.reset(new DescriptorIndexer());
    }

    const ::google::protobuf::Descriptor *descriptor = entry.ctx->descriptor;
    const std::string &inner_type_url = entry.ctx->inner_type_url;
      // If either of these are empty: BUG! TODO asserts
    
    _desc_idx->Observe(entryname, inner_type_url, descriptor);

    if (entry.IsStampedMessage()) {
      _desc_idx->Observe(
        "_protobag.StampedMessage",
        GetTypeURL<StampedMessage>(), 
        StampedMessage().GetDescriptor());
    }
  }
}

BagIndex BagIndexBuilder::Complete(UPtr &&builder) {
  BagIndex index;

  if (!builder) { return index; }

  // Steal meta and time-ordered entries to avoid large copies
  index = std::move(builder->_index);
  if (builder->_do_timeseries_indexing) {
    if (builder->_tto) {
      auto ttq = std::move(builder->_tto);
      ttq->MoveOrderedTTsTo(*index.mutable_time_ordered_entries());
    }
  }
  if (builder->_do_descriptor_indexing) {
    if (builder->_desc_idx) {
      auto desc_idx = std::move(builder->_desc_idx);
      desc_idx->MoveToDescriptorPoolData(
        *index.mutable_descriptor_pool_data());
    }
  }

  return index;
}


} /* namespace protobag */
