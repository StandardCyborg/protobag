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
  assert protobag.get_type_url(msg) == 'type.googleapis.com/string'

  msg = to_std_msg(1337)
  assert protobag.get_type_url(msg) == 'type.googleapis.com/string'

  with pytest.assert_raises(Exception):
    protobag.get_type_url("junk")


def test_to_pb_timestamp():
  import datetime
  from google.protobuf.timestamp_pb2 import Timestamp
  
  assert protobag.to_pb_timestamp(Timestamp(1337, 0)) == Timestamp(1337, 1337)

  assert protobag.to_pb_timestamp(1337) == Timestamp(1337, 0)
  assert protobag.to_pb_timestamp(1337.1337) == Timestamp(1337, 1337)
  assert protobag.to_pb_timestamp((1337, 1337)) == Timestamp(1337, 1337)

  assert protobag.to_pb_timestamp(
    datetime.datetime(second=1337, microsecond=1337)) == \
      Timestamp(1337, 1337000)

  with pytest.assert_raises(ValueError):
    protobag.to_pb_timestamp("123")



## ============================================================================
## == Test SERDES =============================================================
## ============================================================================

def test_typed_bytes():
  t = TypedBytes(type_url='type_url', entryname='entryname')
  assert str(t) == """
    TypedBytes:
      type_url: type_url
      entryname: entryname
      msg_bytes: None ... (0 bytes)
    """

  t = TypedBytes(
        type_url='type_url',
        entryname='entryname',
        msg_bytes=bytearray('abc'))
  assert str(t) == """
    TypedBytes:
      type_url: type_url
      entryname: entryname
      msg_bytes: abc ... (3 bytes)
    """




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
