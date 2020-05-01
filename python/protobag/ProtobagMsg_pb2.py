# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: ProtobagMsg.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from google.protobuf import any_pb2 as google_dot_protobuf_dot_any__pb2
from google.protobuf import timestamp_pb2 as google_dot_protobuf_dot_timestamp__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='ProtobagMsg.proto',
  package='protobag',
  syntax='proto3',
  serialized_options=None,
  serialized_pb=b'\n\x11ProtobagMsg.proto\x12\x08protobag\x1a\x19google/protobuf/any.proto\x1a\x1fgoogle/protobuf/timestamp.proto\"b\n\x0eStampedMessage\x12-\n\ttimestamp\x18\x01 \x01(\x0b\x32\x1a.google.protobuf.Timestamp\x12!\n\x03msg\x18\x02 \x01(\x0b\x32\x14.google.protobuf.Any\"~\n\x06StdMsg\x1a\x15\n\x04\x42ool\x12\r\n\x05value\x18\x01 \x01(\x08\x1a\x14\n\x03Int\x12\r\n\x05value\x18\x01 \x01(\x03\x1a\x16\n\x05\x46loat\x12\r\n\x05value\x18\x01 \x01(\x02\x1a\x17\n\x06String\x12\r\n\x05value\x18\x01 \x01(\t\x1a\x16\n\x05\x42ytes\x12\r\n\x05value\x18\x01 \x01(\x0c\"\\\n\tTopicTime\x12\r\n\x05topic\x18\x01 \x01(\t\x12-\n\ttimestamp\x18\x02 \x01(\x0b\x32\x1a.google.protobuf.Timestamp\x12\x11\n\tentryname\x18\n \x01(\t\"\xa9\x02\n\tSelection\x12,\n\x06window\x18\x01 \x01(\x0b\x32\x1a.protobag.Selection.WindowH\x00\x12,\n\x06\x65vents\x18\x02 \x01(\x0b\x32\x1a.protobag.Selection.EventsH\x00\x1a\x84\x01\n\x06Window\x12\x0e\n\x06topics\x18\x01 \x03(\t\x12)\n\x05start\x18\x02 \x01(\x0b\x32\x1a.google.protobuf.Timestamp\x12\'\n\x03\x65nd\x18\x03 \x01(\x0b\x32\x1a.google.protobuf.Timestamp\x12\x16\n\x0e\x65xclude_topics\x18\x04 \x03(\t\x1a-\n\x06\x45vents\x12#\n\x06\x65vents\x18\n \x03(\x0b\x32\x13.protobag.TopicTimeB\n\n\x08\x63riteria\"\xf3\x02\n\x07\x42\x61gMeta\x12\x15\n\rbag_namespace\x18\x01 \x01(\t\x12)\n\x05start\x18\x02 \x01(\x0b\x32\x1a.google.protobuf.Timestamp\x12\'\n\x03\x65nd\x18\x03 \x01(\x0b\x32\x1a.google.protobuf.Timestamp\x12\x18\n\x10protobag_version\x18\n \x01(\t\x12;\n\x0etopic_to_stats\x18\x14 \x03(\x0b\x32#.protobag.BagMeta.TopicToStatsEntry\x12\x31\n\x14time_ordered_entries\x18\x1e \x03(\x0b\x32\x13.protobag.TopicTime\x1a \n\nTopicStats\x12\x12\n\nn_messages\x18\x01 \x01(\x03\x1aQ\n\x11TopicToStatsEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12+\n\x05value\x18\x02 \x01(\x0b\x32\x1c.protobag.BagMeta.TopicStats:\x02\x38\x01\x62\x06proto3'
  ,
  dependencies=[google_dot_protobuf_dot_any__pb2.DESCRIPTOR,google_dot_protobuf_dot_timestamp__pb2.DESCRIPTOR,])




_STAMPEDMESSAGE = _descriptor.Descriptor(
  name='StampedMessage',
  full_name='protobag.StampedMessage',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='timestamp', full_name='protobag.StampedMessage.timestamp', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='msg', full_name='protobag.StampedMessage.msg', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=91,
  serialized_end=189,
)


_STDMSG_BOOL = _descriptor.Descriptor(
  name='Bool',
  full_name='protobag.StdMsg.Bool',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='value', full_name='protobag.StdMsg.Bool.value', index=0,
      number=1, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=201,
  serialized_end=222,
)

_STDMSG_INT = _descriptor.Descriptor(
  name='Int',
  full_name='protobag.StdMsg.Int',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='value', full_name='protobag.StdMsg.Int.value', index=0,
      number=1, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=224,
  serialized_end=244,
)

_STDMSG_FLOAT = _descriptor.Descriptor(
  name='Float',
  full_name='protobag.StdMsg.Float',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='value', full_name='protobag.StdMsg.Float.value', index=0,
      number=1, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=246,
  serialized_end=268,
)

_STDMSG_STRING = _descriptor.Descriptor(
  name='String',
  full_name='protobag.StdMsg.String',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='value', full_name='protobag.StdMsg.String.value', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=270,
  serialized_end=293,
)

_STDMSG_BYTES = _descriptor.Descriptor(
  name='Bytes',
  full_name='protobag.StdMsg.Bytes',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='value', full_name='protobag.StdMsg.Bytes.value', index=0,
      number=1, type=12, cpp_type=9, label=1,
      has_default_value=False, default_value=b"",
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=295,
  serialized_end=317,
)

_STDMSG = _descriptor.Descriptor(
  name='StdMsg',
  full_name='protobag.StdMsg',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
  ],
  extensions=[
  ],
  nested_types=[_STDMSG_BOOL, _STDMSG_INT, _STDMSG_FLOAT, _STDMSG_STRING, _STDMSG_BYTES, ],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=191,
  serialized_end=317,
)


_TOPICTIME = _descriptor.Descriptor(
  name='TopicTime',
  full_name='protobag.TopicTime',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='topic', full_name='protobag.TopicTime.topic', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='timestamp', full_name='protobag.TopicTime.timestamp', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='entryname', full_name='protobag.TopicTime.entryname', index=2,
      number=10, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=319,
  serialized_end=411,
)


_SELECTION_WINDOW = _descriptor.Descriptor(
  name='Window',
  full_name='protobag.Selection.Window',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='topics', full_name='protobag.Selection.Window.topics', index=0,
      number=1, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='start', full_name='protobag.Selection.Window.start', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='end', full_name='protobag.Selection.Window.end', index=2,
      number=3, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='exclude_topics', full_name='protobag.Selection.Window.exclude_topics', index=3,
      number=4, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=520,
  serialized_end=652,
)

_SELECTION_EVENTS = _descriptor.Descriptor(
  name='Events',
  full_name='protobag.Selection.Events',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='events', full_name='protobag.Selection.Events.events', index=0,
      number=10, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=654,
  serialized_end=699,
)

_SELECTION = _descriptor.Descriptor(
  name='Selection',
  full_name='protobag.Selection',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='window', full_name='protobag.Selection.window', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='events', full_name='protobag.Selection.events', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[_SELECTION_WINDOW, _SELECTION_EVENTS, ],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
    _descriptor.OneofDescriptor(
      name='criteria', full_name='protobag.Selection.criteria',
      index=0, containing_type=None, fields=[]),
  ],
  serialized_start=414,
  serialized_end=711,
)


_BAGMETA_TOPICSTATS = _descriptor.Descriptor(
  name='TopicStats',
  full_name='protobag.BagMeta.TopicStats',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='n_messages', full_name='protobag.BagMeta.TopicStats.n_messages', index=0,
      number=1, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=970,
  serialized_end=1002,
)

_BAGMETA_TOPICTOSTATSENTRY = _descriptor.Descriptor(
  name='TopicToStatsEntry',
  full_name='protobag.BagMeta.TopicToStatsEntry',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='key', full_name='protobag.BagMeta.TopicToStatsEntry.key', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='value', full_name='protobag.BagMeta.TopicToStatsEntry.value', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=b'8\001',
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=1004,
  serialized_end=1085,
)

_BAGMETA = _descriptor.Descriptor(
  name='BagMeta',
  full_name='protobag.BagMeta',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='bag_namespace', full_name='protobag.BagMeta.bag_namespace', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='start', full_name='protobag.BagMeta.start', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='end', full_name='protobag.BagMeta.end', index=2,
      number=3, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='protobag_version', full_name='protobag.BagMeta.protobag_version', index=3,
      number=10, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='topic_to_stats', full_name='protobag.BagMeta.topic_to_stats', index=4,
      number=20, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='time_ordered_entries', full_name='protobag.BagMeta.time_ordered_entries', index=5,
      number=30, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[_BAGMETA_TOPICSTATS, _BAGMETA_TOPICTOSTATSENTRY, ],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=714,
  serialized_end=1085,
)

_STAMPEDMESSAGE.fields_by_name['timestamp'].message_type = google_dot_protobuf_dot_timestamp__pb2._TIMESTAMP
_STAMPEDMESSAGE.fields_by_name['msg'].message_type = google_dot_protobuf_dot_any__pb2._ANY
_STDMSG_BOOL.containing_type = _STDMSG
_STDMSG_INT.containing_type = _STDMSG
_STDMSG_FLOAT.containing_type = _STDMSG
_STDMSG_STRING.containing_type = _STDMSG
_STDMSG_BYTES.containing_type = _STDMSG
_TOPICTIME.fields_by_name['timestamp'].message_type = google_dot_protobuf_dot_timestamp__pb2._TIMESTAMP
_SELECTION_WINDOW.fields_by_name['start'].message_type = google_dot_protobuf_dot_timestamp__pb2._TIMESTAMP
_SELECTION_WINDOW.fields_by_name['end'].message_type = google_dot_protobuf_dot_timestamp__pb2._TIMESTAMP
_SELECTION_WINDOW.containing_type = _SELECTION
_SELECTION_EVENTS.fields_by_name['events'].message_type = _TOPICTIME
_SELECTION_EVENTS.containing_type = _SELECTION
_SELECTION.fields_by_name['window'].message_type = _SELECTION_WINDOW
_SELECTION.fields_by_name['events'].message_type = _SELECTION_EVENTS
_SELECTION.oneofs_by_name['criteria'].fields.append(
  _SELECTION.fields_by_name['window'])
_SELECTION.fields_by_name['window'].containing_oneof = _SELECTION.oneofs_by_name['criteria']
_SELECTION.oneofs_by_name['criteria'].fields.append(
  _SELECTION.fields_by_name['events'])
_SELECTION.fields_by_name['events'].containing_oneof = _SELECTION.oneofs_by_name['criteria']
_BAGMETA_TOPICSTATS.containing_type = _BAGMETA
_BAGMETA_TOPICTOSTATSENTRY.fields_by_name['value'].message_type = _BAGMETA_TOPICSTATS
_BAGMETA_TOPICTOSTATSENTRY.containing_type = _BAGMETA
_BAGMETA.fields_by_name['start'].message_type = google_dot_protobuf_dot_timestamp__pb2._TIMESTAMP
_BAGMETA.fields_by_name['end'].message_type = google_dot_protobuf_dot_timestamp__pb2._TIMESTAMP
_BAGMETA.fields_by_name['topic_to_stats'].message_type = _BAGMETA_TOPICTOSTATSENTRY
_BAGMETA.fields_by_name['time_ordered_entries'].message_type = _TOPICTIME
DESCRIPTOR.message_types_by_name['StampedMessage'] = _STAMPEDMESSAGE
DESCRIPTOR.message_types_by_name['StdMsg'] = _STDMSG
DESCRIPTOR.message_types_by_name['TopicTime'] = _TOPICTIME
DESCRIPTOR.message_types_by_name['Selection'] = _SELECTION
DESCRIPTOR.message_types_by_name['BagMeta'] = _BAGMETA
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

StampedMessage = _reflection.GeneratedProtocolMessageType('StampedMessage', (_message.Message,), {
  'DESCRIPTOR' : _STAMPEDMESSAGE,
  '__module__' : 'ProtobagMsg_pb2'
  # @@protoc_insertion_point(class_scope:protobag.StampedMessage)
  })
_sym_db.RegisterMessage(StampedMessage)

StdMsg = _reflection.GeneratedProtocolMessageType('StdMsg', (_message.Message,), {

  'Bool' : _reflection.GeneratedProtocolMessageType('Bool', (_message.Message,), {
    'DESCRIPTOR' : _STDMSG_BOOL,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.StdMsg.Bool)
    })
  ,

  'Int' : _reflection.GeneratedProtocolMessageType('Int', (_message.Message,), {
    'DESCRIPTOR' : _STDMSG_INT,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.StdMsg.Int)
    })
  ,

  'Float' : _reflection.GeneratedProtocolMessageType('Float', (_message.Message,), {
    'DESCRIPTOR' : _STDMSG_FLOAT,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.StdMsg.Float)
    })
  ,

  'String' : _reflection.GeneratedProtocolMessageType('String', (_message.Message,), {
    'DESCRIPTOR' : _STDMSG_STRING,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.StdMsg.String)
    })
  ,

  'Bytes' : _reflection.GeneratedProtocolMessageType('Bytes', (_message.Message,), {
    'DESCRIPTOR' : _STDMSG_BYTES,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.StdMsg.Bytes)
    })
  ,
  'DESCRIPTOR' : _STDMSG,
  '__module__' : 'ProtobagMsg_pb2'
  # @@protoc_insertion_point(class_scope:protobag.StdMsg)
  })
_sym_db.RegisterMessage(StdMsg)
_sym_db.RegisterMessage(StdMsg.Bool)
_sym_db.RegisterMessage(StdMsg.Int)
_sym_db.RegisterMessage(StdMsg.Float)
_sym_db.RegisterMessage(StdMsg.String)
_sym_db.RegisterMessage(StdMsg.Bytes)

TopicTime = _reflection.GeneratedProtocolMessageType('TopicTime', (_message.Message,), {
  'DESCRIPTOR' : _TOPICTIME,
  '__module__' : 'ProtobagMsg_pb2'
  # @@protoc_insertion_point(class_scope:protobag.TopicTime)
  })
_sym_db.RegisterMessage(TopicTime)

Selection = _reflection.GeneratedProtocolMessageType('Selection', (_message.Message,), {

  'Window' : _reflection.GeneratedProtocolMessageType('Window', (_message.Message,), {
    'DESCRIPTOR' : _SELECTION_WINDOW,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.Selection.Window)
    })
  ,

  'Events' : _reflection.GeneratedProtocolMessageType('Events', (_message.Message,), {
    'DESCRIPTOR' : _SELECTION_EVENTS,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.Selection.Events)
    })
  ,
  'DESCRIPTOR' : _SELECTION,
  '__module__' : 'ProtobagMsg_pb2'
  # @@protoc_insertion_point(class_scope:protobag.Selection)
  })
_sym_db.RegisterMessage(Selection)
_sym_db.RegisterMessage(Selection.Window)
_sym_db.RegisterMessage(Selection.Events)

BagMeta = _reflection.GeneratedProtocolMessageType('BagMeta', (_message.Message,), {

  'TopicStats' : _reflection.GeneratedProtocolMessageType('TopicStats', (_message.Message,), {
    'DESCRIPTOR' : _BAGMETA_TOPICSTATS,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.BagMeta.TopicStats)
    })
  ,

  'TopicToStatsEntry' : _reflection.GeneratedProtocolMessageType('TopicToStatsEntry', (_message.Message,), {
    'DESCRIPTOR' : _BAGMETA_TOPICTOSTATSENTRY,
    '__module__' : 'ProtobagMsg_pb2'
    # @@protoc_insertion_point(class_scope:protobag.BagMeta.TopicToStatsEntry)
    })
  ,
  'DESCRIPTOR' : _BAGMETA,
  '__module__' : 'ProtobagMsg_pb2'
  # @@protoc_insertion_point(class_scope:protobag.BagMeta)
  })
_sym_db.RegisterMessage(BagMeta)
_sym_db.RegisterMessage(BagMeta.TopicStats)
_sym_db.RegisterMessage(BagMeta.TopicToStatsEntry)


_BAGMETA_TOPICTOSTATSENTRY._options = None
# @@protoc_insertion_point(module_scope)