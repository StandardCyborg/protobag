#include "protobag/ReadSession.hpp"

#include <filesystem>
#include <set>

#include <fmt/format.h>
#include <google/protobuf/util/time_util.h>

                                                                          #include <iostream>

#include "protobag/BagMetaBuilder.hpp"
#include "protobag/Utils/PBUtils.hpp"


namespace fs = std::filesystem;

namespace protobag {

Result<ReadSession::Ptr> ReadSession::Create(const ReadSession::Spec &s) {
  auto maybe_archive = archive::Archive::Open(s.archive_spec);
  if (!maybe_archive.IsOk()) {
    return {.error = maybe_archive.error};
  }

  ReadSession::Ptr r(new ReadSession());
  r->_archive = std::move(*maybe_archive.value);
  r->_spec = s;

  return {.value = r};
}

MaybeEntry ReadSession::GetNext() {
  if (!_archive) {
    return MaybeEntry::Err("Programming Error: no archive open for writing");
  }

  // TODO: make a lot faster ... 
  if (!_started) {
    auto maybe_entries_to_read = GetEntriesToRead(_archive, _spec.selection);
    if (!maybe_entries_to_read.IsOk()) {
      return MaybeEntry::Err(
        fmt::format("Could not select entries to read: {}", maybe_entries_to_read.error));
    }
    _entries_to_read = *maybe_entries_to_read.value;
    _started = true;
  }

  if (_entries_to_read.empty()) {
    return MaybeEntry::EndOfSequence();
  }
std::cout << "_entries_to_read " << _entries_to_read.size() << std::endl;
  std::string entryname = _entries_to_read.front();
std::cout << "entryname " << entryname << std::endl;
  _entries_to_read.pop();

  auto maybe_stamped_msg = ReadMessageFrom(_archive, entryname);
  if (!maybe_stamped_msg.IsOk()) {
    return MaybeEntry::Err(
      fmt::format(
        "Could not decode StampedMessage from {}, error {} ", 
        entryname, maybe_stamped_msg.error)
    );
  }

  return MaybeEntry::Ok({
    .topic = GetTopicFromEntryname(entryname),
    .stamped_msg = *maybe_stamped_msg.value,
  });
}


Result<BagMeta> ReadSession::GetIndex(const std::string &path) {
  auto maybe_r = ReadSession::Create(ReadSession::Spec::ReadAllFromPath(path));
  if (!maybe_r.IsOk()) {
    return {.error = maybe_r.error};
  }

  auto rp = *maybe_r.value;
  if (!rp) {
    return {.error = fmt::format("Failed to read {}", path)};
  }

  return ReadLatestIndex(rp->_archive);
}


std::string ReadSession::GetTopicFromEntryname(const std::string &entryname) {
  // return std::string("/") +  // fixme ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //     fs::path(entryname).parent_path().u8string();
  return 
      fs::path(entryname).parent_path().u8string();
}

Result<StampedMessage> ReadSession::ReadMessageFrom(
    archive::Archive::Ptr archive,
    const std::string &entryname) {

  if (!archive) {
    return {.error = "No archive to read"};
  }

  const auto maybe_bytes = archive->ReadAsStr(entryname);
  if (!maybe_bytes.IsOk()) {
    return {.error = 
      fmt::format("Read error for {}: {}", entryname, maybe_bytes.error)
    };
  }

  return PBFactory::LoadFromContainer<StampedMessage>(*maybe_bytes.value);
}

Result<BagMeta> ReadSession::GetReindexed(archive::Archive::Ptr archive) {
  if (!archive) {
    return {.error = "No archive to read"};
  }

  BagMetaBuilder::UPtr builder(new BagMetaBuilder());
  auto namelist = archive->GetNamelist();
  for (const auto &name : namelist) {
                                                std::cout << "name " << name << std::endl;
    auto maybe_stamped_msg = ReadMessageFrom(archive, name);
    if (!maybe_stamped_msg.IsOk()) {
      return {.error = 
        fmt::format(
          "Could not decode StampedMessage from {}, error {} ", 
          name, maybe_stamped_msg.error)
        };
    }

    builder->Observe(
      Entry{
        .topic = GetTopicFromEntryname(name),
        .stamped_msg = *maybe_stamped_msg.value,
      },
      name);
  }

  return {.value = BagMetaBuilder::Complete(std::move(builder))};
}

Result<BagMeta> ReadSession::ReadLatestIndex(archive::Archive::Ptr archive) {
  if (!archive) {
    return {.error = "No archive to read"};
  }

  std::optional<StampedMessage> index_entry;
  {
    auto namelist = archive->GetNamelist();
    for (const auto &entryname : namelist) {
      if (EntryIsInTopic(entryname, "/_protobag_meta/bag_meta")) {
        auto maybe_stamped_msg = ReadMessageFrom(archive, entryname);
        if (maybe_stamped_msg.IsOk()) {
          const StampedMessage &cur_msg = *maybe_stamped_msg.value;
          if (!index_entry.has_value() ||
                (cur_msg.timestamp() < index_entry->timestamp())) {
            index_entry = cur_msg;
          }
        }
      }
    }
  }

  if (index_entry.has_value()) {
    return PBFactory::UnpackFromAny<BagMeta>(index_entry->msg());
  } else {
    return {.error = "Could not find an index"};
  }

}

Result<std::queue<std::string>> ReadSession::GetEntriesToRead(
    archive::Archive::Ptr archive,
    const Selection &sel) {

  if (!archive) {
    return {.error = "No archive to read"};
  }

  auto maybe_index = ReadLatestIndex(archive);
  if (!maybe_index.IsOk()) {
    // Then create one!
    maybe_index = GetReindexed(archive);
  }

  if (!maybe_index.IsOk()) {
    return {.error = 
      fmt::format(
        "Could not index or read index from {} : {}",
        archive->ToString(),
        maybe_index.error)
    };
  }

  const BagMeta &index = *maybe_index.value;
        std::cout << " index " << *PBFactory::ToTextFormatString(index).value << std::endl;

  if (sel.has_events()) {
    const Selection_Events &sel_events = sel.events();

    std::set<TopicTime> events;
    for (size_t i = 0; i < sel_events.events_size(); ++i) {
      TopicTime tt = sel_events.events(i);
      tt.set_entryname(""); // Do not match on archive entryname
      events.insert(sel_events.events(i));
    }

    std::queue<std::string> entries_to_read;
    for (size_t i = 0; i < index.time_ordered_entries_size(); ++i) {
      TopicTime tt = index.time_ordered_entries().Get(i);
      std::string entryname = tt.entryname();
      tt.set_entryname(""); // Do not match on archive entryname
      if (events.find(tt) != events.end()) {
        entries_to_read.push(entryname);
      }
    }

    return {.value = entries_to_read};


  } else if (sel.has_window()) {

    const Selection_Window &window = sel.window();
    
    std::set<std::string> exclude_topics;
    for (size_t i = 0; i < window.exclude_topics_size(); ++i) {
      exclude_topics.insert(window.exclude_topics(i));
    }

    std::set<std::string> include_topics;
    for (size_t i = 0; i < window.topics_size(); ++i) {
      include_topics.insert(window.topics(i));
    }

    std::queue<std::string> entries_to_read;
    for (size_t i = 0; i < index.time_ordered_entries_size(); ++i) {
      TopicTime tt = index.time_ordered_entries().Get(i);
      
      if (!exclude_topics.empty() &&
            (exclude_topics.find(tt.topic()) != exclude_topics.end())) {
        continue;
      }

      if (!include_topics.empty() &&
            (include_topics.find(tt.topic()) == include_topics.end())) {
        continue;
      }

      if (window.has_start() && (tt.timestamp() < window.start())) {
        continue;
      }

      if (window.has_end() && (window.end() < tt.timestamp())) {
        continue;
      }
std::cout << "tt.entryname() " << tt.entryname() << std::endl;
      entries_to_read.push(tt.entryname());
    }
    return {.value = entries_to_read};

  } else {
    // We don't support whatever criteria `sel` has
    std::string sel_txt;
    {
      auto maybe_pb_txt = PBFactory::ToTextFormatString(sel);
      if (maybe_pb_txt.IsOk()) {
        sel_txt = *maybe_pb_txt.value;
      }
    }
    return {.error = 
      fmt::format("Unsupported selection {}", sel_txt)
    };
  }
}


} /* namespace protobag */
