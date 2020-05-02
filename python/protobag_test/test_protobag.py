
from protobag import Protobag

def test():
  p = Protobag()




from protobag import ProtoBag
from protobag import Entry

from protobag.ProtobagMsg_pb2 import StdMsg

def to_std_msg(v):
  if isinstance(v, bool):
    return StdMsg.Bool(value=v)
  elif isinstance(v, int):
    return StdMsg.Int(value=v)
  elif isinstance(v, float):
    return StdMsg.Float(value=v)
  elif isinstance(v, str):
    # TODO python2 support ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # NB: deprecation warning fixed in pb 3.3 https://github.com/protocolbuffers/protobuf/pull/2922
    return StdMsg.String(value=v)
  elif isinstance(v, (bytes, bytearray)):
    return StdMsg.Bytes(value=v)
  else:
    raise TypeError(v)


def to_unixtime(dt):
  import time
  return int(time.mktime(dt.timetuple()))



def test_demo():
  import tempfile

  f = tempfile.NamedTemporaryFile(suffix='.zip')
  print('temp file', f.name)
  b = ProtoBag.write_zip(f)

  ENTRIES = (
    Entry(
      topic='/topic1',
      timestamp=0,
      msg=to_std_msg("foo")),
    Entry(
      topic='/topic1',
      timestamp=1,
      msg=to_std_msg("bar")),
    Entry(
      topic='/topic2',
      timestamp=0,
      msg=to_std_msg(1337)),
  )

  for entry in ENTRIES:
    b.write_entry(entry)
  
  # Flush / close the zip
  del b

  b = ProtoBag.read_zip(f)  
  expected = sorted(
    (entry.topic, entry.timestamp, entry.msg.value)
    for entry in ENTRIES)
  actual = sorted(
    (entry.topic, to_unixtime(entry.timestamp), entry.msg.value)
    for entry in b.iter_entries())
  
  assert expected == actual
