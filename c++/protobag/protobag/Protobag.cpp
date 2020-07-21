#include "protobag/Protobag.hpp"

namespace protobag {

BagIndex Protobag::GetIndex() const {
  auto maybe_index = ReadSession::GetIndex(path);
  if (!maybe_index.IsOk()) {
    return BagIndex();
  }
  return *maybe_index.value;
}

} /* namespace protobag */
