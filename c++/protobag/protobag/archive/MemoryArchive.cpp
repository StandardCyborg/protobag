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

#include "protobag/archive/MemoryArchive.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace protobag {
namespace archive {

std::string CanonEntryname(const std::string &entryname) {
  // Trim leading path sep from `entryname`; matches DirectoryArchive
  std::string entry_path_rel = entryname;
  if (
    !entry_path_rel.empty() && entry_path_rel[0] ==
    std::filesystem::path::preferred_separator) {
    
    entry_path_rel = entry_path_rel.substr(1, entry_path_rel.size() - 1);
  }
  return entry_path_rel;
}

Result<Archive::Ptr> MemoryArchive::Open(Archive::Spec s) {
  MemoryArchive *ma = new MemoryArchive();
  Archive::Ptr pa(ma);
  return {.value = pa};
}
  
std::vector<std::string> MemoryArchive::GetNamelist() {
  std::vector<std::string> namelist;
  namelist.reserve(_archive_data.size());
  for (const auto &entry : _archive_data) {
    namelist.push_back(
      std::filesystem::path::preferred_separator + entry.first);
  }
  return namelist;
}

Archive::ReadStatus MemoryArchive::ReadAsStr(const std::string &entryname) {
  const std::string &canon_entryname = CanonEntryname(entryname);

  if (_archive_data.find(canon_entryname) == _archive_data.end()) {
    return Archive::ReadStatus::EntryNotFound();
  } else {
    return Archive::ReadStatus::OK(
      std::string(_archive_data[canon_entryname]));
  }
}

OkOrErr MemoryArchive::Write(
    const std::string &entryname, const std::string &data) {
  
  const std::string &canon_entryname = CanonEntryname(entryname);
  _archive_data[canon_entryname] = data;
  return kOK; 
}

std::string MemoryArchive::ToString() const {
  std::stringstream ss;
  ss << "MemoryArchive: (" << _archive_data.size() << ")" << std::endl;
  ss << "Entries:" << std::endl;

  std::vector<std::string> names;
  names.reserve(_archive_data.size());
  for (const auto &entry : _archive_data) {
    names.push_back(entry.first);
  }
  std::sort(names.begin(), names.end());
  for (const auto &name : names) {
    ss << name << std::endl;
  }

  return ss.str();
}


} /* namespace archive */
} /* namespace protobag */
