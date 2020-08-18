import copy
import itertools
import os

import pytest

import protobag


###
### Test Utils
###

def mkdir(path):
  import errno
  try:
    os.makedirs(path)
  except OSError as exc:
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else:
      raise


def rm_rf(path):
  import shutil
  shutil.rmtree(path)


def get_test_tempdir(testname, clean=True):
  import tempfile
  path = os.path.join(tempfile.gettempdir(), testname)

  if clean:
    mkdir(path)
    rm_rf(path)
    mkdir(path)
  
  return path


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

def _check_zip_has_expected_files(path, expected_files):
  import zipfile
  f = zipfile.ZipFile(path, 'r')
  actual = set(f.namelist())
  expected = set(expected_files)
  missing = (expected - actual)
  assert not missing, "Expected\n%s\nActual\n%s" % (expected, actual)


def test_write_read_msg():
  test_root = get_test_tempdir('test_write_read_msg')
  path = os.path.join(test_root, 'bag.zip')

  bag = protobag.Protobag(path=path)
  writer = bag.create_writer()
  writer.write_msg('txt_foo', to_std_msg("foo"))
  writer.write_msg('int_1337', to_std_msg(1337))
  writer.close()

  # Test zip archive contents using protobuf-blind zipfile
  _check_zip_has_expected_files(path, ('int_1337', 'txt_foo'))
  
  # Read messages back
  bag = protobag.Protobag(path=path)
  with pytest.raises(KeyError):
    entry = bag.get_entry('does_not_exist')
  
  entry = bag.get_entry('txt_foo')
  assert entry.entryname == 'txt_foo'
  assert entry.msg == to_std_msg("foo")

  entry = bag.get_entry('int_1337')
  assert entry.entryname == 'int_1337'
  assert entry.msg == to_std_msg(1337)


def test_write_read_stamped_msg():
  test_root = get_test_tempdir('test_write_read_stamped_msg')
  path = os.path.join(test_root, 'bag.zip')

  bag = protobag.Protobag(path=path)
  writer = bag.create_writer()
  for t in range(3):
    writer.write_stamped_msg("my_t1", to_std_msg(t), t_sec=t)
    writer.write_stamped_msg("my_t2", to_std_msg(t+1), t_sec=t+1)
  writer.close()

  # Test zip archive contents using protobuf-blind zipfile
  _check_zip_has_expected_files(
    path,
    (
      'my_t1/0.0.stampedmsg.protobin',
      'my_t1/1.0.stampedmsg.protobin',
      'my_t1/2.0.stampedmsg.protobin',
      
      'my_t2/1.0.stampedmsg.protobin',
      'my_t2/2.0.stampedmsg.protobin',
      'my_t2/3.0.stampedmsg.protobin',
    ))

  ## Read messages back
  bag = protobag.Protobag(path=path)
  with pytest.raises(KeyError):
    entry = bag.get_entry('does_not_exist')
  
  # Test getting topic list
  assert sorted(bag.get_topics()) == sorted(['my_t1', 'my_t2'])

  # Test getting time series data
  def _check_expected_topic_t_value(sel, expected_topic_t_value):
    actual_topic_t_value = []
    for entry in bag.iter_entries(selection=sel):
      actual_topic_t_value.append(
        (entry.topic, entry.timestamp.seconds, entry.msg.value))
    assert expected_topic_t_value == actual_topic_t_value

  _check_expected_topic_t_value(
    protobag.SelectionBuilder.select_window_all(),
    [
      ('my_t1', 0, 0), ('my_t1', 1, 1), ('my_t2', 1, 1), ('my_t1', 2, 2),
      ('my_t2', 2, 2), ('my_t2', 3, 3)
    ])
  _check_expected_topic_t_value(
    protobag.SelectionBuilder.select_window(topics=['my_t1']),
    [
      ('my_t1', 0, 0), ('my_t1', 1, 1), ('my_t1', 2, 2),
    ])
  _check_expected_topic_t_value(
    protobag.SelectionBuilder.select_window(topics=['my_t2']),
    [
      ('my_t2', 1, 1), ('my_t2', 2, 2), ('my_t2', 3, 3)
    ])
  _check_expected_topic_t_value(
    protobag.SelectionBuilder.select_window(topics=['does_not_exist']),
    [])
  
  # Test sync
  from protobag.protobag_native import MaxSlopTimeSyncSpec
  spec = MaxSlopTimeSyncSpec()
  spec.topics = ['my_t1', 'my_t2']
  spec.set_max_slop(seconds=2, nanos=0)

  actual_bundles = []
  sel = protobag.SelectionBuilder.select_window_all()
  for bundle in bag.iter_entries(selection=sel, sync_using_max_slop=spec):
    actual_bundles.append(sorted(
      (entry.topic, entry.timestamp.seconds, entry.msg.value)
      for entry in bundle
    ))

  expected_bundles = [
    [('my_t1', 1, 1), ('my_t2', 1, 1)],
    [('my_t1', 2, 2), ('my_t2', 2, 2)],
  ]
  assert actual_bundles == expected_bundles


def test_write_read_raw():
  test_root = get_test_tempdir('test_write_read_raw')
  path = os.path.join(test_root, 'bag.zip')

  bag = protobag.Protobag(path=path)
  writer = bag.create_writer()
  writer.write_raw('raw_data', b"i am a raw string")
  writer.close()

  # Test zip archive contents using protobuf-blind zipfile
  _check_zip_has_expected_files(path, ('raw_data',))
  
  # Read messages back
  bag = protobag.Protobag(path=path)
  with pytest.raises(KeyError):
    entry = bag.get_entry('does_not_exist')
  
  entry = bag.get_entry("raw_data")
  assert entry.entryname == 'raw_data'
  assert entry.raw_bytes == b"i am a raw string"


## ============================================================================
## == Test DictRowEntry =======================================================
## ============================================================================

def test_dict_row_entry_round_trip():
  ## First create a fixture; need a backing protobag to create entries
  test_root = get_test_tempdir('test_dict_row_entry_round_trip')
  path = os.path.join(test_root, 'bag.zip')

  bag = protobag.Protobag(path=path)
  writer = bag.create_writer()
  writer.write_msg('txt_foo', to_std_msg("foo"))
  for t in range(3):
    writer.write_stamped_msg("my_t1", to_std_msg(str(t)), t_sec=t)
  writer.write_raw('raw_data', b"i am a raw string")
  writer.close()


  ## Now read entries and test round trip entry -> dict -> entry
  path_rewrite = os.path.join(test_root, 'bag_rewrite.zip')

  bag = protobag.Protobag(path=path)
  bag_rewrite = protobag.Protobag(path=path_rewrite)
  writer = bag_rewrite.create_writer()

  # Simple messages
  entry = bag.get_entry("txt_foo")
  row = protobag.DictRowEntry.from_entry(entry)
  assert row.entryname == "txt_foo"
  assert row.type_url == 'type.googleapis.com/protobag.StdMsg.String'
  assert row.msg_dict == {'value': 'foo'}
  writer.write_entry(row.to_entry())

  # Time-series data
  sel = protobag.SelectionBuilder.select_window(topics=['my_t1'])
  entry = None
  for e in bag.iter_entries(selection=sel):
    entry = e
    break
  row = protobag.DictRowEntry.from_entry(entry)
  assert row.type_url == 'type.googleapis.com/protobag.StdMsg.String'
  assert row.msg_dict == {'value': '0'}
  assert row.topic == 'my_t1'
  assert row.timestamp == protobag.to_pb_timestamp(0)
  writer.write_entry(row.to_entry())

  # Raw data
  entry = bag.get_entry("raw_data")
  row = protobag.DictRowEntry.from_entry(entry)
  assert row.entryname == "raw_data"
  assert row.type_url == ''
  assert row.msg_dict == {'protobag_raw_entry_bytes': b"i am a raw string"}
  writer.write_entry(row.to_entry())

  writer.close()
