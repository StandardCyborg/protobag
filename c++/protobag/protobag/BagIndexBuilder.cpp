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
    repeated_field.Reserve(int(observed.size()));
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
        const ::google::protobuf::Descriptor *descriptor,
        const ::google::protobuf::FileDescriptorSet *fds) {
    
    if (!(descriptor || fds)) {
      // Nothing to index
      return;
    }

    if (type_url.empty()) { return; }
    if (entryname.empty()) { return; }

    entryname_to_type_url[entryname] = type_url;

    if (type_url_to_fds.find(type_url) != type_url_to_fds.end()) {
      // Don't re-index
      return;
    }

    if (fds) {

      // Use the included FileDescriptorSet
      type_url_to_fds[type_url] = *fds;
    
    } else if (descriptor) {

      // Do a BFS of the file containing `descriptor` and the file's
      // dependencies, being careful not to get caught in a cycle.
      // TODO: collect a smaller set of total descriptor defs that petain
      // only to `descriptor`.
      ::google::protobuf::FileDescriptorSet collected_fds;
      {
        std::queue<const ::google::protobuf::FileDescriptor*> q;
        q.push(descriptor->file());
        std::unordered_set<std::string> visited;
        while (!q.empty()) {
          const ::google::protobuf::FileDescriptor *current = q.front();
          q.pop();
          if (!current) { continue; } // BUG! All pointers should be non-null
          
          if (visited.find(current->name()) != visited.end()) {
            continue;
          }

          // Visit this file
          {
            visited.insert(current->name());
              // TODO: can user have two different files with same name?
            
            ::google::protobuf::FileDescriptorProto *fd = 
              collected_fds.add_file();
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

      type_url_to_fds[type_url] = collected_fds;

    }
  }

  void MoveToDescriptorPoolData(BagIndex_DescriptorPoolData &dpd) {
    {
      auto &type_url_to_descriptor = *dpd.mutable_type_url_to_descriptor();
      for (const auto &entry : type_url_to_fds) {
        type_url_to_descriptor[entry.first] = entry.second;
      }
    }

    {
      auto &idx_entryname_to_type_url = *dpd.mutable_entryname_to_type_url();
      for (const auto &entry : entryname_to_type_url) {
        idx_entryname_to_type_url[entry.first] = entry.second;
      }
    }
  }
};



BagIndexBuilder::BagIndexBuilder() {
  *_index.mutable_start() = MaxTimestamp();
  *_index.mutable_end() = MinTimestamp();
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

void BagIndexBuilder::Observe(
    const Entry &entry, const std::string &final_entryname) {
  
  const std::string entryname = 
    final_entryname.empty() ? entry.entryname : final_entryname;

  if (_do_timeseries_indexing) {
    if (entry.IsStampedMessage()) {
      const auto &maybe_tt = entry.GetTopicTime();
      if (maybe_tt.has_value()) {
        TopicTime tt = *maybe_tt;
        tt.set_entryname(entryname);

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
  }

  if (_do_descriptor_indexing && entry.ctx.has_value()) {
    if (!_desc_idx) {
      _desc_idx.reset(new DescriptorIndexer());
    }
    
    _desc_idx->Observe(
                  entryname,
                  entry.ctx->inner_type_url,
                  entry.ctx->descriptor,
                  entry.ctx->fds);

    if (entry.IsStampedMessage()) {
      // A hack to ensure our StampedMessage type gets indexed at least once
      // when needed
      _desc_idx->Observe(
        "_protobag.StampedMessage",
        GetTypeURL<StampedMessage>(), 
        StampedMessage().GetDescriptor(),
        nullptr);
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
