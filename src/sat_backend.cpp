/***************************************************************************
 *            sat_backend.cpp
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

/*! \file sat_backend.cpp
 *  \brief BDD and CDCL backends for the public SAT solver.
 */

#include "sat_backend.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

#include "bdd/robdd.hpp"
#include "cdcl/cdcl_solver.hpp"

namespace Ariadne {
namespace {

//! Preserves the previous exact CNF-to-ROBDD implementation.
class BddSatBackend final : public SatBackend {
  public:
    explicit BddSatBackend(SizeType count)
        : _variable_count(count),
          _manager(),
          _formula(_manager.constant(true)) { }

    Void add_clause(SatClause const& clause) override
    {
        Robdd disjunction=_manager.constant(false);
        for(SatLiteral const& literal:clause) {
            Robdd variable=_manager.variable(literal.variable());
            Robdd term=literal.positive()
                ? variable : _manager.negation(variable);
            disjunction=_manager.disjunction(disjunction,term);
        }
        _formula=_manager.conjunction(_formula,disjunction);
    }

    Bool solve(BinaryWord& model) override
    {
        if(_formula.is_false()) {
            return false;
        }
        Robdd remaining=_formula;
        for(SizeType index=0u;index!=_variable_count;++index) {
            Robdd variable=_manager.variable(index);
            Robdd with_false=_manager.conjunction(
                remaining,_manager.negation(variable));
            if(not with_false.is_false()) {
                model.append(false);
                remaining=with_false;
            } else {
                remaining=_manager.conjunction(remaining,variable);
                model.append(true);
            }
        }
        return true;
    }

    SizeType bdd_node_count() const override
    {
        return _manager.node_count();
    }

  private:
    SizeType _variable_count;
    RobddManager _manager;
    Robdd _formula;
};

//! Accumulates CNF clauses; each solve uses a fresh single-shot CDCL search.
class CdclSatBackend final : public SatBackend {
  public:
    explicit CdclSatBackend(SizeType count) : _variable_count(count) { }

    Void add_clause(SatClause const& clause) override
    {
        BooleanClause converted;
        converted.reserve(clause.size());
        for(SatLiteral const& literal:clause) {
            converted.emplace_back(literal.variable(),literal.positive());
        }
        _clauses.push_back(std::move(converted));
    }

    Bool solve(BinaryWord& model) override
    {
        CdclSolver solver(_variable_count,_clauses);
        CdclSolveStatus status=solver.solve();
        if(status==CdclSolveStatus::UNSAT) {
            return false;
        }
        if(status!=CdclSolveStatus::SAT) {
            throw std::logic_error("Pure Boolean CDCL unexpectedly returned UNKNOWN");
        }
        CdclAssignment assignment=solver.assignment();
        if(assignment.size()!=_variable_count) {
            throw std::logic_error("CDCL returned an incomplete Boolean assignment");
        }
        for(auto const& value:assignment) {
            if(not value.has_value()) {
                throw std::logic_error("CDCL returned an unassigned model variable");
            }
            model.append(*value);
        }
        return true;
    }

    SizeType bdd_node_count() const override
    {
        throw std::logic_error("BDD node count unavailable for CDCL SAT backend");
    }

  private:
    SizeType _variable_count;
    std::vector<BooleanClause> _clauses;
};

} // namespace

std::unique_ptr<SatBackend> make_sat_backend(
    SizeType variable_count,SatBackendKind kind)
{
    switch(kind) {
        case SatBackendKind::BDD:
            return std::make_unique<BddSatBackend>(variable_count);
        case SatBackendKind::CDCL:
            return std::make_unique<CdclSatBackend>(variable_count);
        default:
            throw std::invalid_argument("Invalid SAT backend kind");
    }
}

} // namespace Ariadne
