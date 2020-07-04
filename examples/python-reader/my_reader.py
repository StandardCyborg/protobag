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
    print(entry)
    print()
    print()
