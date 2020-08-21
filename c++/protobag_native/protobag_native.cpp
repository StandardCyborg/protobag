/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>
#include <optional>

#include <fmt/format.h>

#include <google/protobuf/descriptor.h>

#include <protobag/Protobag.hpp>
#include <protobag/ReadSession.hpp>
#include <protobag/WriteSession.hpp>
#include <protobag/Utils/PBUtils.hpp>
#include <protobag/Utils/TimeSync.hpp>

#ifndef PROTOBAG_VERSION
#define PROTOBAG_VERSION "unknown"
#endif

namespace py = pybind11;

using namespace protobag;

// A convenience wrapper to avoid pybind conversion of Entry optional context
struct native_entry final {
  std::string entryname;
  std::string type_url;
  py::bytes msg_bytes;

  bool is_stamped = false;
  std::string topic;
  int64_t sec;
  int32_t nanos;

  // NB: Don't need descriptor context on decode because python Protobag
  // handles the index directly.  FMI see `DynamicMessageFactory`.
  
  static native_entry FromEntry(const Entry &entry) {
    if (entry.IsStampedMessage()) {

      auto maybe_unpacked = entry.UnpackFromStamped();
      if (!maybe_unpacked.IsOk()) {
        throw std::runtime_error(fmt::format(
          "Failed to unpack stamped entry for entry {}: {}",
          entry.entryname,
          maybe_unpacked.error));
      }

      if (!maybe_unpacked.value->ctx.has_value()) {
        throw std::runtime_error(fmt::format(
          "Incorrect unpacking for stamped entry for entry {}",
          entry.entryname));
      }

      const Entry &unpacked = *maybe_unpacked.value;
      return {
        .entryname = entry.entryname,
        .type_url = unpacked.ctx->inner_type_url,
        .msg_bytes = unpacked.msg.value(),
        
        .is_stamped = true,
        .topic = unpacked.ctx->topic,
        .sec = unpacked.ctx->stamp.seconds(),
        .nanos = unpacked.ctx->stamp.nanos(),
      };

    } else {

      return {
        .entryname = entry.entryname,
        .type_url = entry.msg.type_url(),
        .msg_bytes = entry.msg.value(),
      };
    }
  }
};



class PyReader final {
public:
  void Start(const std::string &path, const std::string &sel_pb_bytes) {
    auto maybe_sel = PBFactory::LoadFromContainer<Selection>(sel_pb_bytes);
    if (!maybe_sel.IsOk()) {
      throw std::invalid_argument(
        fmt::format(
          "Failed to decode a Selection; expected a protobuf Selection "
          "message in either binary or text_format form. Error {}",
          maybe_sel.error));
    }

    const Selection &sel = *maybe_sel.value;
    auto maybe_rp = ReadSession::Create({
      .archive_spec = {
        .path = path,
        .mode = "read",
      },
      .selection = sel,
    });
    if (!maybe_rp.IsOk()) {
      throw std::runtime_error(
        fmt::format("Failed to start reader: {}", maybe_rp.error));
    }

    _read_sess = *maybe_rp.value;
  }

  std::optional<native_entry> GetNext() {
    if (!_read_sess) {
      throw std::runtime_error("Invalid read session");
    }
    auto &reader = *_read_sess;

    auto maybe_entry = reader.GetNext();
    if (maybe_entry.IsEndOfSequence() || maybe_entry.IsNotFound()) {
      // NB: We use this exception instead of pybind11::stop_iteration due
      // to a bug in pybind related to libc++.  FMI see:
      // * https://gitter.im/pybind/Lobby?at=5f18cfc9361e295cf01fd21a
      // * (This fix appears to still have a bug)
      //      https://github.com/pybind/pybind11/pull/949
      return std::nullopt;

    } else if (!maybe_entry.IsOk()) {
      throw std::runtime_error(maybe_entry.error);
    }

    return native_entry::FromEntry(*maybe_entry.value);
  }

  static py::bytes GetIndex(const std::string &path) {
    auto maybe_index = ReadSession::GetIndex(path);
    if (!maybe_index.IsOk()) {
      throw std::runtime_error(
        fmt::format("Failed to read index from {}: {}",
          path, maybe_index.error));
    }
    
    // We return a string-serialized Protobuf message to avoid having
    // to translate the BagIndex message class through pybind11
    auto maybe_str = PBFactory::ToBinaryString(*maybe_index.value);
    if (!maybe_str.IsOk()) {
      throw std::runtime_error(
        fmt::format("Failed to re-encode index {}: {}",
          path, maybe_str.error));
    }

    return *maybe_str.value;
  }

  static std::vector<std::string> GetAllTopics(const std::string &path) {
    auto maybe_topics = ReadSession::GetAllTopics(path);
    if (!maybe_topics.IsOk()) {
      throw std::runtime_error(
        fmt::format("Failed to read topics from {}: {}",
          path, maybe_topics.error));
    }
    return *maybe_topics.value;
  }

  ReadSession::Ptr GetSession() const { return _read_sess; }

protected:
  ReadSession::Ptr _read_sess;
};



class PyTimeSyncBase {
public:
  virtual ~PyTimeSyncBase() { }
  virtual std::optional<std::list<native_entry>> GetNext() {
    if (!_sync) {
      throw std::runtime_error("Invalid synchronizer");
    }

    MaybeBundle next = _sync->GetNext();
    if (next.IsEndOfSequence()) {
      // NB: We use this exception instead of pybind11::stop_iteration due
      // to a bug in pybind related to libc++.  FMI see:
      // * https://gitter.im/pybind/Lobby?at=5f18cfc9361e295cf01fd21a
      // * (This fix appears to still have a bug)
      //      https://github.com/pybind/pybind11/pull/949
      return std::nullopt;

    } else if (!next.IsOk()) {
      throw std::runtime_error(next.error);
    }

    std::list<native_entry> nbundle;
    for (const auto &entry : *next.value) {
      nbundle.push_back(native_entry::FromEntry(entry));
    }
    return nbundle;
  }

protected:
  TimeSync::Ptr _sync;
};

class PyMaxSlopTimeSync : public PyTimeSyncBase {
public:
  void Start(
    const PyReader &reader,
    const MaxSlopTimeSync::Spec &spec) {

      auto read_sess = reader.GetSession();
      if (!read_sess) {
        throw std::runtime_error("Invalid read session");
      }

      _spec = spec;
      auto maybe_sync = MaxSlopTimeSync::Create(read_sess, spec);
      if (!maybe_sync.IsOk()) {
        throw std::runtime_error(fmt::format(
          "Failed to create MaxSlopTimeSync: {}", maybe_sync.error));
      }

      _sync = *maybe_sync.value;
  }

  MaxSlopTimeSync::Spec GetSpec() const { return _spec; }

protected:
  MaxSlopTimeSync::Spec _spec;
};



class PyWriter final {
public:
  void Start(WriteSession::Spec s) {
    s.archive_spec.mode = "write";
    auto maybe_w = WriteSession::Create(s);
    if (!maybe_w.IsOk()) {
      throw std::invalid_argument(
        fmt::format("Failed to create write session. Error {}", maybe_w.error));
    }
    _write_sess = *maybe_w.value;
  }

  void Close() {
    if (_write_sess) {
      _write_sess->Close();
      _write_sess = nullptr;
    }
  }

  void WriteRaw(const std::string &entryname, const py::bytes &raw_bytes) {
    HasSessOrThrow();

    auto maybe_ok = _write_sess->WriteEntry(
      Entry::CreateRawFromBytes(
        entryname,
        raw_bytes));
    if (!maybe_ok.IsOk()) {
      throw std::runtime_error(maybe_ok.error);
    }
  
  }

  void WriteMsg(
          const std::string &entryname,
          const std::string &type_url,
          const py::bytes &msg_bytes,
          const std::optional<py::bytes> &fds_bytes) {
    HasSessOrThrow();

    auto maybe_fds = DecodeFDS(fds_bytes);

    auto maybe_ok = _write_sess->WriteEntry(
      Entry::CreateUnchecked(
        entryname,
        type_url,
        msg_bytes,
        {
          .fds = maybe_fds.has_value() ? &maybe_fds.value() : nullptr,
        }));
    if (!maybe_ok.IsOk()) {
      throw std::runtime_error(maybe_ok.error);
    }    
  }

  void WriteStampedMsg(
          const std::string &topic,
          int64_t sec,
          int32_t nanos,
          const std::string &type_url,
          const py::bytes &msg_bytes,
          const std::optional<py::bytes> &fds_bytes) {
    HasSessOrThrow();

    auto maybe_fds = DecodeFDS(fds_bytes);

    auto maybe_ok = _write_sess->WriteEntry(
      Entry::CreateStampedUnchecked(
        topic,
        sec,
        nanos,
        type_url,
        msg_bytes,
        /* fds = */ maybe_fds.has_value() ? &maybe_fds.value() : nullptr,
        /* descriptor = */ nullptr));
    if (!maybe_ok.IsOk()) {
      throw std::runtime_error(maybe_ok.error);
    }
  }

protected:
  WriteSession::Ptr _write_sess;

  void HasSessOrThrow() const {
    if (!_write_sess) {
      throw std::runtime_error("Programming error: null write session");
    }
  }

  static std::optional<::google::protobuf::FileDescriptorSet> DecodeFDS(
      const std::optional<py::bytes> &fds_bytes) {

    if (!fds_bytes.has_value() || fds_bytes->is_none()) {
      return std::nullopt;
    }

    auto maybe_fds = 
      PBFactory::LoadFromContainer<::google::protobuf::FileDescriptorSet>(
        std::string(*fds_bytes));
    if (!maybe_fds.IsOk()) {
      throw std::runtime_error(fmt::format(
        "Failed to decode FileDescriptorSet, error: {}}", maybe_fds.error));
    }
    return *maybe_fds.value;
  }
};


PYBIND11_MODULE(protobag_native, m) {
  m.doc() = 
    "protobag_native: a pybind11-based interface to the Protobag C++ back-end";

  m.def("get_version", []() { return std::string(PROTOBAG_VERSION);});


  /// native_entry
  py::class_<native_entry>(m, "native_entry", "Handle to a native entry")
    .def(py::init<>())
    .def_readwrite("entryname", &native_entry::entryname)
    .def_readwrite("type_url", &native_entry::type_url)
    .def_readwrite("msg_bytes", &native_entry::msg_bytes)
    .def_readwrite("is_stamped", &native_entry::is_stamped)
    .def_readwrite("topic", &native_entry::topic)
    .def_readwrite("sec", &native_entry::sec)
    .def_readwrite("nanos", &native_entry::nanos);


  /// Reading
  py::class_<PyReader>(m, "PyReader", "Handle to a Protobag ReadSession")
    .def(py::init<>(), "Create a null session")
    .def("start", &PyReader::Start, "Begin reading the given Selection")
    .def(
      "get_next",
      &PyReader::GetNext,
      "Get next item or None for end of sequence")
    .def_static(
      "get_index",
      &PyReader::GetIndex,
      "Get the (string-serialized) BagIndex for the bag at the given path")
    .def_static(
      "get_topics",
      &PyReader::GetAllTopics,
      "Get the list of topics (for any time-series data) in the bag "
      "at the given path");


  /// TimeSync
  py::class_<MaxSlopTimeSync::Spec>(
    m, "MaxSlopTimeSyncSpec", "Spec for a MaxSlopTimeSync")
    .def(py::init<>())
    .def_readwrite(
      "topics", &MaxSlopTimeSync::Spec::topics, "Synchronize these topics")
    .def("set_max_slop", 
      [](MaxSlopTimeSync::Spec &s, int64_t sec, int32_t nanos) { 
        s.max_slop.set_seconds(sec); s.max_slop.set_nanos(nanos);
      },
        py::arg("seconds"),
        py::arg("nanos"),
      "Discard any bundle with total time difference greater than `max_slop`")
    .def("get_max_slop",
      [](MaxSlopTimeSync::Spec &s) {
        py::dict d;
        d["seconds"] = s.max_slop.seconds();
        d["nanos"] = s.max_slop.nanos();
        return d;
      })
    .def_readwrite(
      "max_queue_size",
      &MaxSlopTimeSync::Spec::max_queue_size,
      "Buffer at most this many messages per topic");

  py::class_<PyMaxSlopTimeSync>(
    m, "PyMaxSlopTimeSync",
    "Approximately synchronize two or more StampedMessage topics using "
    "a max slop algorithm.  FMI see docs for `protobag::MaxSlopTimeSync`. "
    "Non-timestamped entries read during synchronization are dropped and "
    "ignored. ")
    .def(py::init<>())
    .def(
      "start", &PyMaxSlopTimeSync::Start,
      "Begin synchronizing the given reader")
    .def(
      "get_next",
      &PyMaxSlopTimeSync::GetNext,
      "Get next bundle or None for end of sequence");


  /// Writing
  py::class_<WriteSession::Spec>(m, "WriterSpec", "Spec for a WriteSession")
    .def(py::init<>())
    .def_readwrite(
      "save_timeseries_index", &WriteSession::Spec::save_timeseries_index)
    .def_readwrite(
      "save_descriptor_index", &WriteSession::Spec::save_descriptor_index)
    .def_property("path", 
      [](WriteSession::Spec &s) { return s.archive_spec.path; },
      [](WriteSession::Spec &s, const std::string &v) {
        s.archive_spec.path = v;
      },
      "Write to this path")
    .def_property("format", 
      [](WriteSession::Spec &s) { return s.archive_spec.format; },
      [](WriteSession::Spec &s, const std::string &v) {
        s.archive_spec.format = v;
      },
      "Write in this format");

  py::class_<PyWriter>(m, "PyWriter", "Handle to a Protobag WriteSession")
    .def(py::init<>(), "Create a null session")
    .def("start", &PyWriter::Start, "Begin writing given a WriteSession::Spec")
    .def("close", &PyWriter::Close, "End writing session")
    .def("write_raw", &PyWriter::WriteRaw, "Write the given raw bytes")
    .def(
      "write_msg",
      &PyWriter::WriteMsg,
        py::arg("entryname"),
        py::arg("type_url"),
        py::arg("msg_bytes"),
        py::arg("fds_bytes").none(true),
      "Write the given message")
    .def(
      "write_stamped_msg",
      &PyWriter::WriteStampedMsg,
        py::arg("topic"),
        py::arg("sec"),
        py::arg("nanos"),
        py::arg("type_url"),
        py::arg("msg_bytes"),
        py::arg("fds_bytes").none(true),
      "Write the given stamped message");

}
