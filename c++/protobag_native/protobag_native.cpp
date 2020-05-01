#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>

#include <fmt/format.h>

#include <protobag/Protobag.hpp>
#include <protobag/Utils/PBUtils.hpp>
#include <protobag_msg/ProtobagMsg.pb.h>

namespace py = pybind11;

using namespace protobag;

// A convenience SERDES to avoid pybind boxing of StampedMessage
struct entry {
  std::string topic;
  int64_t sec = 0;
  int32_t nanos = 0;
  std::string type_url;
  std::string msg_bytes;

  Entry AsEntry() {
    StampedMessage stamped_msg;
    *stamped_msg.mutable_msg()->mutable_type_url() = type_url;
    *stamped_msg.mutable_msg()->mutable_value() = msg_bytes;
    stamped_msg.mutable_timestamp()->set_seconds(sec);
    stamped_msg.mutable_timestamp()->set_nanos(nanos);
    return Entry{.topic = topic, .stamped_msg = stamped_msg};
  }

  static entry FromEntry(const Entry &entry) {
    return {
      .topic = entry.topic,
      .sec = entry.stamped_msg.timestamp().seconds(),
      .nanos = entry.stamped_msg.timestamp().nanos(),
      .type_url = entry.stamped_msg.msg().type_url(),
      .msg_bytes = entry.stamped_msg.msg().value(),
    };
  }
};

struct read_sess_generator final {
  ReadSession::Ptr r;

  entry next() {
    if (!r) {
      throw std::runtime_error("Invalid read session");
    }
    auto &reader = *r;

    auto maybe_entry = reader.GetNext();
    if (maybe_entry.IsEndOfSequence()) {
      throw pybind11::stop_iteration();
    }

    if (!maybe_entry.IsOk()) {
      throw std::runtime_error(maybe_entry.error);
    }

    return entry::FromEntry(*maybe_entry.value);
  }
};

PYBIND11_MODULE(protobag_native, m) {
  m.doc() = 
    "protobag_native: a pybind11-based interface to the Protobag C++ back-end";

  m.def("foo", &protobag::foo, "yo momma"); // fixme ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`

  py::class_<Protobag>(m, "Protobag", "Handle to a single Protobag archive")
    .def(py::init<>(),
          "Create a null protobag")
    .def(py::init<const std::string &>(),
          "Read or write a Protobag to the given path",
          py::arg("path"))
    .def("read_entries", 
      [](const Protobag &bag, const std::string &sel_pb_bytes) {
          auto maybe_sel = 
            PBFactory::LoadFromContainer<Selection>(sel_pb_bytes);
          if (!maybe_sel.IsOk()) {
            throw std::invalid_argument(
              fmt::format(
                "Failed to decode a Selection; expected a protobuf Selection "
                "message in either binary or text_format form. Error {}",
                maybe_sel.error));
          }

          auto maybe_rp = bag.ReadEntries(*maybe_sel.value);
          if (!maybe_rp.IsOk()) {
            throw std::runtime_error(maybe_rp.error);
          }

          return read_sess_generator{.r = *maybe_rp.value};
      },
      "Create & return a generator that emits entries for the given Selection",
      py::arg("selection"));

  py::class_<read_sess_generator>(
    m, "read_sess_generator", "A generator emitting entries in a Protobag")
    
    .def(py::init<>())
    .def("next", &read_sess_generator::next);

}
