#include "protobag/Protobag.hpp"

#include <archive.h>
#include <archive_entry.h>

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

int foo() {

  struct archive *a;
  a = archive_write_new();
  archive_write_close(a);
  archive_write_free(a);
	// struct archive_entry *entry;

  std::string s = "yay";
  return 1337;
}

} /* namespace protobag */
