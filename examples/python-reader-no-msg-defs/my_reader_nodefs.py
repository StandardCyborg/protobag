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
      print(MessageToDict(entry.get_msg()))
    print()
    print()
  
  print()
  print("Decoder:")
  print(bag.decoder)
  print()
