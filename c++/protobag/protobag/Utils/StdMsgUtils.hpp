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
