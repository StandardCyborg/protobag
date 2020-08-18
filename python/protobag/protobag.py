import copy
import datetime

import attr
import six

from google.protobuf import json_format
from google.protobuf.timestamp_pb2 import Timestamp

from protobag.ProtobagMsg_pb2 import BagIndex
from protobag.ProtobagMsg_pb2 import Selection
from protobag.ProtobagMsg_pb2 import StampedMessage
from protobag.ProtobagMsg_pb2 import StdMsg
from protobag.ProtobagMsg_pb2 import TopicTime



## ============================================================================
## == Utils ===================================================================
## ============================================================================

def get_type_url(pb_msg):
  # See also `protobag::GetTypeURL()`
  return 'type.googleapis.com/' + pb_msg.DESCRIPTOR.full_name


def to_pb_timestamp(v):
  """Try to convert value `v` to a Protobuf `Timestamp` instance."""
  if isinstance(v, Timestamp):
    return v
  elif isinstance(v, (list, tuple)) and len(v) == 2:
    return Timestamp(seconds=v[0], nanos=v[1])
  elif isinstance(v, datetime.datetime):
    ts = Timestamp()
    ts.FromDatetime(v)
    return ts
  elif isinstance(v, int):
    ts = Timestamp()
    ts.FromSeconds(v)
    return ts
  elif isinstance(v, float):
    sec = int(v)
    nsec = int((v - sec) * 1e9)
    ts = Timestamp(seconds=sec, nanos=nsec)
    return ts
  else:
    raise ValueError(
      "Don't know what to do with timestamp %s" % (v,))


def to_sec_nanos(v):
  """Try to convert value `v` to a (seconds, nanoseconds) tuple"""
  if isinstance(v, (tuple, list)) and len(v) == 2:
    return v
  elif isinstance(v, Timestamp):
    return (v.seconds, v.nanos)
  elif isinstance(v, datetime.datetime):
    import calendar
    return (
      calendar.timegm(v.utctimetuple()), # seconds
      v.microsecond * 1000)              # nanos
  elif isinstance(v, int):
    return (v, 0)
  elif isinstance(v, float):
    sec = int(v)
    nsec = int((v - sec) * 1e9)
    return (sec, nsec)
  else:
    raise ValueError(
      "Don't know what to do with value %s" % (v,))


def to_topic_time(v):
  """Try to convert value `v` to a TopicTime instance."""
  if isinstance(v, TopicTime):
    return v
  elif isinstance(v, dict):
    tt = TopicTime(
              topic=v['topic'],
              timestamp=to_pb_timestamp(v['timestamp']))
    if 'entryname' in v:
      tt.entryname = v['entryname']
    return tt
  elif isinstance(v, (tuple, list)):
    entryname = None
    if len(v) == 2:
      topic, ts = v
    elif len(v) == 3:
      topic, ts, entryname = v
    else:
      raise ValueError("Can't unpack to TopicTime: %s" % (v,))
    tt = TopicTime(
              topic=topic,
              timestamp=to_pb_timestamp(ts))
    if entryname is not None:
      tt.entryname = entryname
    return tt
  else:
    raise ValueError(
      "Don't know what to do with value %s" % (v,))



def build_fds_for_msg(msg):
  """
  Given a Protobuf message `msg` (or message class), build a
  `FileDescriptorSet` that can be used with `DynamicMessageFactory` below (or
  `protobag::DynamicMsgFactory` in C++) to dynamically deserialize instances
  of `msg` at runtime (when the Protobuf-generated code for `msg` is 
  unavailable).

  See also `protobag::DynamicMsgFactory` in C++.

  We run a BFS of `msg`'s descriptor and its dependencies to collect all
  data necessary to decode a `msg` instance.  (NB: the current search is today
  over-complete and pulls in unrelated types, too).  The algorithm below
  mirrors that in `protobag::BagIndexBuilder::Observe()`.  We must run this
  collection in python (and not C++) because we assume we only have the
  Protobuf python-generated code available for `msg` in this code path.

  Args:
      msg (Protobuf message or class): Build a `FileDescriptorSet` based upon
        the `DESCRIPTOR` of this message.
    
  Returns:
  A `FileDescriptorSet` protobuf message instance.
  """

  from google.protobuf.descriptor_pb2 import FileDescriptorProto
  from google.protobuf.descriptor_pb2 import FileDescriptorSet

  q = [msg.DESCRIPTOR.file]
  visited = set()
  files = []
  while q:
    current = q.pop()
    if current.name not in visited:
      # Visit!
      visited.add(current.name)
      
      fd = FileDescriptorProto()
      current.CopyToProto(fd)
      files.append(fd)

      q.extend(current.dependencies)
  
  return FileDescriptorSet(file=files)
    


## ============================================================================
## == Public API ==============================================================
## ============================================================================


###
### Entries that one might find in a Protobag
###

@attr.s(slots=True, eq=True, weakref_slot=False)
class Entry(object):
  """A Protobag Entry, similar to a c++ `protobag::Entry`.  You should probably
  use a subclass like `MessageEntry`, `StampedEntry`, or `RawEntry` below."""

  ## Core Data
  
  entryname = attr.ib(default='', type='str')
  """str: Location of the entry in the Protobag archive."""

  type_url = attr.ib(default='', type='str')
  """str: The Protobuf Type URL (if any) documenting the type of the message.
  This field is empty for `RawEntry`s"""


  ## Optional Context

  serdes = attr.ib(default=None)
  """PBSerdes: Handle to SERDES instance, if available"""

  descriptor_data = attr.ib(default=None)
  """object: Protobuf data needed to decode messages of this type when
  protoc-generated code is not available."""


  @classmethod
  def from_nentry(cls, nentry, serdes=None):
    """Construct and return an `Entry` subclass from the given
    `protobag_native.nentry` `nentry`."""
    serdes = serdes or DEFAULT_SERDES
    if nentry.type_url == '':
      return RawEntry.from_nentry(nentry, serdes=serdes)
    elif nentry.is_stamped:
      return StampedEntry.from_nentry(nentry, serdes=serdes)
    else:
      return MessageEntry.from_nentry(nentry, serdes=serdes)


@attr.s(slots=True, eq=True, weakref_slot=False)
class MessageEntry(Entry):
  """A Protobuf message entry.
  
  Entrynames look like: 'foo/bar'
  """

  msg = attr.ib(default=None)
  """google.protobuf.message.Message: Raw message contents"""

  @classmethod
  def from_msg(cls, entryname, msg, **kwargs):
    return cls(
            entryname=entryname,
            msg=msg,
            type_url=get_type_url(msg),
            **kwargs)

  @classmethod
  def from_nentry(cls, nentry, serdes=None):
    msg = serdes.msg_from_typed_bytes(
            TypedBytes(
              type_url=nentry.type_url,
              entryname=nentry.entryname,
              msg_bytes=nentry.msg_bytes))

    return cls(
      entryname=nentry.entryname,
      type_url=nentry.type_url,
      serdes=serdes,
      msg=msg)
  
  def __str__(self):
    lines = [
      'MessageEntry:',
      '  entryname: %s' % self.entryname,
      '  type_url: %s' % self.type_url,
      '  has serdes: %s' % (self.serdes is not None),
      '  has descriptor_data: %s' % (self.descriptor_data is not None),
      '  msg:\n%s' % str(self.msg), # Uses protobuf text_format
    ]
    return "\n".join(lines)


@attr.s(slots=True, eq=True, weakref_slot=False)
class StampedEntry(MessageEntry):
  """A Protobag StampedMessage entry"""

  topic = attr.ib(default='', type='str')
  """str: The topic (or channel) of time-series data for this message. This is
  *not* the entryname, which might not be known unless the message has been
  written.
  
  Example topic: '/sensor/data'
  """

  timestamp = attr.ib(default='', type=Timestamp, converter=to_pb_timestamp)
  """google.protobuf.timestamp_pb2.Timestamp: The time associated with this 
  entry"""

  @classmethod
  def from_msg(cls, topic, timestamp, msg, **kwargs):
    return cls(
            topic=topic,
            timestamp=to_pb_timestamp(timestamp),
            type_url=get_type_url(msg),
            msg=msg,
            **kwargs)

  @classmethod
  def from_nentry(cls, nentry, serdes=None):
    msg_entry = MessageEntry.from_nentry(nentry, serdes=serdes)

    assert nentry.is_stamped, "Not a stamped message"
    return cls(
      topic=nentry.topic,
      timestamp=Timestamp(
                  seconds=nentry.sec,
                  nanos=nentry.nanos),
      **attr.asdict(msg_entry))

  def __str__(self):
    lines = [
      'StampedEntry:',
      '  topic: %s' % self.topic,
      '  timestamp: %s sec  %s ns' % (
                self.timestamp.seconds, self.timestamp.nanos),
      '  type_url: %s' % self.type_url,
      '  entryname: %s' % self.entryname,
      '  has serdes: %s' % (self.serdes is not None),
      '  has descriptor_data: %s' % (self.descriptor_data is not None),
      '  msg:\n%s' % str(self.msg), # Uses protobuf text_format
    ]
    return "\n".join(lines)


@attr.s(slots=True, eq=True, weakref_slot=False)
class RawEntry(Entry):
  """A Raw entry with no known type url (might not even be
  a Protobuf message!).
  
  Entrynames look like: 'foo/bar.png'
  """
  
  raw_bytes = attr.ib(default='', type='bytearray')
  """bytearray: Raw message contents"""

  @classmethod
  def from_bytes(cls, entryname, raw_bytes, **kwargs):
    return cls(
            entryname=entryname,
            raw_bytes=raw_bytes,
            type_url='', # Raw messages have no type
            **kwargs)

  @classmethod
  def from_nentry(cls, nentry, serdes=None):
    return cls(
      entryname=nentry.entryname,
      type_url=nentry.type_url,
      serdes=serdes,

      raw_bytes=nentry.msg_bytes)
  
  def __str__(self):
    lines = [
      'RawEntry:',
      '  entryname: %s' % self.entryname,
      '  raw_bytes: %s ... (%s bytes)' % (
        self.raw_bytes[:20].decode() if self.raw_bytes is not None else 'None',
        len(self.raw_bytes) if self.raw_bytes is not None else 0),
    ]
    return "\n".join(lines)


###
### Interacting with Protobag files
###

class Protobag(object):

  def __init__(self, path=None, serdes=None, msg_classes=None):
    """Handle to a Protobag archive on disk at the given `path`.  Use this
    object to help organize your reads and writes to an existing or new 
    Protobag archive.
    """
  
    self._path = str(path or '')
    self._serdes = serdes
    self._writer = None
    if msg_classes is not None:
      for msg_cls in msg_classes:
        self.register_msg_type(msg_cls)
  

  ## Utils

  @property
  def serdes(self):
    if self._serdes is None:
      self._serdes = copy.deepcopy(DEFAULT_SERDES)
    return self._serdes
  
  @property
  def path(self):
    return self._path

  def register_msg_type(self, msg_cls):
    """Shortcut to update the wrapped decoder"""
    self.serdes.register_msg_type(msg_cls)


  ## Reading

  def get_bag_index(self):
    """Get the (latest) `BagIndex` instance from this protobag."""
    from protobag.protobag_native import PyReader
    bag_index_str = PyReader.get_index(self._path)
    msg = BagIndex()
    msg.ParseFromString(bag_index_str)
    return msg

  def get_topics(self):
    """Get the list of topics for any time-series data in this protobag."""
    from protobag.protobag_native import PyReader
    return PyReader.get_topics(self._path)

  def iter_entries(
        self,
        selection=None,
        dynamic_decode=True,
        sync_using_max_slop=None):
    """Create a `ReadSession` and iterate through entries specified by
    the given `selection`; by default "SELECT ALL" (read all entries in 
    the protobag).

    Args:
      selection (Selection or str): Select only entries that match this
        `Selection` instance (or protobuf-string-serialized string). Use
        `SelectionBuilder` to help create these.  By default, we read all
        entries in the protobag.
      dynamic_decode (optional bool): Enable dynamic decoding, as a fallback,
        of protobuf messages using the descriptor data indexed in the protobag.
        If you lack the generated protobuf message definition code for your
        messages, try this option; note that dynamic decoding is slower than
        normal protobuf deserialization.
      sync_using_max_slop (optional protobag_native.MaxSlopTimeSyncSpec):
        Synchronize StampedEntry instances in the `selection` using
        a max slop algorithm.  FMI see `protobag_native.PyMaxSlopTimeSync`.
    
    Returns:
    Generates `Entry` subclass instances (or a list of `Entry` instances
      when synchronization is requested)
    """

    if selection is None:
      selection_bytes = SelectionBuilder.select_all().SerializeToString()
    elif isinstance(selection, Selection):
      selection_bytes = selection.SerializeToString()
    else:
      selection_bytes = selection
    
    if dynamic_decode:
      self.serdes.register_dynamic_types_from_index(self.get_bag_index())

    def iter_results(pb_seq, unpack):
      while True:
        res = pb_seq.get_next()
        # NB: We use this exception instead of pybind11::stop_iteration due
        # to a bug in pybind related to libc++.  FMI see:
        # * https://gitter.im/pybind/Lobby?at=5f18cfc9361e295cf01fd21a
        # * (This fix appears to still have a bug)
        #      https://github.com/pybind/pybind11/pull/949
        if res is not None:
          yield unpack(res)
        else:
          return

    from protobag.protobag_native import PyReader
    reader = PyReader()
    reader.start(self._path, selection_bytes)

    if sync_using_max_slop is not None:
      # Synchronize!
      from protobag.protobag_native import PyMaxSlopTimeSync
      sync = PyMaxSlopTimeSync()
      sync.start(reader, sync_using_max_slop)

      def unpack_bundle(bundle):
        return [
          Entry.from_nentry(nentry, serdes=self.serdes)
          for nentry in bundle
        ]
      for entry in iter_results(sync, unpack_bundle):
        yield entry
    
    else:

      unpack = lambda nentry: Entry.from_nentry(nentry, serdes=self.serdes)
      for entry in iter_results(reader, unpack):
        yield entry
  
  def get_entry(self, entryname):
    """Convenience for getting a single entry with `entryname`."""
    sel = SelectionBuilder.select_entry(entryname)
    for entry in self.iter_entries(selection=sel):
      return entry
    raise KeyError("Protobag %s missing entry %s" % (self._path, entryname))
  

  ## Writing

  def create_writer(
        self,
        bag_format='',
        save_timeseries_index=True,
        save_descriptor_index=True):
      """Create and return a `Protobag._Writer` instance that encapsulates
      a single write session.
      
      Args:
        bag_format: (optional string) Write the bag in this archive format; if
          empty, infer the format from the file name. Options:
            * archive formats: 'zip', 'tar', etc
            * write as a directory on local filesystem: 'directory'
          FMI see `protobag::archive::Archive::Spec`.
        save_timeseries_index: (optional bool) When writing timestamped 
          messages to the bag (via `write_stamped_msg`), index these messages
          in order to support features like time-ordered playback.  Ignored if
          the user does not write timestamped messages.
        save_descriptor_index: (optional bool) For every message written,
          collect Protobuf descriptor data and index it into the bag in order
          to support dynamic decoding at read / playback time (i.e. the reader
          may decode messages without having any Protobuf-generated code
          for the message types available).  Incurs a small runtime cost for
          each *distinct* message type recorded.  Note that the pure C++
          implementation of this feature is somewhat faster.
      
      Returns:
      A `Protobag._Writer` instance.
      """

      from protobag.protobag_native import WriterSpec
      spec = WriterSpec()
      spec.path = self._path
      spec.format = bag_format
      spec.save_timeseries_index = save_timeseries_index
      spec.save_descriptor_index = save_descriptor_index
      return _Writer(spec)


class _Writer(object):
  def __init__(self, spec):
    self._spec = spec
    self._indexed_type_urls = set()

    from protobag.protobag_native import PyWriter
    self._writer = PyWriter()
    self._writer.start(spec)

  def close(self):
    self._writer.close()  


  def write_entry(self, entry):
    if isinstance(entry, RawEntry):
      self.write_raw(entry.entryname, entry.raw_bytes)
    elif isinstance(entry, MessageEntry):
      self.write_msg(entry.entryname, entry.msg)
    elif isinstance(entry, StampedEntry):
      self.write_stamped_msg(
        entry.topic,
        entry.msg,
        timestamp=entry.timestamp)
    else:
      raise ValueError("Don't know what to do with %s" % (entry,))


  def write_raw(self, entryname, raw_bytes):
    self._writer.write_raw(entryname, raw_bytes)

  def write_msg(self, entryname, msg):
    self._writer.write_msg(
          entryname,
          get_type_url(msg),
          msg.SerializeToString(),
          self._maybe_get_serialized_fds(msg))
  
  def write_stamped_msg(self, topic, msg, timestamp=None, t_sec=0, t_nanos=0):
    if timestamp is not None:
      t_sec, t_nanos = to_sec_nanos(timestamp)

    self._writer.write_stamped_msg(
          topic,
          t_sec,
          t_nanos,
          get_type_url(msg),
          msg.SerializeToString(),
          self._maybe_get_serialized_fds(msg))
  
  def _maybe_get_serialized_fds(self, msg):
    type_url = get_type_url(msg)
    if type_url in self._indexed_type_urls:
      return None

    fds = build_fds_for_msg(msg)
    fds_bytes = fds.SerializeToString()
    return fds_bytes


###
### Selecting Data from Protobags
###

class SelectionBuilder(object):
  """Helper for creating Protobag (read) Selections."""
  
  @classmethod
  def select_all(cls, all_entries_are_raw=False):
    return Selection(
              select_all={'all_entries_are_raw': all_entries_are_raw})

  @classmethod
  def select_entries(
        cls,
        entrynames,
        ignore_missing_entries=False,
        entries_are_raw=False):
    
    return Selection(
              entrynames={
                'entrynames': entrynames,
                'ignore_missing_entries': ignore_missing_entries,
                'entries_are_raw': entries_are_raw,
              })
  
  @classmethod
  def select_entry(cls, entryname, **kwargs):
    return cls.select_entries([entryname], **kwargs)

  @classmethod
  def select_window(
        cls,
        topics=None,
        start_time=None,
        end_time=None,
        exclude_topics=None):

    spec = {}
    if topics is not None:
      spec['topics'] = topics
    if start_time is not None:
      spec['start'] = to_pb_timestamp(start_time)
    if end_time is not None:
      spec['end'] = to_pb_timestamp(end_time)
    if exclude_topics is not None:
      spec['exclude_topics'] = exclude_topics
    return Selection(window=spec)
  
  @classmethod
  def select_window_all(cls):
    return cls.select_window()

  @classmethod
  def select_events(
        cls,
        topic_times=None,
        require_all=False):
    spec = {}
    if topic_times is not None:
      spec['events'] = [
        to_topic_time(tt)
        for tt in topic_times
      ]
    spec['require_all'] = require_all
    return Selection(events=spec)



## ============================================================================
## == SERDES: Decoding / Encoding Protobuf Messages ===========================
## ============================================================================

DEFAULT_MSG_TYPES = (
    StdMsg.Bool,
    StdMsg.Int,
    StdMsg.Float,
    StdMsg.String,
    StdMsg.Bytes,
    StdMsg.SSMap,
    
    BagIndex,
    Selection,
    StampedMessage,
    StdMsg,
    TopicTime,
)

@attr.s(slots=True, eq=True, weakref_slot=False)
class TypedBytes(object):
  type_url = attr.ib(default='', type='str')
  entryname = attr.ib(default='', type='str')
  msg_bytes = attr.ib(default=None, type='bytearray')

  def __str__(self):
    lines = [
      'TypedBytes:',
      '  type_url: %s' % self.type_url,
      '  entryname: %s' % self.entryname,
      '  msg_bytes: %s ... (%s bytes)' % (
        self.msg_bytes[:20].decode() if self.msg_bytes is not None else 'None',
        len(self.msg_bytes) if self.msg_bytes is not None else 0),
    ]
    return "\n".join(lines)


class PBSerdes(object):
  """A SERDES utility for Protobuf messages.  Not a primary public protobag 
  API; you probably want to use protobag.Protobag directly.  
  How `PBSerdes` helps:
   Decoding:
   * When `protoc`-generated Protobuf python code is available for your
       messages, you can register that code with `PBSerdes` and then `PBSerdes`
       will help you decode arbitrary messages given a `TypedBytes`
       instance.
   * When `protoc`-generated code is unavailable, you can register descriptor
       data (e.g. the kind that `protobag` indexes at recording time) with
       `PBSerdes`, and `PBSerdes` will use Protobuf's dynamic message support
       for decoding `TypedBytes`.
   Encoding:
   * Facilitates lookups of cached descriptor data (primarily for DictRowEntry
       API).
  """

  ## Setup

  @classmethod
  def create_with_types(cls, pb_msg_clss):
    decoder = cls()
    for msg_cls in pb_msg_clss:
      decoder.register_msg_type(msg_cls)
    return decoder

  def register_msg_type(self, pb_msg_cls):
    type_url = get_type_url(pb_msg_cls)
    self._type_url_to_cls[type_url] = pb_msg_cls

  def register_dynamic_types_from_index(self, bag_index):
    if hasattr(bag_index, 'descriptor_pool_data'):
      dpd = bag_index.descriptor_pool_data
      self._dynamic_factory = \
        DynamicMessageFactory.create_from_descriptor_pool_data(dpd)
          # TODO support multiple indices
      
      self._type_url_to_descriptor_data = dict(dpd.type_url_to_descriptor)
    
  def register_descriptor_data(self, type_url, descriptor_data):
    if type_url not in self._type_url_to_descriptor_data:
      is_binary = (
        isinstance(descriptor_data, six.string_types) or
        isinstance(descriptor_data, six.binary_type) or
        isinstance(descriptor_data, bytearray))
      if is_binary:
        from google.protobuf.descriptor_pb2 import FileDescriptorSet
        fds = FileDescriptorSet()
        fds.ParseFromString(descriptor_data)
        descriptor_data = fds
      
      self._type_url_to_descriptor_data[type_url] = descriptor_data

      if self._dynamic_factory is None:
        self._dynamic_factory = DynamicMessageFactory()
      self._dynamic_factory.register_type(type_url, descriptor_data)

  ## I/O

  @classmethod
  def msg_to_typed_bytes(cls, msg):
    """Serialize protobuf `msg` and return a `TypedBytes` wrapper"""
    return TypedBytes(
              type_url=get_type_url(msg),
              msg_bytes=msg.SerializeToString()) # TODO support text format

  def msg_from_typed_bytes(self, typed_bytes):
    """Decode string-serialized Protobuf message (wraped in `TypedBytes`)
    `typed_bytes` and return a decoded Protobuf message instance.
    Picks a message deserializer based upon:
     * `type_url`, the identifer of a message class that the user registered
        using `register_msg_type()`
     * using dynamic Protobuf message generation and Protobuf descriptor data
        indexed to the Protobag at write time (and that data has been made
        available through `register_dynamic_types_from_index()`)
    """
    if typed_bytes.type_url in self._type_url_to_cls:
      msg_cls = self._type_url_to_cls[typed_bytes.type_url]
      msg = msg_cls()
      msg.ParseFromString(typed_bytes.msg_bytes) # TODO support text format
      return msg
    elif self._dynamic_factory is not None:
      return self._dynamic_factory.dynamic_decode(typed_bytes)
    else:
      raise ValueError(
              "Could not decode message from %s \n%s" % (
                typed_bytes, str(self)))

  def get_msg_cls_for_type(self, type_url):
    if type_url in self._type_url_to_cls:
      return self._type_url_to_cls[type_url]
    elif type_url in self._type_url_to_descriptor_data:
      assert self._dynamic_factory
      return self._dynamic_factory.get_msg_cls_for_type_url(type_url)
  
  def get_descriptor_data_for_type(
            self,
            type_url,
            msg=None,
            lazyily_register=True):
    """Fetch the descriptor data for `type_url`; lazily deduce such
    descriptor data and register it only if `lazyily_register`."""
    
    if type_url in self._type_url_to_descriptor_data:
      return self._type_url_to_descriptor_data[type_url]
    else:
      if type_url in self._type_url_to_cls:
        msg_cls = self._type_url_to_cls[type_url]
        descriptor_data = build_fds_for_msg(msg_cls)
      elif msg is not None:
        descriptor_data = build_fds_for_msg(msg)
      else:
        raise KeyError("Can't find or build descriptor data for %s" % type_url)

      if lazyily_register:
        self.register_descriptor_data(type_url, descriptor_data)
      return descriptor_data


  ## Misc

  def __init__(self):
    self._type_url_to_cls = {}
    self._dynamic_factory = None
    self._type_url_to_descriptor_data = {}

  def __str__(self):
    return '\n'.join((
      'PBSerdes',
      'User-registered types:',
      '\n'.join(sorted(self._type_url_to_cls.keys())),
      '',
      'Dynamic type support:',
      str(self._dynamic_factory),
    ))

# NB: each protobag.Protobag instance owns a *copy* of this default
DEFAULT_SERDES = PBSerdes.create_with_types(DEFAULT_MSG_TYPES)



class DynamicMessageFactory(object):
  def __init__(self):
    from google.protobuf import symbol_database
    self._db = symbol_database.Default()
    self._registered_type_urls = set()
    self._entryname_to_type_url = {}

  @classmethod
  def create_from_descriptor_pool_data(cls, dpd):
    """Create a new `DynamicMessageFactory` instance from a
    `protobag.DescriptorPoolData` message."""
    f = cls()
    f.register_entries(dpd.entryname_to_type_url)
    for type_url, fds in dpd.type_url_to_descriptor.items():
      f.register_type(type_url, fds)
    return f

  def register_entry(self, entryname, type_url):
    self._entryname_to_type_url[entryname] = type_url
  
  def register_entries(self, entryname_to_type_url):
    for entryname, type_url in entryname_to_type_url.items():
      self.register_entry(entryname, type_url)

  def register_type(self, type_url, fds):
    if type_url not in self._registered_type_urls:
      for fd in fds.file:
        self._db.pool.Add(fd)
      self._registered_type_urls.add(type_url)
  
  def dynamic_decode(self, typed_bytes):
    """Decode the given `typed_bytes` into a Protobuf message instance of
    either type `typed_bytes.type_url` or whatever type the entry
    `typed_bytes.entryname` was indexed to have.
    """
    assert typed_bytes.type_url or typed_bytes.entryname, \
      "Need a type_url or entryname"
    
    # Prefer entryname, which Protobag pins to a specific FileDescriptorSet
    # at time of writing (in case the message type evolves between write 
    # sessions).
    if typed_bytes.entryname:
      assert typed_bytes.entryname in self._entryname_to_type_url, \
        "Unregistered protobag entry: %s" % typed_bytes.entryname
      typed_bytes.type_url = self._entryname_to_type_url[typed_bytes.entryname]
    
    try:
      msg_cls = self.get_msg_cls_for_type_url(typed_bytes.type_url)
    except Exception as e:
      raise KeyError("Cannot dynamic decode %s: %s" % (typed_bytes, e))
    
    msg = msg_cls()
    msg.ParseFromString(typed_bytes.msg_bytes)
      # TODO support text format
    return msg
  
  def get_msg_cls_for_type_url(self, type_url):
    # Based upon https://github.com/protocolbuffers/protobuf/blob/86b3ccf28ca437330cc42a2b3a75a1314977fcfd/python/google/protobuf/json_format.py#L397
    type_name = type_url.split('/')[-1]
    try:
      descriptor = self._db.pool.FindMessageTypeByName(type_name)
    except Exception as e:
      raise KeyError(
        "Could not find descriptor for %s: %s" % (type_url, e))

    msg_cls = self._db.GetPrototype(descriptor)
    return msg_cls

  def __str__(self):
    return '\n'.join((
      'protobag.DynamicMessageFactory',
      'Known types:',
      '\n'.join(sorted(desc.name for desc in self._db._classes.keys())),
      '',
      'entryname -> type_url',
      '\n'.join(
        '%s -> %s' % (k, v)
        for (k, v) in sorted(self._entryname_to_type_url.items()))
    ))



## ============================================================================
## == Protobag <-> Tables of Rows =============================================
## ============================================================================

def to_pb_timestamp_safe(v):
  try:
    return to_pb_timestamp(v)
  except Exception:
    return None

@attr.s(slots=True, eq=True, weakref_slot=False)
class DictRowEntry(object):
  """Utility for converting Protobag entries to and from python dicts / 
  table "rows".  

  # Examples:
  ## Entry -> dict
  >>> bag = protobag.Protobag(path='my_bag.zip')
  >>> entry = bag.get_entry('foo')
  >>> row = DictRowEntry.from_entry(entry)
      # row is now a DictRowEntry; use the attributes directly or try
      # `attrs.asdict()`

  ## dict -> Entry
  >>> bag = protobag.Protobag(path='my_bag.zip', msg_classes=(MyPbMsgType,))
  >>> d = dict(
            entryname='foo',
            type_url='type.googleapis.com/MyPbMsgType',
              # Can get this using `get_type_url(MyPbMsgType)`
            msg_dict={'x': 5, 'y': 7},
            serdes=bag.serdes)
        # Need to provide the `serdes` that knows how to encode `MyPbMsgType`
        # instances from python dicts.  If your dict has `descriptor_data`, 
        # you can omit the serdes
  >>> row = DictRowEntry(**d)
  >>> entry = row.to_entry()
        # entry is now a MessageEntry instance
  >>> writer = bag.create_writer()
  >>> writer.write_entry(entry)

  """
  
  msg_dict = attr.ib(factory=dict, type=dict)
  
  entryname = attr.ib(default='', type='str')
  type_url = attr.ib(default='', type='str')
  topic = attr.ib(default='', type='str')
  timestamp = attr.ib(
    default=None, type=Timestamp, converter=to_pb_timestamp_safe)
  descriptor_data = attr.ib(default=None)
  serdes = attr.ib(default=DEFAULT_SERDES)

  @classmethod
  def from_entry(cls, entry):
    if isinstance(entry, RawEntry):
      msg_dict = {'protobag_raw_entry_bytes': entry.raw_bytes}
    else:
      msg_dict = json_format.MessageToDict(entry.msg)

    return cls(
      entryname=entry.entryname,
      type_url=entry.type_url,
      msg_dict=msg_dict,
      
      serdes=entry.serdes,
      descriptor_data=
        entry.serdes.get_descriptor_data_for_type(entry.type_url)
        if not isinstance(entry, RawEntry)
        else None,

      topic=entry.topic if isinstance(entry, StampedEntry) else '',
      timestamp=entry.timestamp if isinstance(entry, StampedEntry) else None)

  def is_raw_entry(self):
    return 'protobag_raw_entry_bytes' in self.msg_dict
  
  def is_stamped_entry(self):
    return self.topic and (self.timestamp is not None)

  def to_entry(self):
    if self.is_raw_entry():
      return RawEntry(
                entryname=self.entryname,
                type_url='',
                raw_bytes=self.msg_dict['protobag_raw_entry_bytes'],
                serdes=self.serdes)
    else:
      if self.descriptor_data:
        # Maybe use this data to facilitate messsage parsing below
        self.serdes.register_descriptor_data(
          self.type_url,
          self.descriptor_data)
      msg_cls = self.serdes.get_msg_cls_for_type(self.type_url)
      msg = msg_cls()
      json_format.ParseDict(self.msg_dict, msg)

      if self.is_stamped_entry():
        return StampedEntry(
                entryname=self.entryname,
                type_url=self.type_url,
                msg=msg,
                
                topic=self.topic,
                timestamp=self.timestamp,
                
                serdes=self.serdes,
                descriptor_data=self.descriptor_data)
      else:
        return MessageEntry(
                entryname=self.entryname,
                type_url=self.type_url,
                msg=msg,
                
                serdes=self.serdes,
                descriptor_data=self.descriptor_data)

  def __str__(self):
    import pprint

    def get_descriptor_data_formatted():
      v = self.descriptor_data
      if isinstance(v, six.string_types):
        return "(binary) %s ... (%s bytes)" % (v[:20].decode(), len(v))
      elif hasattr(v, 'DESCRIPTOR'):
        return "(protobuf message) %s %s (%s bytes)" % (
          v.DESCRIPTOR.full_name,
          str(v)[:20],
          len(v.SerializeToString()))
      else:
        return str(v)

    return '\n'.join((
      'protobag.DictRowEntry:',
      '  entryname: %s' % self.entryname,
      '  topic: %s timestamp: %s' % (
        self.topic,
        '%s sec %s ns' % (self.timestamp.seconds, self.timestamp.nanos))
      if self.timestamp is not None
      else '  (not a time-series entry)',
      '  type_url: %s' % self.type_url,
      '  has serdes: %s' % (self.serdes is not None),
      '  descriptor_data: %s' % get_descriptor_data_formatted(),
      '  msg_dict:\n %s' % pprint.pformat(self.msg_dict),
    ))

