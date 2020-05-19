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

OkOrErr WriteSession::WriteEntry(const Entry &entry) {
  if (!_archive) {
    return OkOrErr::Err("Programming Error: no archive open for writing");
  }

  auto maybe_m_bytes = PBFactory::ToBinaryString<StampedMessage>(entry.stamped_msg);
  if (!maybe_m_bytes.IsOk()) {
    return OkOrErr::Err(maybe_m_bytes.error);
  }

  if (!IsProtoBagIndexTopic(entry.topic)) {
    if (!_indexer) {
      return OkOrErr::Err("Programming Error: no indexer (at least for file counter)");
    }
    auto next_filenum = _indexer->GetNextFilenum(entry.topic);
    std::string entryname = fmt::format(
      "{}/{}.protobin", entry.topic, next_filenum);

    OkOrErr res = _archive->Write(entryname, *maybe_m_bytes.value);
    if (res.IsOk() && _indexer) {
      _indexer->Observe(entry, entryname);
    }

    return res;

  } else {

    // The `/_protobag_index` topic gets no indexing
    std::string entryname = fmt::format(
        "{}/{}.{}.protobin",
        entry.topic,
        entry.stamped_msg.timestamp().seconds(), 
        entry.stamped_msg.timestamp().nanos());
    return _archive->Write(entryname, *maybe_m_bytes.value);

  }
}

void WriteSession::Close() {
  if (_indexer) {
    BagIndex index = BagIndexBuilder::Complete(std::move(_indexer));
    WriteEntry(
      Entry::Create(
        "/_protobag_index/bag_index",
        ::google::protobuf::util::TimeUtil::GetCurrentTime(),
        index));
    _indexer = nullptr; // FIXME do we need this?  getting double index entries ....
  }
}


} /* namespace protobag */
