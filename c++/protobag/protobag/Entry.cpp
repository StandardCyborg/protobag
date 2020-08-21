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

#include "protobag/Entry.hpp"

#include <filesystem>
#include <sstream>

#include <google/protobuf/util/time_util.h>

#include "protobag/archive/Archive.hpp"

namespace protobag {

std::string Entry::ToString() const {
  std::stringstream ss;

  ss << 
    "Entry: " << entryname << std::endl <<
    "type_url: " << msg.type_url() << std::endl <<
    "msg: (" << msg.value().size() << " bytes)" << std::endl;
  
  if (ctx.has_value()) {
    ss <<
      "topic: " << ctx->topic << std::endl <<
      "time: " << ctx->stamp << std::endl <<
      "descriptor: " << 
        (ctx->descriptor ? 
          ctx->descriptor->full_name() : "(unavailable)")
      << std::endl;
  } else if (IsStampedMessage()) {
    auto maybe_stamped = GetAs<StampedMessage>();
    if (maybe_stamped.IsOk()) {
      const StampedMessage &stamped_msg = *maybe_stamped.value;
      ss <<
        "time: " << stamped_msg.timestamp() << std::endl <<
        "inner_type_url: " << stamped_msg.msg().type_url() << std::endl <<
        "stamped_msg: (" << stamped_msg.msg().value().size() << " bytes)" 
          << std::endl;
    }
  }

  return ss.str();
}

// bool Entry::operator==(const Entry &other) const {
//   return 
//     entryname == other.entryname &&
//     msg == other.msg && 
//     ctx.has_value() == other.ctx.has_value() &&
//     (!ctx.has_value() || (
//       ctx.topic == other.ctx->topic &&
//       ctx.stamp == other.ctx->stamp &&
//       ctx.inner_type_url == other.ctx.inner_type_url &&
//       ctx.descriptor == other.ctx.descriptor
//     ));
// }

bool MaybeEntry::IsNotFound() const {
  static const std::string kIsNotFoundPrefix = 
    archive::Archive::ReadStatus::EntryNotFound().error + ": ";
  return error.find(kIsNotFoundPrefix) == 0;
}

MaybeEntry MaybeEntry::NotFound(const std::string &entryname) {
  MaybeEntry m;
  m.error = fmt::format(
    "{}: {}",
    archive::Archive::ReadStatus::EntryNotFound().error,
    entryname);
  return m;
}

std::string GetTopicFromEntryname(const std::string &entryname) {
  return std::filesystem::path(entryname).parent_path().u8string();
}

} // namespace protobag