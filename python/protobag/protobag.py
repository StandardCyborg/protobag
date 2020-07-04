import datetime

from google.protobuf.timestamp_pb2 import Timestamp

from protobag_native import native_entry
from protobag.ProtobagMsg_pb2 import Selection
from protobag.ProtobagMsg_pb2 import StampedMessage
from protobag.ProtobagMsg_pb2 import BagIndex
from protobag.ProtobagMsg_pb2 import StdMsg

DEFAULT_MSG_TYPES = (
    StdMsg.Bool,
    StdMsg.Int,
    StdMsg.Float,
    StdMsg.String,
    StdMsg.Bytes,
    StdMsg.SSMap,
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
      raise KeyError()

_DefaultPBDecoder = PBDecoder.create_with_types(DEFAULT_MSG_TYPES)



class PBEntry(object):
  """A single entry in a protobag; analogous to a C++ `protobag::Entry`"""

  __slots__ = ['_nentry', '_pb_msg', '_pb_decoder']

  def __init__(self, nentry=None, pb_decoder=None):
    self._nentry = nentry or native_entry()
    self._pb_decoder = pb_decoder or _DefaultPBDecoder
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
      self._pb_msg = self._pb_decoder.decode(self.type_url, self.raw_msg_bytes)
        # TODO decode stamped
    return self._pb_msg

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


class Protobag(object):

  ## Public API

  def __init__(self, path=None):
    """TODO
    """
  
    self._path = str(path or '')
  
  def iter_entries(self):
    pass
  
  def get_entry(self, entryname):
    pass

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

  def iter_entries(self, selection):
    iter_entries = _NativeReadSession(self.path, selection)
    for entry in iter_entries:
      yield entry
  

  ## Support

  class _NativeReadSession(object):
    def __init__(self, path):
      self._path = path
    
    @property
    def _reader(self):
      
      assert self._sel is not None
      if not hasattr(self, '_reader_impl'):
        pass
      return self._reader_impl

    def iter_entries(self, selection):
      from google.protobuf.any_pb2 import Any

      assert self._path is not None
      sel_bytes = selection
      if isinstance(sel_bytes, Selection):
        sel_bytes = sel_bytes.SerializeToString()

      from protobag_native import Reader
      reader = Reader()
      reader.start(path, sel_bytes)

      for nentry in reader:
        entry = Entry(
                  topic=nentry.topic,
                  timestamp=Timestamp(
                    seconds=nentry.sec,
                    nanos=nentry.nanos),
                  msg=StampedMessage(
                    timestamp=Timestamp(
                      seconds=nentry.sec,
                      nanos=nentry.nanos),
                    msg=Any(
                      type_url=nentry.type_url,
                      value=nentry.msg_bytes)))
        yield entry
