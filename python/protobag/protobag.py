import datetime

import attr

from google.protobuf.timestamp_pb2 import Timestamp

from protobag.ProtobagMsg_pb2 import Selection
from protobag.ProtobagMsg_pb2 import StampedMessage
from protobag.ProtobagMsg_pb2 import BagMeta


from protobag.ProtobagMsg_pb2 import StdMsg
DEFAULT_MSG_TYPES = (
    StdMsg.Bool,
    StdMsg.Int,
    StdMsg.Float,
    StdMsg.String,
    StdMsg.Bytes,
)

@attr.s(slots=True, eq=True, weakref_slot=False)
class Entry(object):
  topic = attr.ib(type=str, default="")
  """str: The topic for this entry, e.g. `/camera_front/image`"""
  
  timestamp = attr.ib(type=Timestamp, default=Timestamp())
  """datetime: Nanosecond-precision timestamp for this entry"""
  
  msg = attr.ib(default=None)
  """A protobuf message; might be `StampedMessage` or a user message type"""


class Protobag(object):

  ## Public API

  def __init__(self, path=None):
    """TODO
    """
  
    self._path = str(path or '')
  
  def write_entry(self, entry=None, topic=None, ts=None, msg=None):
    """TODO Docs
    
    Args:
      entry (Entry): Write this `Entry` instance
      topic (str): Create and write an entry with this topic
      ts (int, float, datetime, protobuf.Timestamp): Create and 
        write an entry with this timestamp.  Int and float values interpreted
        as unix epoch seconds.
      msg (Any): A protobuf message
    
    """

    # Normalize timestamp
    if ts is not None:
      if isinstance(ts, datetime.datetime):
        t = ts
        ts = Timestamp()
        ts.FromDatetime(t)
      elif isinstance(ts, int):
        t = ts
        ts = Timestamp()
        ts.FromSeconds(t)
      elif isinstance(ts, float):
        sec = int(ts)
        nsec = int((ts - sec) * 1e9)
        ts = Timestamp(seconds=sec, nanos=nsec)
      else:
        raise ValueError(
          "Don't know what to do with timestamp %s" % (ts,))
    
    # Pack into Entry
    if entry is None:
      assert msg is not None, "No message to write"
      entry = Entry(
                topic=topic,
                timestamp=ts,
                msg=msg)

    self._write_entry(entry)

  def _write_entry(self, entry):

    # Convert to native_entry
    if isinstance(entry.msg, StampedMessage):
      s_msg = entry.msg
    else:
      s_msg = StampedMessage()
      s_msg.timestamp.CopyFrom(entry.timestamp)
      s_msg.msg.Pack(entry.msg)
    assert s_msg is not None
    from protobag_native import native_entry
    nentry = native_entry()
    nentry.topic = entry.topic
    nentry.sec = s_msg.timestamp.seconds
    nentry.nanos = s_msg.timestamp.nanos
    nentry.type_url = s_msg.msg.type_url
    nentry.msg_bytes = s_msg.msg.value

    if not hasattr(self, '_native_writer'):
      from protobag_native import Writer
      self._native_writer = Writer()

      from protobag_native import WriterSpec
      s = WriterSpec()
      s.path = self._path
      s.format = "zip"
      s.save_meta_index = True

      self._native_writer.start(s)
    
    writer = self._native_writer
    writer.write_entry(nentry)

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
