#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <protobag/Protobag.hpp>

namespace py = pybind11;

using namespace protobag;

PYBIND11_MODULE(protobag_native, m) {
  m.doc() = 
    "protobag_native: a pybind11-based interface to the Protobag C++ back-end";

  m.def("foo", &protobag::foo, "yo momma");

  // py::class_<ReadSession>(
  //   m, "ReadSession", "This class represents a pointcloud or triangle mesh")

  //     .def(py::init<>(), "Create an empty geometry, with no faces nor vertices.")

  //     .def("vertexCount", &Geometry::vertexCount, "The number of vertices of this geometry.")


}
