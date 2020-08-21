# Copyright 2020 Standard Cyborg
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys

import protobag


if __name__ == '__main__':
  path = sys.argv[1]

  print("Using protobag library %s" % protobag.__file__)
  print("Reading bag %s" % path)

  bag = protobag.Protobag(path=path)
  for entry in bag.iter_entries():
    # ignore the index
    if '_protobag_index' in entry.entryname:
      continue
    print(entry.entryname)
    print(entry.type_url)
    if 'raw' in entry.entryname:
      print(entry)
    else:
      from google.protobuf.json_format import MessageToDict
      import pprint
      pprint.pprint(MessageToDict(entry.get_msg()))
    print()
    print()
  
  print()
  print("Decoder:")
  print(bag.decoder)
  print()
