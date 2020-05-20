#include "protobag/Entry.hpp"

#include <sstream>

#include <google/protobuf/util/time_util.h>

namespace protobag {

std::string Entry::ToString() const {
  std::stringstream ss;

  ss << 
    "Entry: " << entryname << std::endl <<
    "type_url: " << msg.type_url() << std::endl <<
    "msg: " << msg.value().size() << " bytes" << std::endl;
  
  if (ctx.has_value()) {
    ss <<
      "topic: " << ctx->topic << std::endl <<
      "time: " << ctx->stamp << std::endl <<
      "descriptor: " << 
        (ctx->descriptor ? 
          ctx->descriptor->full_name() : "(unavailable)")
      << std::endl;
  }

  return ss.str();
}

} // namespace protobag