#include "protobag/ReadSession.hpp"

#include <filesystem>
#include <list>
#include <set>
#include <sstream>

#include <fmt/format.h>
#include <google/protobuf/util/time_util.h>

                                                                          #include <iostream>

#include "protobag/BagIndexBuilder.hpp"
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

MaybeEntry ReadSession::ReadEntryFrom(
      archive::Archive::Ptr archive,
      const std::string &entryname,
      bool raw_mode) {

  if (!archive) {
    return MaybeEntry::Err("No archive to read");
  }

  const auto maybe_bytes = archive->ReadAsStr(entryname);
  if (maybe_bytes.IsEntryNotFound()) {
    return MaybeEntry::Err(maybe_bytes.error);
  } else if (!maybe_bytes.IsOk()) {
    return MaybeEntry::Err(
      fmt::format("Read error for {}: {}", entryname, maybe_bytes.error));
  }

  if (raw_mode) {
    
    Entry entry;
    entry.entryname = entryname;
    entry.msg.set_value(std::move(*maybe_bytes.value));
    return MaybeEntry::Ok(std::move(entry));

  } else {

    auto maybe_any = 
      PBFactory::LoadFromContainer<google::protobuf::Any>(*maybe_bytes.value);
        // do we need to handle text format separately ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if (!maybe_any.IsOk()) {
      return MaybeEntry::Err(fmt::format(
        "Could not read protobuf from {}: {}", entryname, maybe_any.error));
    }

    return MaybeEntry::Ok({
      .entryname = entryname,
      .msg = std::move(*maybe_any.value),
    });

  }
}

MaybeEntry ReadSession::GetNext() {
  // TODO: make a lot faster ... 
  if (!_started) {
    auto maybe_entries_to_read = GetEntriesToRead(_archive, _spec.selection);
    if (!maybe_entries_to_read.IsOk()) {
      return MaybeEntry::Err(
        fmt::format(
          "Could not select entries to read: \n{}",
          maybe_entries_to_read.error));
    }
    _plan = *maybe_entries_to_read.value;
    _started = true;
  }

  if (_plan.entries_to_read.empty()) {
    return MaybeEntry::EndOfSequence();
  }
std::cout << "_entries_to_read " << _plan.entries_to_read.size() << std::endl;
  std::string entryname = _plan.entries_to_read.front();
std::cout << "entryname " << entryname << std::endl;
  _plan.entries_to_read.pop();

  if (!_archive) {
    return MaybeEntry::Err("Programming Error: no archive open for writing");
  }

  auto maybe_entry = ReadEntryFrom(_archive, entryname, _plan.raw_mode);
  if (maybe_entry.IsNotFound()) {
    if (_plan.require_all) {
      return MaybeEntry::Err(fmt::format("Entry not found: {}", entryname));
    } else {
      return GetNext();
    }
  } else {
    return maybe_entry;
  }
}





//   auto maybe_stamped_msg = ReadMessageFrom(_archive, entryname);
//   if (!maybe_stamped_msg.IsOk()) {
//     return MaybeEntry::Err(
//       fmt::format(
//         "Could not decode StampedMessage from {}, error {} ", 
//         entryname, maybe_stamped_msg.error)
//     );
//   }

//   return MaybeEntry::Ok({
//     .topic = GetTopicFromEntryname(entryname),
//     .stamped_msg = *maybe_stamped_msg.value,
//   });
// }


Result<BagIndex> ReadSession::GetIndex(const std::string &path) {
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

// Result<std::string> ReadSession::ReadMessageFrom(
//     archive::Archive::Ptr archive,
//     const std::string &entryname) {

//   if (!archive) {
//     return {.error = "No archive to read"};
//   }

  

//   return PBFactory::LoadFromContainer<StampedMessage>(*maybe_bytes.value);
// }

// Result<BagIndex> ReadSession::GetReindexed(archive::Archive::Ptr archive) {
//   TODO if we bring this back, use stampedmsg in filename to key off sniffing?
//   if (!archive) {
//     return {.error = "No archive to read"};
//   }

//   BagIndexBuilder::UPtr builder(new BagIndexBuilder());
//   auto namelist = archive->GetNamelist();
//   for (const auto &name : namelist) {
//                                                 std::cout << "name " << name << std::endl;
//     auto maybe_stamped_msg = ReadMessageFrom(archive, name);
//     if (!maybe_stamped_msg.IsOk()) {
//       return {.error = 
//         fmt::format(
//           "Could not decode StampedMessage from {}, error {} ", 
//           name, maybe_stamped_msg.error)
//         };
//     }

//     builder->Observe(
//       Entry{
//         .topic = GetTopicFromEntryname(name),
//         .stamped_msg = *maybe_stamped_msg.value,
//       },
//       name);
//   }

//   return {.value = BagIndexBuilder::Complete(std::move(builder))};
// }

Result<BagIndex> ReadSession::ReadLatestIndex(archive::Archive::Ptr archive) {
  if (!archive) {
    return {.error = "No archive to read"};
  }

  std::optional<StampedMessage> index_entry;
  {
    auto namelist = archive->GetNamelist();
    for (const auto &entryname : namelist) {
      if (EntryIsInTopic(entryname, "/_protobag_index/bag_index")) {
        auto maybe_entry = ReadEntryFrom(archive, entryname);
        if (maybe_entry.IsOk()) {
          auto maybe_stamped_msg = maybe_entry.value->GetAs<StampedMessage>();
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
  }

  if (index_entry.has_value()) {
    return PBFactory::UnpackFromAny<BagIndex>(index_entry->msg());
  } else {
    return {.error = "Could not find an index"};
  }

}

Result<ReadSession::ReadPlan> ReadSession::GetEntriesToRead(
    archive::Archive::Ptr archive,
    const Selection &sel) {

  if (!archive) {
    return {.error = "No archive to read"};
  }

  auto maybe_index = ReadLatestIndex(archive); // TODO support multiple indices ~~~~~~~~~~~~~~~~
  if (!maybe_index.IsOk()) {
    // // Then create one!
    // maybe_index = GetReindexed(archive);
    return {.error = "Unindexed protobag not supported right now"}; // ~~~~~~~~~~~~~~~~~~~~~~~~~
  }

  if (!maybe_index.IsOk()) {
    return {.error = 
      fmt::format(
        "Could not index or read index from {} : {}",
        archive->ToString(),
        maybe_index.error)
    };
  }

  const BagIndex &index = *maybe_index.value;
        std::cout << " index " << *PBFactory::ToTextFormatString(index).value << std::endl;

  if (sel.has_select_all()) {

    auto namelist = archive->GetNamelist();
    std::queue<std::string> entries_to_read;
    for (const auto &n : namelist) { entries_to_read.push(n); }
    return {.value = ReadPlan{
      .entries_to_read = entries_to_read,
      .require_all = false,
      .raw_mode = sel.select_all().all_entries_are_raw(),
    }};

  } else if (sel.has_entrynames()) {

    const Selection_Entrynames &sel_entrynames = sel.entrynames();
    std::queue<std::string> entries_to_read;
    for (int i = 0; i < sel_entrynames.entrynames_size(); ++i) {
      entries_to_read.push(sel_entrynames.entrynames(i));
    }
    return {.value = ReadPlan{
      .entries_to_read = entries_to_read,
      .require_all = !sel_entrynames.ignore_missing_entries(),
      .raw_mode = sel_entrynames.entries_are_raw(),
    }};

  } else if (sel.has_events()) {

    const Selection_Events &sel_events = sel.events();

    std::set<TopicTime> events;
    for (size_t i = 0; i < sel_events.events_size(); ++i) {
      TopicTime tt = sel_events.events(i);
      tt.set_entryname(""); // Do not match on archive entryname
      events.insert(tt);
    }

    std::queue<std::string> entries_to_read;
    std::list<TopicTime> missing_entries;
    for (size_t i = 0; i < index.time_ordered_entries_size(); ++i) {
      TopicTime tt = index.time_ordered_entries().Get(i);
      std::string entryname = tt.entryname();
      tt.set_entryname(""); // Do not match on archive entryname
      if (events.find(tt) != events.end()) {
        entries_to_read.push(entryname);
      } else if (sel_events.require_all()) {
        missing_entries.push_back(tt);
      }
    }

    if (sel_events.require_all() && !missing_entries.empty()) {
      std::stringstream ss;
      for (const auto &missing : missing_entries) {
        auto maybe_txt = PBFactory::ToTextFormatString(missing);
        if (!maybe_txt.IsOk()) {
          return {.error = maybe_txt.error};
        }
        ss << *maybe_txt.value << "\n";
      }
      return {.error = fmt::format((
        "Could not find all requested entries and all were required.  "
        "Missing: \n{}"), ss.str())
      };
    }

    return {.value = ReadPlan{
      .entries_to_read = entries_to_read,
      .require_all = sel_events.require_all(),
      .raw_mode = false,
    }};

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
    return {.value = ReadPlan{
      .entries_to_read = entries_to_read,
      .require_all = false, // TODO should we report if index and archive don't match? ~~~~~~~~~~~~~~
      .raw_mode = false,
    }};

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
