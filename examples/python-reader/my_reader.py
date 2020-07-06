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
