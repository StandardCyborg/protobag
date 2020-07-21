import copy
import datetime

from google.protobuf.timestamp_pb2 import Timestamp

from protobag.ProtobagMsg_pb2 import BagIndex
from protobag.ProtobagMsg_pb2 import Selection
from protobag.ProtobagMsg_pb2 import StampedMessage
from protobag.ProtobagMsg_pb2 import StdMsg
from protobag.ProtobagMsg_pb2 import TopicTime


## ============================================================================
## == Public API ==============================================================
## ============================================================================

class Protobag(object):

  def __init__(self, path=None, decoder=None, msg_classes=None):
    """TODO
    """
  
    self._path = str(path or '')
    self._decoder = decoder
    self._writer = None
    if msg_classes is not None:
      for msg_cls in msg_classes:
        self.register_msg_type(msg_cls)
  

  ## Utils

  @property
  def decoder(self):
    if self._decoder is None:
      self._decoder = copy.deepcopy(_DefaultPBDecoder)
    return self._decoder
  
  @property
  def path(self):
    return self._path

  def register_msg_type(self, msg_cls):
    """Shortcut to update the wrapped decoder"""
    self.decoder.register_msg_type(msg_cls)


  ## Reading

  def get_bag_index(self):
    """Get the (latest) `BagIndex` instance from this protobag."""
    from protobag.protobag_native import Reader
    bag_index_str = Reader.get_index(self._path)
    msg = BagIndex()
    msg.ParseFromString(bag_index_str)
    return msg

  def iter_entries(self, selection=None, dynamic_decode=True):
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
    
    Returns:
    Generates `PBEntry` instances
    """

    if selection is None:
      selection_bytes = SelectionBuilder.select_all().SerializeToString()
    elif isinstance(selection, Selection):
      selection_bytes = selection.SerializeToString()
    else:
      selection_bytes = selection
    
    if dynamic_decode:
      self.decoder.register_dynamic_types_from_index(self.get_bag_index())

    from protobag.protobag_native import Reader
    reader = Reader()
    reader.start(self._path, selection_bytes)
    for nentry in reader:
      yield PBEntry(nentry=nentry, decoder=self.decoder)
  
  def get_entry(self, entryname):
    """Convenience for getting a single entry with `entryname`."""
    sel = SelectionBuilder.get_entry(entryname)
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

    from protobag.protobag_native import Writer
    self._writer = Writer()
    self._writer.start(spec)

  def close(self):
    self._writer.close()  

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
      calendar.timegm(dt.utctimetuple()), # seconds
      dt.microsecond * 1000)              # nanos
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
  data necessary to decode a `msg` instance.  The algorithm below mirrors that
  in `protobag::BagIndexBuilder::Observe()`.  We must run this collection in
  python because (we assume) we only have the Protobuf python-generated code
  available for `msg` in this code path.

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
## == Protobuf Message Decoding ===============================================
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

class PBDecoder(object):

  def __init__(self):
    self._type_url_to_cls = {}
    self._dynamic_factory = None

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

  def decode(self, type_url, msg_bytes, entryname=None):
    """Decode string-serialized Protobuf message `msg_bytes`, interpreting
    the bytes as `type_url`, and return a decoded Protobuf message instance.
    Picks a message deserializer based upon:
     * `type_url`, the identifer of a message class that the user registered
        using `register_msg_type()`
     * using dynamic Protobuf message generation and Protobuf descriptor data
        indexed to the Protobag at write time (and that data has been made
        available through `register_dynamic_types_from_index()`)
    """
    if type_url in self._type_url_to_cls:
      
      msg_cls = self._type_url_to_cls[type_url]
      msg = msg_cls()
      msg.ParseFromString(msg_bytes) # TODO support text format
      return msg

    elif self._dynamic_factory is not None:

      return self._dynamic_factory.dynamic_decode(
                    msg_bytes,
                    type_url=type_url,
                    entryname=entryname)

    else:
      
      raise ValueError("Could not decode message for type %s " % type_url)
    
  def __str__(self):
    return '\n'.join((
      'PBDecoder',
      'User-registered types:',
      '\n'.join(sorted(self._type_url_to_cls.keys())),
      '',
      'Dynamic type support:',
      str(self._dynamic_factory),
    ))

_DefaultPBDecoder = PBDecoder.create_with_types(DEFAULT_MSG_TYPES)



class DynamicMessageFactory(object):
  def __init__(self):
    from google.protobuf import symbol_database
    self._db = symbol_database.Default()
    self._entryname_to_type_url = {}

  @classmethod
  def create_from_descriptor_pool_data(cls, dpd):
    """Create a new `DynamicMessageFactory` instance from a
    `protobag.DescriptorPoolData` message."""
    f = cls()
    f.register_entries(dpd.entryname_to_type_url)
    for fds in dpd.type_url_to_descriptor.values():
      f.register_types(fds)
    return f

  def register_entry(self, entryname, type_url):
    self._entryname_to_type_url[entryname] = type_url
  
  def register_entries(self, entryname_to_type_url):
    for entryname, type_url in entryname_to_type_url.items():
      self.register_entry(entryname, type_url)

  def register_types(self, fds):
    for fd in fds.file:
      self._db.pool.Add(fd)
  
  def dynamic_decode(self, msg_bytes, type_url=None, entryname=None):
    """Decode the given `msg_bytes` into a Protobuf message instance of either
    type `type_url` or whatever type the entry `entryname` was indexed to have.
    """
    assert type_url or entryname, "Need a type_url or entryname"
    
    # Prefer entryname, which Protobag pins to a specific FileDescriptorSet
    # at time of writing (in case the message type evolves between write 
    # sessions).
    if entryname is not None:
      assert entryname in self._entryname_to_type_url, \
        "Unregistered protobag entry: %s" % entryname
      type_url = self._entryname_to_type_url[entryname]
    
    # Based upon https://github.com/protocolbuffers/protobuf/blob/86b3ccf28ca437330cc42a2b3a75a1314977fcfd/python/google/protobuf/json_format.py#L397
    type_name = type_url.split('/')[-1]
    try:
      descriptor = self._db.pool.FindMessageTypeByName(type_name)
    except Exception as e:
      raise KeyError(
        "Could not find descriptor for %s: %s" % ((type_url, entryname), e))

    msg_cls = self._db.GetPrototype(descriptor)
    msg = msg_cls()
    msg.ParseFromString(msg_bytes)
      # TODO support text format
    return msg
  
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
## == Entries (Reading) =======================================================
## ============================================================================

class PBEntry(object):
  """A single entry in a protobag; analogous to a C++ `protobag::Entry`"""

  __slots__ = ['_nentry', '_pb_msg', '_decoder', '_topic', '_timestamp']

  def __init__(self, nentry=None, decoder=None):
    if nentry is None:
      from protobag.protobag_native import native_entry
      nentry = native_entry()
    self._nentry = nentry
    self._decoder = decoder or _DefaultPBDecoder
    self._pb_msg = None
    self._topic = None
    self._timestamp = None

  def __str__(self):
    lines = []
    if self.is_stamped_message():
      lines += [
        "Topic: %s" % self.topic,
        "Timestamp: %s sec  %s ns" % (
          self.timestamp.seconds, self.timestamp.nanos),
      ]
    lines += [
      "Entryname: %s" % self.entryname,
      "type_url: %s" % self.type_url,
      "size: %s bytes" % len(self.raw_msg_bytes),
    ]

    if self._pb_msg:
      lines.append("msg: \n%s\n" % str(self._pb_msg))
    else:
      lines.append("msg: (not yet deserialized)")
    
    return "\n".join(lines)

  @property
  def entryname(self):
    return self._nentry.entryname
  
  @property
  def raw_msg_bytes(self):
    return self._nentry.msg_bytes
  
  @property
  def type_url(self):
    return self._nentry.type_url

  def get_msg(self):
    if not self._pb_msg:
      if self.type_url:
        # NB: If this is a Stamped Message, protobag_native will have already
        # unwrapped the StampedMessage wrapper.
        self._pb_msg = self._decoder.decode(self.type_url, self.raw_msg_bytes)
      else:
        # This message is raw
        self._pb_msg = self.raw_msg_bytes
    return self._pb_msg


  # For Stamped Messages only

  def is_stamped_message(self):
    return self._nentry.is_stamped

  @property
  def topic(self):
    assert self._nentry.is_stamped, "Not a stamped message"
    return self._nentry.topic
  
  @property
  def timestamp(self):
    if not self._timestamp:
      assert self._nentry.is_stamped, "Not a stamped message"
      self._timestamp = Timestamp(
                            seconds=self._nentry.sec,
                            nanos=self._nentry.nanos)
    return self._timestamp



## ============================================================================
## == Selections ==============================================================
## ============================================================================

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
