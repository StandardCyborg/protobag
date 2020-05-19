#include "protobag/Protobag.hpp"

namespace protobag {

int foo() {

  return 1337;
}

BagMeta Protobag::GetIndex() const {
  auto maybe_index = ReadSession::GetIndex(path);
  if (!maybe_index.IsOk()) {
    return BagMeta();
  }
  return *maybe_index.value;
}

} /* namespace protobag */
