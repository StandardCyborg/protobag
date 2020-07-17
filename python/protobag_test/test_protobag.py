import pytest

import protobag


def to_std_msg(v):
  from protobag.ProtobagMsg_pb2 import StdMsg
  if isinstance(v, bool):
    return StdMsg.Bool(value=v)
  elif isinstance(v, int):
    return StdMsg.Int(value=v)
  elif isinstance(v, float):
    return StdMsg.Float(value=v)
  elif isinstance(v, str):
    # TODO python2 support? NB: deprecation warning fixed in protobuf 3.3 
    # https://github.com/protocolbuffers/protobuf/pull/2922
    return StdMsg.String(value=v)
  elif isinstance(v, (bytes, bytearray)):
    return StdMsg.Bytes(value=v)
  else:
    raise TypeError(v)


## ============================================================================
## == Test Protobag Utils =====================================================
## ============================================================================

def test_type_url():
  msg = to_std_msg("foo")
  assert protobag.get_type_url(msg) == \
      'type.googleapis.com/protobag.StdMsg.String'

  msg = to_std_msg(1337)
  assert protobag.get_type_url(msg) == \
      'type.googleapis.com/protobag.StdMsg.Int'

  with pytest.raises(Exception):
    protobag.get_type_url("junk")


def test_to_pb_timestamp():
  import datetime
  from google.protobuf.timestamp_pb2 import Timestamp
  
  expected = Timestamp(seconds=1337, nanos=1337)
  assert protobag.to_pb_timestamp(expected) == expected

  assert protobag.to_pb_timestamp(1337) == Timestamp(seconds=1337)
  assert protobag.to_pb_timestamp(1337.1337) == \
    Timestamp(seconds=1337, nanos=133700000)
  assert protobag.to_pb_timestamp((1337, 1337)) == expected

  assert protobag.to_pb_timestamp(
    datetime.datetime(
      year=1970, month=1, day=1, second=13, microsecond=37)) == \
      Timestamp(seconds=13, nanos=37000)

  with pytest.raises(ValueError):
    protobag.to_pb_timestamp("123")



## ============================================================================
## == Test SERDES =============================================================
## ============================================================================

def test_typed_bytes():
  t = protobag.TypedBytes(type_url='type_url', entryname='entryname')
  assert str(t) == (
    "TypedBytes:\n"
    "  type_url: type_url\n"
    "  entryname: entryname\n"
    "  msg_bytes: None ... (0 bytes)")

  t = protobag.TypedBytes(
        type_url='type_url',
        entryname='entryname',
        msg_bytes=bytearray(b'abcabcabcabcabcabcabcabcabcabc'))
  assert str(t) == (
    "TypedBytes:\n"
    "  type_url: type_url\n"
    "  entryname: entryname\n"
    "  msg_bytes: abcabcabcabcabcabcab ... (30 bytes)")




# def test():
#   p = Protobag()

# from protobag import ProtoBag
# from protobag import Entry




# def to_unixtime(dt):
#   import time
#   return int(time.mktime(dt.timetuple()))



# def test_demo():
#   import tempfile

#   f = tempfile.NamedTemporaryFile(suffix='.zip')
#   print('temp file', f.name)
#   b = ProtoBag.write_zip(f)

#   ENTRIES = (
#     Entry(
#       topic='/topic1',
#       timestamp=0,
#       msg=to_std_msg("foo")),
#     Entry(
#       topic='/topic1',
#       timestamp=1,
#       msg=to_std_msg("bar")),
#     Entry(
#       topic='/topic2',
#       timestamp=0,
#       msg=to_std_msg(1337)),
#   )

#   for entry in ENTRIES:
#     b.write_entry(entry)
  
#   # Flush / close the zip
#   del b

#   b = ProtoBag.read_zip(f)  
#   expected = sorted(
#     (entry.topic, entry.timestamp, entry.msg.value)
#     for entry in ENTRIES)
#   actual = sorted(
#     (entry.topic, to_unixtime(entry.timestamp), entry.msg.value)
#     for entry in b.iter_entries())
  
#   assert expected == actual
