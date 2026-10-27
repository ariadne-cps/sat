/***************************************************************************
 *            sat_submodule.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne SAT.
 *
 *  Ariadne SAT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne SAT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne SAT.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! \file sat_submodule.cpp
 *  \brief Python bindings for Boolean literals and the SAT solver.
 */

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

    pybind11::enum_<SatBackendKind>(module,"SatBackendKind")
        .value("BDD",SatBackendKind::BDD)
        .value("CDCL",SatBackendKind::CDCL);

    pybind11::class_<SatSolver>(module,"SatSolver")
        .def(pybind11::init<SizeType,SatBackendKind>(),
             pybind11::arg("variable_count"),
             pybind11::arg("backend")=SatBackendKind::BDD)
        .def("add_clause",&SatSolver::add_clause)
        .def("solve",&SatSolver::solve)
        .def("variable_count",&SatSolver::variable_count)
        .def("clause_count",&SatSolver::clause_count)
        .def("backend_kind",&SatSolver::backend_kind)
        .def("bdd_node_count",&SatSolver::bdd_node_count);
}
