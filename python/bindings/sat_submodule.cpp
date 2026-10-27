#include "pybind11.hpp"
#include "sat/sat_solver.hpp"

using namespace Ariadne;

void sat_submodule(pybind11::module& module) {
    pybind11::class_<SatLiteral>(module,"SatLiteral")
        .def(pybind11::init<SizeType,Bool>(),
             pybind11::arg("variable"),pybind11::arg("positive")=true)
        .def("variable",&SatLiteral::variable)
        .def("positive",&SatLiteral::positive);

    pybind11::class_<SatResult>(module,"SatResult")
        .def("is_sat",&SatResult::is_sat)
        .def("is_unsat",&SatResult::is_unsat)
        .def("has_model",&SatResult::has_model)
        .def("model",&SatResult::model,pybind11::return_value_policy::copy);

    pybind11::class_<SatSolver>(module,"SatSolver")
        .def(pybind11::init<SizeType>(),pybind11::arg("variable_count"))
        .def("add_clause",&SatSolver::add_clause)
        .def("solve",&SatSolver::solve)
        .def("variable_count",&SatSolver::variable_count)
        .def("clause_count",&SatSolver::clause_count)
        .def("bdd_node_count",&SatSolver::bdd_node_count);
}
