import copy
import itertools

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


def test_to_sec_nanos():
  import datetime
  from google.protobuf.timestamp_pb2 import Timestamp

  assert protobag.to_sec_nanos((1337, 1337)) == (1337, 1337)

  assert \
    protobag.to_sec_nanos(Timestamp(seconds=1337, nanos=1337)) == (1337, 1337)

  assert protobag.to_sec_nanos(1337) == (1337, 0)
  assert protobag.to_sec_nanos(1337.1337) == (1337, 133700000)

  assert \
    protobag.to_sec_nanos(
      datetime.datetime(
      year=1970, month=1, day=1, second=13, microsecond=37)) \
        == (13, 37000)
    
  with pytest.raises(ValueError):
    protobag.to_sec_nanos("123")

  with pytest.raises(ValueError):
    protobag.to_sec_nanos((1, 2, 3))


def test_to_topic_time():
  from google.protobuf.timestamp_pb2 import Timestamp
  from protobag.ProtobagMsg_pb2 import TopicTime

  assert protobag.to_topic_time(TopicTime()) == TopicTime()

  assert \
    protobag.to_topic_time({'topic': 't', 'timestamp': 1.1}) == \
      TopicTime(topic='t', timestamp=Timestamp(seconds=1, nanos=100000000))
  
  assert \
    protobag.to_topic_time(('t', 1.1)) == \
      TopicTime(topic='t', timestamp=Timestamp(seconds=1, nanos=100000000))
  
  with pytest.raises(ValueError):
    protobag.to_topic_time("foo")


def test_build_fds_for_msg():
  from protobag.ProtobagMsg_pb2 import TopicTime
  fds = protobag.build_fds_for_msg(TopicTime)
  actual_types = sorted(list(itertools.chain.from_iterable(
    (f.package + '.' + t.name for t in f.message_type)
    for f in fds.file)))
  
  expected_types = (
    # These types get pulled in by `build_fds_for_msg()` naively grabbing
    # everything else in the package that contains the target type TopicTime
    'protobag.BagIndex',
    'protobag.Selection',
    'protobag.StampedMessage',
    'protobag.StdMsg',
    'protobag.TopicTime',
    
    # These protobuf types get pulled in by `build_fds_for_msg()` 
    # looking for transitive dependencies.  So for example, if google's 
    # Timestamp message changes, then our snapshot of TopicTime will
    # capture the version of Timestamp used when the TopicTime message was
    # recorded
    'google.protobuf.Any',
    'google.protobuf.DescriptorProto',
    'google.protobuf.EnumDescriptorProto',
    'google.protobuf.EnumOptions',
    'google.protobuf.EnumValueDescriptorProto',
    'google.protobuf.EnumValueOptions',
    'google.protobuf.ExtensionRangeOptions',
    'google.protobuf.FieldDescriptorProto',
    'google.protobuf.FieldOptions',
    'google.protobuf.FileDescriptorProto',
    'google.protobuf.FileDescriptorSet',
    'google.protobuf.FileOptions',
    'google.protobuf.GeneratedCodeInfo',
    'google.protobuf.MessageOptions',
    'google.protobuf.MethodDescriptorProto',
    'google.protobuf.MethodOptions',
    'google.protobuf.OneofDescriptorProto',
    'google.protobuf.OneofOptions',
    'google.protobuf.ServiceDescriptorProto',
    'google.protobuf.ServiceOptions',
    'google.protobuf.SourceCodeInfo',
    'google.protobuf.Timestamp',
    'google.protobuf.UninterpretedOption',
  )
  assert actual_types == sorted(expected_types)



## ============================================================================
## == Tet Public API ==========================================================
## ============================================================================

def test_msg_entry_print():
  entry = protobag.MessageEntry.from_msg(
            entryname='my_entry',
            msg=to_std_msg('foo'))
  assert str(entry) == (
    "MessageEntry:\n"
    "  entryname: my_entry\n"
    "  type_url: type.googleapis.com/protobag.StdMsg.String\n"
    "  has serdes: False\n"
    "  has descriptor_data: False\n"
    "  msg:\n"
    "value: \"foo\"\n")


def test_raw_entry_print():
  entry = protobag.RawEntry.from_bytes(
            entryname='my_entry',
            raw_bytes=bytearray(b'abcabcabcabcabcabcabcabcabcabc'))
  assert str(entry) == (
    "RawEntry:\n"
    "  entryname: my_entry\n"
    "  raw_bytes: abcabcabcabcabcabcab ... (30 bytes)")


def test_stamped_entry_print():
  entry = protobag.StampedEntry.from_msg(
            topic='my_topic',
            timestamp=(1337, 1337),
            msg=to_std_msg('foo'))
  assert str(entry) == (
    "StampedEntry:\n"
    "  topic: my_topic\n"
    "  timestamp: 1337 sec  1337 ns\n"
    "  type_url: type.googleapis.com/protobag.StdMsg.String\n"
    "  entryname: \n"
    "  has serdes: False\n"
    "  has descriptor_data: False\n"
    "  msg:\n"
    "value: \"foo\"\n")



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


_to_typed_bytes = protobag.PBSerdes.msg_to_typed_bytes

def test_serdes_msg_from_typed_bytes_empty():
  tb = _to_typed_bytes(to_std_msg('moof'))
  serdes = protobag.PBSerdes()
  with pytest.raises(ValueError):
    msg = serdes.msg_from_typed_bytes(tb)


def test_serdes_msg_from_typed_bytes_default_serdes():
  tb = _to_typed_bytes(to_std_msg('moof'))
  serdes = copy.deepcopy(protobag.DEFAULT_SERDES)
    # The DEFAULT_SERDES has built-in support for protobag standard messages
  msg = serdes.msg_from_typed_bytes(tb)
  assert msg.value == 'moof'


def test_serdes_msg_from_typed_bytes_user_registered():
  tb = _to_typed_bytes(to_std_msg('moof'))

  from protobag.ProtobagMsg_pb2 import StdMsg
  serdes = protobag.PBSerdes.create_with_types([StdMsg.String])
  
  msg = serdes.msg_from_typed_bytes(tb)
  assert msg.value == 'moof'


def test_serdes_msg_from_typed_bytes_dynamic_decode():
  tb = _to_typed_bytes(to_std_msg('moof'))
  
  serdes = protobag.PBSerdes()
  
  from protobag.ProtobagMsg_pb2 import StdMsg
  fds = protobag.build_fds_for_msg(StdMsg.String)
  descriptor_data = fds.SerializeToString()
  serdes.register_descriptor_data(tb.type_url, descriptor_data)

  msg = serdes.msg_from_typed_bytes(tb)
  assert msg.value == 'moof'



## ============================================================================
## == Test Public API =========================================================
## ============================================================================

def test_write_msg():
  pass


def test_write_stamped_msg():
  pass


def test_write_raw():
  pass


def test_read_msg():
  pass


def test_read_stamped_msg():
  pass


def test_read_raw():
  pass


def test_write_read_full():
  pass


def test_read_stamped_msg_max_slop_sync():
  pass


## ============================================================================
## == Test DictRowEntry =======================================================
## ============================================================================

def test_entry_to_dict_row():
  pass


def test_dict_row_to_entry():
  pass

