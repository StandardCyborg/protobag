import copy
import datetime

from google.protobuf.timestamp_pb2 import Timestamp

from protobag.ProtobagMsg_pb2 import BagIndex
from protobag.ProtobagMsg_pb2 import Selection
from protobag.ProtobagMsg_pb2 import StampedMessage
from protobag.ProtobagMsg_pb2 import StdMsg
from protobag.ProtobagMsg_pb2 import TopicTime


## ============================================================================
## == Utils ===================================================================
## ============================================================================

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
  

## ============================================================================
## == PBDecoder ===============================================================
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

  @classmethod
  def create_with_types(cls, pb_msg_clss):
    decoder = cls()
    for msg_cls in pb_msg_clss:
      decoder.register_msg_type(msg_cls)
    return decoder

  @classmethod
  def get_type_url(cls, pb_msg_cls):
    # See also `protobag::GetTypeURL()`
    return 'type.googleapis.com/' + pb_msg_cls.DESCRIPTOR.full_name

  def register_msg_type(self, pb_msg_cls):
    type_url = self.get_type_url(pb_msg_cls)
    self._type_url_to_cls[type_url] = pb_msg_cls

  def decode(self, type_url, msg_bytes):
    """TODO"""
    if type_url in self._type_url_to_cls:
      
      msg_cls = self._type_url_to_cls[type_url]
      msg = msg_cls()
      msg.ParseFromString(msg_bytes) # TODO support text format
      return msg
    
    else:
      # TODO dynamic message if supported
      raise KeyError(type_url)

_DefaultPBDecoder = PBDecoder.create_with_types(DEFAULT_MSG_TYPES)


## ============================================================================
## == Entries =================================================================
## ============================================================================

class PBEntry(object):
  """A single entry in a protobag; analogous to a C++ `protobag::Entry`"""

  __slots__ = ['_nentry', '_pb_msg', '_decoder']

  def __init__(self, nentry=None, decoder=None):
    if nentry is None:
      from protobag.protobag_native import native_entry
      nentry = native_entry()
    self._nentry = nentry
    self._decoder = decoder or _DefaultPBDecoder
    self._pb_msg = None

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
        self._pb_msg = self._decoder.decode(self.type_url, self.raw_msg_bytes)
          # TODO decode stamped
      else:
        # This message is raw
        self._pb_msg = self.raw_msg_bytes
    return self._pb_msg

  def __str__(self):
    msg_txt = str(self.get_msg()) # TODO: support no-decode
    return "\n".join((
      "Entryname: %s" % self.entryname,
      "type_url: %s" % self.type_url,
      "msg: \n%s\n" % msg_txt,
    ))

  # def set_msg(self, msg):
  #   # TODO do pb serialize etc
  #   return

"""
reading:
[ ] -> entry or keyerror
readsession.__iter__() -> generate entry

 * raw entry -> just string data
    Entry:
      entryname
      str data
 * pb message -> an Any, maybe auto-decode
    Entry:
      entryname
      Any
      ... descriptor?
    or
    Entry:
      entryname
      type_url
      msg
 * stamped entry -> also maybe auto-decode
    Entry:
      entryname
      topic
      timestamp
      Any or msg...




writing:


"""

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


class Protobag(object):

  ## Public API

  def __init__(self, path=None, decoder=None, msg_classes=None):
    """TODO
    """
  
    self._path = str(path or '')
    self._decoder = decoder
    if msg_classes is not None:
      for msg_cls in msg_classes:
        self.register_msg_type(msg_cls)
  
  @property
  def decoder(self):
    if self._decoder is None:
      self._decoder = copy.deepcopy(_DefaultPBDecoder)
    return self._decoder

  def register_msg_type(self, msg_cls):
    """Shortcut to update the wrapped decoder"""
    self.decoder.register_msg_type(msg_cls)

  def iter_entries(self, selection=None, dynamic_decode=True):
    """Create a `ReadSession` and iterate through entries specified by
    the given `selection`; by default "SELECT ALL" (read all entries in 
    the protobag).

    Args:
      selection - (Selection or str): Select only entries that match this
        `Selection` instance (or protobuf-string-serialized string). Use
        `SelectionBuilder` to help create these.  By default, we read all
        entries in the protobag.
      dynamic_decode - (optional bool): Enable dynamic decoding, as a fallback,
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
      print('todo update decoder with index exactly once')

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

  # def write_entry(self, entry=None, topic=None, ts=None, msg=None):
  #   """TODO Docs
    
  #   Args:
  #     entry (Entry): Write this `Entry` instance
  #     topic (str): Create and write an entry with this topic
  #     ts (int, float, datetime, protobuf.Timestamp): Create and 
  #       write an entry with this timestamp.  Int and float values interpreted
  #       as unix epoch seconds.
  #     msg (Any): A protobuf message
    
  #   """

  #   # Normalize timestamp
  #   if ts is not None:
  #     if isinstance(ts, datetime.datetime):
  #       t = ts
  #       ts = Timestamp()
  #       ts.FromDatetime(t)
  #     elif isinstance(ts, int):
  #       t = ts
  #       ts = Timestamp()
  #       ts.FromSeconds(t)
  #     elif isinstance(ts, float):
  #       sec = int(ts)
  #       nsec = int((ts - sec) * 1e9)
  #       ts = Timestamp(seconds=sec, nanos=nsec)
  #     else:
  #       raise ValueError(
  #         "Don't know what to do with timestamp %s" % (ts,))
    
  #   # Pack into Entry
  #   if entry is None:
  #     assert msg is not None, "No message to write"
  #     entry = Entry(
  #               topic=topic,
  #               timestamp=ts,
  #               msg=msg)

  #   self._write_entry(entry)

  # def _write_entry(self, entry):

  #   # Convert to native_entry
  #   if isinstance(entry.msg, StampedMessage):
  #     s_msg = entry.msg
  #   else:
  #     s_msg = StampedMessage()
  #     s_msg.timestamp.CopyFrom(entry.timestamp)
  #     s_msg.msg.Pack(entry.msg)
  #   assert s_msg is not None
  #   from protobag_native import native_entry
  #   nentry = native_entry()
  #   nentry.topic = entry.topic
  #   nentry.sec = s_msg.timestamp.seconds
  #   nentry.nanos = s_msg.timestamp.nanos
  #   nentry.type_url = s_msg.msg.type_url
  #   nentry.msg_bytes = s_msg.msg.value

  #   if not hasattr(self, '_native_writer'):
  #     from protobag_native import Writer
  #     self._native_writer = Writer()

  #     from protobag_native import WriterSpec
  #     s = WriterSpec()
  #     s.path = self._path
  #     s.format = "zip"
  #     s.save_index_index = True

  #     self._native_writer.start(s)
    
  #   writer = self._native_writer
  #   writer.write_entry(nentry)

  # def iter_entries(self, selection):
  #   iter_entries = _NativeReadSession(self.path, selection)
  #   for entry in iter_entries:
  #     yield entry
  

  # ## Support

  # class _NativeReadSession(object):
  #   def __init__(self, path):
  #     self._path = path
    
  #   @property
  #   def _reader(self):
      
  #     assert self._sel is not None
  #     if not hasattr(self, '_reader_impl'):
  #       pass
  #     return self._reader_impl

  #   def iter_entries(self, selection):
  #     from google.protobuf.any_pb2 import Any

  #     assert self._path is not None
  #     sel_bytes = selection
  #     if isinstance(sel_bytes, Selection):
  #       sel_bytes = sel_bytes.SerializeToString()

  #     from protobag_native import Reader
  #     reader = Reader()
  #     reader.start(path, sel_bytes)

  #     for nentry in reader:
  #       entry = Entry(
  #                 topic=nentry.topic,
  #                 timestamp=Timestamp(
  #                   seconds=nentry.sec,
  #                   nanos=nentry.nanos),
  #                 msg=StampedMessage(
  #                   timestamp=Timestamp(
  #                     seconds=nentry.sec,
  #                     nanos=nentry.nanos),
  #                   msg=Any(
  #                     type_url=nentry.type_url,
  #                     value=nentry.msg_bytes)))
  #       yield entry
