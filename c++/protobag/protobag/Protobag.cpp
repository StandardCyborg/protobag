#include "protobag/Protobag.hpp"

namespace protobag {

int foo() {

  return 1337;
}

BagIndex Protobag::GetIndex() const {
  auto maybe_index = ReadSession::GetIndex(path);
  if (!maybe_index.IsOk()) {
    return BagIndex();
  }
  return *maybe_index.value;
}

} /* namespace protobag */
