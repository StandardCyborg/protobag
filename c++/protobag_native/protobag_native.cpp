#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>

#include <fmt/format.h>

#include <protobag/Protobag.hpp> // fixme ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <protobag/ReadSession.hpp>
#include <protobag/WriteSession.hpp>
#include <protobag/Utils/PBUtils.hpp>
#include <protobag_msg/ProtobagMsg.pb.h>

namespace py = pybind11;

using namespace protobag;

// A convenience SERDES to avoid pybind boxing of StampedMessage
struct native_entry final {
  std::string topic;
  int64_t sec = 0;
  int32_t nanos = 0;
  std::string type_url;
  py::bytes msg_bytes;

  Entry AsEntry() const {
    StampedMessage stamped_msg;
    *stamped_msg.mutable_msg()->mutable_type_url() = type_url;
    *stamped_msg.mutable_msg()->mutable_value() = msg_bytes;
    stamped_msg.mutable_timestamp()->set_seconds(sec);
    stamped_msg.mutable_timestamp()->set_nanos(nanos);
    return Entry{.topic = topic, .stamped_msg = stamped_msg};
  }

  static native_entry FromEntry(const Entry &entry) {
    return {
      .topic = entry.topic,
      .sec = entry.stamped_msg.timestamp().seconds(),
      .nanos = entry.stamped_msg.timestamp().nanos(),
      .type_url = entry.stamped_msg.msg().type_url(),
      .msg_bytes = entry.stamped_msg.msg().value(),
    };
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

  m.def("foo", &protobag::foo, "yo momma"); // fixme ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`

  py::class_<native_entry>(m, "native_entry", "Handle to a native entry")
    .def(py::init<>())
    .def_readwrite("topic", &native_entry::topic)
    .def_readwrite("sec", &native_entry::sec)
    .def_readwrite("nanos", &native_entry::nanos)
    .def_readwrite("type_url", &native_entry::type_url)
    .def_readwrite("msg_bytes", &native_entry::msg_bytes);

  py::class_<Reader>(m, "Reader", "Handle to a Protobag ReadSession")
    .def(py::init<>(), "Create a null session")
    .def("start", &Reader::Start, "Begin reading the given Selection")
    .def("__iter__", [](Reader &r) -> Reader& { return r; })
    .def("next", &Reader::Next, "Generator interface: emit the next entry")
    .def("__next__", &Reader::Next, "Generator interface: emit the next entry");

  py::class_<WriteSession::Spec>(m, "WriterSpec", "Spec for a WriteSession")
    .def(py::init<>())
    .def_readwrite("save_index_index", &WriteSession::Spec::save_index_index)
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
    



  // py::class_<Protobag>(m, "Protobag", "Handle to a single Protobag archive")
  //   .def(py::init<>(),
  //         "Create a null protobag")
  //   .def(py::init<const std::string &>(),
  //         "Read or write a Protobag to the given path",
  //         py::arg("path"))
  //   .def("read_entries", 
  //     [](const Protobag &bag, const std::string &sel_pb_bytes) {
          

  //         return read_sess_generator{.r = *maybe_rp.value};
  //     },
  //     "Create & return a generator that emits entries for the given Selection",
  //     py::arg("selection"));

  // py::class_<read_sess_generator>(
  //   m, "read_sess_generator", "A generator emitting entries in a Protobag")
    
  //   .def(py::init<>())
  //   .def("next", &read_sess_generator::next);

}
