#include <nanobind/nanobind.h>

namespace nb = nanobind;

NB_MODULE(textfoundry, m) {
  m.attr("__version__") = "0.2.7";
}
