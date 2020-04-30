#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <protobag/Protobag.hpp>

namespace py = pybind11;

PYBIND11_MODULE(protobag_native, m) {
  m.doc() = "pybind11 example plugin";

  m.def("foo", &protobag::foo, "yo momma");
}
