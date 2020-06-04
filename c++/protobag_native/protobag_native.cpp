#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>

#include <fmt/format.h>

#include <protobag/Protobag.hpp> // fixme includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <protobag/ReadSession.hpp>
#include <protobag/WriteSession.hpp>
#include <protobag/Utils/PBUtils.hpp>
#include <protobag_msg/ProtobagMsg.pb.h>

namespace py = pybind11;

using namespace protobag;

// A convenience SERDES to avoid pybind boxing of StampedMessage
struct native_entry final {
  std::string entryname;
  std::string type_url;
  py::bytes msg_bytes;

  std::string ctx_topic;
  int64_t ctx_sec = 0;
  int32_t ctx_nanos = 0;
  // TODO not msg descriptor pointer but all the FileDescriptorSet stuff BagIndexBuilder would need ~~~~~
  
  

  Entry AsEntry() const {
    if (type_url.empty()) {

      return Entry::CreateRawFromBytes(entryname, msg_bytes);
    
    } else if (!ctx_topic.empty()) {

      return Entry::CreateStampedUnchecked(
        ctx_topic,
        ctx_sec,
        ctx_nanos,
        type_url,
        std::move(msg_bytes));  

    } else {

      return Entry::CreateUnchecked(
        entryname,
        type_url,
        std::move(msg_bytes),
        {}); // TODO add descriptor stuff for context ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    }
  }

  static native_entry FromEntry(const Entry &entry) {
    native_entry ne = {
      .entryname = entry.entryname,
      .type_url = entry.msg.type_url(),
      .msg_bytes = entry.msg.value(),
    };
    if (entry.ctx.has_value()) {
      ne.ctx_topic = entry.ctx->topic;
      ne.ctx_sec = entry.ctx->stamp.seconds();
      ne.ctx_nanos = entry.ctx->stamp.nanos();
      ne.type_url = entry.ctx->inner_type_url; // overrides msg.type_url()
      // TODO add descriptor context for decode ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    }
    return ne;
  }
};


class Reader final {
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
      throw std::runtime_error(maybe_rp.error);
    }

    _read_sess = *maybe_rp.value;
  }

  native_entry Next() {
    if (!_read_sess) {
      throw std::runtime_error("Invalid read session");
    }
    auto &reader = *_read_sess;

    auto maybe_entry = reader.GetNext();
    if (maybe_entry.IsEndOfSequence()) {
      throw pybind11::stop_iteration();
    } else if (!maybe_entry.IsOk()) {
      throw std::runtime_error(maybe_entry.error);
    }

    return native_entry::FromEntry(*maybe_entry.value);
  }

protected:
  ReadSession::Ptr _read_sess;
};


class Writer final {
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

  void WriteEntry(const native_entry &nentry) {
    if (!_write_sess) {
      throw std::runtime_error("Invalid write session");
    }

    auto &writer = *_write_sess;
    auto maybe_ok = writer.WriteEntry(nentry.AsEntry());
    if (!maybe_ok.IsOk()) {
      throw std::runtime_error(maybe_ok.error);
    }
  }

protected:
  WriteSession::Ptr _write_sess;
};


PYBIND11_MODULE(protobag_native, m) {
  m.doc() = 
    "protobag_native: a pybind11-based interface to the Protobag C++ back-end";

  py::class_<native_entry>(m, "native_entry", "Handle to a native entry")
    .def(py::init<>())
    .def_readwrite("entryname", &native_entry::entryname)
    .def_readwrite("type_url", &native_entry::type_url)
    .def_readwrite("msg_bytes", &native_entry::msg_bytes)
    .def_readwrite("ctx_topic", &native_entry::ctx_topic)
    .def_readwrite("ctx_sec", &native_entry::ctx_sec)
    .def_readwrite("ctx_nanos", &native_entry::ctx_nanos);

  py::class_<Reader>(m, "Reader", "Handle to a Protobag ReadSession")
    .def(py::init<>(), "Create a null session")
    .def("start", &Reader::Start, "Begin reading the given Selection")
    .def("__iter__", [](Reader &r) -> Reader& { return r; })
    .def("next", &Reader::Next, "Generator interface: emit the next entry")
    .def("__next__", &Reader::Next, "Generator interface: emit the next entry");

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

  py::class_<Writer>(m, "Writer", "Handle to a Protobag WriteSession")
    .def(py::init<>(), "Create a null session")
    .def("start", &Writer::Start, "Begin writing given a WriteSession::Spec")
    .def("__iter__", [](Reader &r) -> Reader& { return r; })
    .def("write_entry", &Writer::WriteEntry, "Write the given `native_entry`");
    
}
