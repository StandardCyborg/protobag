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

from MyMessages_pb2 import DinoHunter
from MyMessages_pb2 import Position

if __name__ == '__main__':
  path = sys.argv[1]

  print("Using protobag library %s" % protobag.__file__)
  print("Reading bag %s" % path)

  bag = protobag.Protobag(
          path=path,
          msg_classes=(
            DinoHunter,
            Position))
  for entry in bag.iter_entries():
    # ignore the index
    if '_protobag_index' in entry.entryname:
      continue
    
    print(entry)
    print("Message contents:")
    print(entry.get_msg())
    print()
    print()
