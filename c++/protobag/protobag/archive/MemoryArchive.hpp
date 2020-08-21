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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "protobag/archive/Archive.hpp"

namespace protobag {
namespace archive {

// Archive Impl: A fake "archive" that is simple stores all data in-memory
// (and never touches the disk!).  Useful for tests.  Not yet suitable for 
// memristor / NVME storage, but maybe one day!
class MemoryArchive final : public Archive {
public:
  
  // Archive Interface impl

  static Result<Archive::Ptr> Open(Archive::Spec s);
  
  virtual std::vector<std::string> GetNamelist() override;
  
  virtual Archive::ReadStatus ReadAsStr(const std::string &entryname) override;

  virtual OkOrErr Write(
    const std::string &entryname, const std::string &data) override;

  virtual std::string ToString() const override;

  // Convenience Utils

  static std::shared_ptr<MemoryArchive> Create(
      const std::unordered_map<std::string, std::string> &archive_data={}) {
    
    std::shared_ptr<MemoryArchive> ma(new MemoryArchive());
    for (const auto &entry : archive_data) {
      ma->Write(entry.first, entry.second);
    }
    return ma;
  }

  const std::unordered_map<std::string, std::string> GetData() const {
    return _archive_data;
  }

protected:
  std::unordered_map<std::string, std::string> _archive_data;
};

} /* namespace archive */
} /* namespace protobag */
