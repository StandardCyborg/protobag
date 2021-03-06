# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: MyMessages.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='MyMessages.proto',
  package='my_messages',
  syntax='proto3',
  serialized_options=None,
  serialized_pb=b'\n\x10MyMessages.proto\x12\x0bmy_messages\"\xd7\x02\n\nDinoHunter\x12\x12\n\nfirst_name\x18\x01 \x01(\t\x12\n\n\x02id\x18\x02 \x01(\x05\x12\x35\n\x07\x61ttribs\x18\x03 \x03(\x0b\x32$.my_messages.DinoHunter.AttribsEntry\x12+\n\x05\x64inos\x18\x04 \x03(\x0b\x32\x1c.my_messages.DinoHunter.Dino\x1a.\n\x0c\x41ttribsEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12\r\n\x05value\x18\x02 \x01(\t:\x02\x38\x01\x1a\x44\n\x04\x44ino\x12\x0c\n\x04name\x18\x01 \x01(\t\x12.\n\x04type\x18\x02 \x01(\x0e\x32 .my_messages.DinoHunter.DinoType\"O\n\x08\x44inoType\x12\x07\n\x03IDK\x10\x00\x12\x10\n\x0cVEGGIESAURUS\x10\x01\x12\x10\n\x0cMEATIESAURUS\x10\x02\x12\x16\n\x12PEOPLEEATINGSAURUS\x10\x03\" \n\x08Position\x12\t\n\x01x\x18\x01 \x01(\x02\x12\t\n\x01y\x18\x02 \x01(\x02\x62\x06proto3'
)



_DINOHUNTER_DINOTYPE = _descriptor.EnumDescriptor(
  name='DinoType',
  full_name='my_messages.DinoHunter.DinoType',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='IDK', index=0, number=0,
      serialized_options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='VEGGIESAURUS', index=1, number=1,
      serialized_options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='MEATIESAURUS', index=2, number=2,
      serialized_options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='PEOPLEEATINGSAURUS', index=3, number=3,
      serialized_options=None,
      type=None),
  ],
  containing_type=None,
  serialized_options=None,
  serialized_start=298,
  serialized_end=377,
)
_sym_db.RegisterEnumDescriptor(_DINOHUNTER_DINOTYPE)


_DINOHUNTER_ATTRIBSENTRY = _descriptor.Descriptor(
  name='AttribsEntry',
  full_name='my_messages.DinoHunter.AttribsEntry',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='key', full_name='my_messages.DinoHunter.AttribsEntry.key', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='value', full_name='my_messages.DinoHunter.AttribsEntry.value', index=1,
      number=2, type=9, cpp_type=9, label=1,
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
  serialized_options=b'8\001',
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=180,
  serialized_end=226,
)

_DINOHUNTER_DINO = _descriptor.Descriptor(
  name='Dino',
  full_name='my_messages.DinoHunter.Dino',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='name', full_name='my_messages.DinoHunter.Dino.name', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='type', full_name='my_messages.DinoHunter.Dino.type', index=1,
      number=2, type=14, cpp_type=8, label=1,
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
  serialized_start=228,
  serialized_end=296,
)

_DINOHUNTER = _descriptor.Descriptor(
  name='DinoHunter',
  full_name='my_messages.DinoHunter',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='first_name', full_name='my_messages.DinoHunter.first_name', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='id', full_name='my_messages.DinoHunter.id', index=1,
      number=2, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='attribs', full_name='my_messages.DinoHunter.attribs', index=2,
      number=3, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='dinos', full_name='my_messages.DinoHunter.dinos', index=3,
      number=4, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[_DINOHUNTER_ATTRIBSENTRY, _DINOHUNTER_DINO, ],
  enum_types=[
    _DINOHUNTER_DINOTYPE,
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=34,
  serialized_end=377,
)


_POSITION = _descriptor.Descriptor(
  name='Position',
  full_name='my_messages.Position',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='x', full_name='my_messages.Position.x', index=0,
      number=1, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='y', full_name='my_messages.Position.y', index=1,
      number=2, type=2, cpp_type=6, label=1,
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
  serialized_start=379,
  serialized_end=411,
)

_DINOHUNTER_ATTRIBSENTRY.containing_type = _DINOHUNTER
_DINOHUNTER_DINO.fields_by_name['type'].enum_type = _DINOHUNTER_DINOTYPE
_DINOHUNTER_DINO.containing_type = _DINOHUNTER
_DINOHUNTER.fields_by_name['attribs'].message_type = _DINOHUNTER_ATTRIBSENTRY
_DINOHUNTER.fields_by_name['dinos'].message_type = _DINOHUNTER_DINO
_DINOHUNTER_DINOTYPE.containing_type = _DINOHUNTER
DESCRIPTOR.message_types_by_name['DinoHunter'] = _DINOHUNTER
DESCRIPTOR.message_types_by_name['Position'] = _POSITION
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

DinoHunter = _reflection.GeneratedProtocolMessageType('DinoHunter', (_message.Message,), {

  'AttribsEntry' : _reflection.GeneratedProtocolMessageType('AttribsEntry', (_message.Message,), {
    'DESCRIPTOR' : _DINOHUNTER_ATTRIBSENTRY,
    '__module__' : 'MyMessages_pb2'
    # @@protoc_insertion_point(class_scope:my_messages.DinoHunter.AttribsEntry)
    })
  ,

  'Dino' : _reflection.GeneratedProtocolMessageType('Dino', (_message.Message,), {
    'DESCRIPTOR' : _DINOHUNTER_DINO,
    '__module__' : 'MyMessages_pb2'
    # @@protoc_insertion_point(class_scope:my_messages.DinoHunter.Dino)
    })
  ,
  'DESCRIPTOR' : _DINOHUNTER,
  '__module__' : 'MyMessages_pb2'
  # @@protoc_insertion_point(class_scope:my_messages.DinoHunter)
  })
_sym_db.RegisterMessage(DinoHunter)
_sym_db.RegisterMessage(DinoHunter.AttribsEntry)
_sym_db.RegisterMessage(DinoHunter.Dino)

Position = _reflection.GeneratedProtocolMessageType('Position', (_message.Message,), {
  'DESCRIPTOR' : _POSITION,
  '__module__' : 'MyMessages_pb2'
  # @@protoc_insertion_point(class_scope:my_messages.Position)
  })
_sym_db.RegisterMessage(Position)


_DINOHUNTER_ATTRIBSENTRY._options = None
# @@protoc_insertion_point(module_scope)
