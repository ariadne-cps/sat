/***************************************************************************
 *            sat/sat_solver.cpp
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

/*! \file sat/sat_solver.cpp
 *  \brief Implementation of the Boolean satisfiability solver.
 */

#include <stdexcept>
#include <utility>

#include "sat/sat_solver.hpp"
#include "sat_backend.hpp"

namespace Ariadne {

SatResult::SatResult(Bool satisfiable, BinaryWord model)
    : _satisfiable(satisfiable), _model(std::move(model)) { }

BinaryWord const& SatResult::model() const {
    if(not _satisfiable) {
        throw std::logic_error("UNSAT results have no model");
    }
    return _model;
}

SatSolver::SatSolver(SizeType variable_count,SatBackendKind backend)
    : _variable_count(variable_count),
      _backend_kind(backend),
      _backend(make_sat_backend(variable_count,backend)) { }

SatSolver::~SatSolver() = default;
SatSolver::SatSolver(SatSolver&&) noexcept = default;
SatSolver& SatSolver::operator=(SatSolver&&) noexcept = default;

Void SatSolver::add_clause(SatClause const& clause)
{
    // Validate all literals before mutating the selected backend.
    for(SatLiteral const& literal:clause) {
        if(literal.variable()>=_variable_count) {
            throw std::out_of_range("SAT literal variable exceeds declared variable count");
        }
    }
    _backend->add_clause(clause);
    ++_clause_count;
}

SatResult SatSolver::solve()
{
    BinaryWord model;
    Bool satisfiable=_backend->solve(model);
    return SatResult(satisfiable,std::move(model));
}

SizeType SatSolver::bdd_node_count() const
{
    if(_backend_kind!=SatBackendKind::BDD) {
        throw std::logic_error("BDD node count only available for BDD SAT backend");
    }
    return _backend->bdd_node_count();
}

} // namespace Ariadne
