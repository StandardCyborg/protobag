#include "protobag/WriteSession.hpp"

#include <fmt/format.h>

#include <google/protobuf/util/time_util.h>

#include "protobag/Utils/PBUtils.hpp"


namespace protobag {

Result<WriteSession::Ptr> WriteSession::Create(const Spec &s) {
  auto maybe_archive = archive::Archive::Open(s.archive_spec);
  if (!maybe_archive.IsOk()) {
    return {.error = maybe_archive.error};
  }

  WriteSession::Ptr w(new WriteSession());
  w->_spec = s;
  w->_archive = std::move(*maybe_archive.value);
  if (s.ShouldDoIndexing()) {
    w->_indexer.reset(new BagIndexBuilder());
    if (!w->_indexer) { return {.error = "Could not allocate indexer"}; }
    w->_indexer->DoTimeseriesIndexing(s.save_timeseries_index);
    w->_indexer->DoDescriptorIndexing(s.save_descriptor_index);
  }

  return {.value = w};
}

OkOrErr WriteSession::WriteEntry(const Entry &entry, bool use_text_format) {
  if (!_archive) {
    return OkOrErr::Err("Programming Error: no archive open for writing");
  }

  std::string entryname = entry.entryname;
  if (entryname.empty()) {
    // Derive entryname from topic & time
    const auto &maybe_tt = entry.GetTopicTime();
    if (!maybe_tt.has_value()) {
      return {.error = fmt::format(
        "Invalid entry; needs entryname or topic/timestamp. {}", 
        entry.ToString())
      };
    }
    const TopicTime &tt = *maybe_tt;

    if (tt.topic().empty()) {
      return {.error = fmt::format(
        "Entry must have an entryname or a topic.  Got {}",
        entry.ToString())
      };
    }

    entryname = fmt::format(
        "{}/{}.{}.stampedmsg",
        tt.topic(),
        tt.timestamp().seconds(),
        tt.timestamp().nanos());

    entryname = 
      use_text_format ? 
        fmt::format("{}.prototxt", entryname) : 
        fmt::format("{}.protobin", entryname);
  }

  auto maybe_m_bytes = 
    use_text_format ?
      PBFactory::ToTextFormatString(entry.msg) :
      PBFactory::ToBinaryString(entry.msg);
  if (!maybe_m_bytes.IsOk()) {
    return {.error = maybe_m_bytes.error};
  }

  OkOrErr res = _archive->Write(entryname, *maybe_m_bytes.value);
  if (res.IsOk() && _indexer) {
    _indexer->Observe(entry, entryname);
  }
  return res;
}



//   if (use_text_format) {
//     entryname = fmt::format("{}.prototxt", entryname);

//     res = _archive->Write(entryname, *maybe_m_bytes.value);
//   } else {
//     entryname = fmt::format("{}.protobin", entryname);
//     res = _archive->Write(entryname, *maybe_m_bytes.value);
//   }



//   auto maybe_m_bytes = PBFactory::ToBinaryString<StampedMessage>(entry.stamped_msg);
//   if (!maybe_m_bytes.IsOk()) {
//     return OkOrErr::Err(maybe_m_bytes.error);
//   }

//   if (!IsProtoBagIndexTopic(entry.topic)) {
//     if (!_indexer) {
//       return OkOrErr::Err("Programming Error: no indexer (at least for file counter)");
//     }
//     auto next_filenum = _indexer->GetNextFilenum(entry.topic);
//     std::string entryname = fmt::format(
//       "{}/{}.protobin", entry.topic, next_filenum);

//     OkOrErr res = _archive->Write(entryname, *maybe_m_bytes.value);
//     if (res.IsOk() && _indexer) {
//       _indexer->Observe(entry, entryname);
//     }

//     return res;

//   } else {

//     // The `/_protobag_index` topic gets no indexing
//     std::string entryname = fmt::format(
//         "{}/{}.{}.protobin",
//         entry.topic,
//         entry.stamped_msg.timestamp().seconds(), 
//         entry.stamped_msg.timestamp().nanos());
//     return _archive->Write(entryname, *maybe_m_bytes.value);

//   }
// }

void WriteSession::Close() {
  if (_indexer) {
    BagIndex index = BagIndexBuilder::Complete(std::move(_indexer));
    WriteEntry(
      Entry::CreateStamped(
        "/_protobag_index/bag_index",
        ::google::protobuf::util::TimeUtil::GetCurrentTime(),
        index));
    _indexer = nullptr; // FIXME do we need this?  getting double index entries ....~~~~~~~~~~~~~~~~~~~~~~~~
  }
}


} /* namespace protobag */
