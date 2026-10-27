#include "pybind11.hpp"

void sat_submodule(pybind11::module& module);

void sat_module(pybind11::module& module) {
    sat_submodule(module);
}
