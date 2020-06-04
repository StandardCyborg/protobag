#pragma once

#include <string>

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {

inline StdMsg_Bool ToBoolMsg(bool v) {
  StdMsg_Bool m;
  m.set_value(v);
  return m;
}

inline StdMsg_Int ToIntMsg(int v) {
  StdMsg_Int m;
  m.set_value(v);
  return m;
}

inline StdMsg_Float ToFloatMsg(int v) {
  StdMsg_Float m;
  m.set_value(v);
  return m;
}

inline StdMsg_String ToStringMsg(std::string s) {
  StdMsg_String m;
  m.set_value(s);
  return m;
}

inline StdMsg_Bytes ToBytesMsg(std::string s) {
  StdMsg_Bytes m;
  m.set_value(s);
  return m;
}

} /* namespace protobag */
