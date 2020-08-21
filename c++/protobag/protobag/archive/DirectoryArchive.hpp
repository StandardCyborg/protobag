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

#include "protobag/archive/Archive.hpp"

namespace protobag {
namespace archive {

// Archive Impl: A fake "archive" that is simply a directory on local disk
class DirectoryArchive final : public Archive {
public:
  static Result<Archive::Ptr> Open(Archive::Spec s);
  
  virtual std::vector<std::string> GetNamelist() override;
  virtual Archive::ReadStatus ReadAsStr(const std::string &entryname) override;

  virtual OkOrErr Write(
    const std::string &entryname, const std::string &data) override;

  virtual std::string ToString() const override { 
    return std::string("DirectoryArchive: ") + GetSpec().path;
  }
};

} /* namespace archive */
} /* namespace protobag */
