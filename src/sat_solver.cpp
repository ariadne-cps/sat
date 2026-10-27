/***************************************************************************
 *            sat_solver.cpp
 *
 *  Copyright  2026  Luca Geretti
 ****************************************************************************/

/*
 *  This file is part of Ariadne SAT.
 *
 *  Ariadne SAT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <stdexcept>
#include <utility>

#include "sat/sat_solver.hpp"

namespace Ariadne {

SatResult::SatResult(Bool satisfiable, BinaryWord model)
    : _satisfiable(satisfiable), _model(std::move(model)) { }

BinaryWord const& SatResult::model() const {
    if(not _satisfiable) {
        throw std::logic_error("UNSAT results have no model");
    }
    return _model;
}

SatSolver::SatSolver(SizeType variable_count)
    : _variable_count(variable_count),
      _manager(),
      _formula(_manager.constant(true)) { }

Void SatSolver::add_clause(SatClause const& clause) {
    // Validate the entire clause before changing the formula.
    for(auto const& literal : clause) {
        if(literal.variable() >= _variable_count) {
            throw std::out_of_range("SAT literal variable exceeds declared variable count");
        }
    }

    Robdd disjunction = _manager.constant(false);
    for(auto const& literal : clause) {
        Robdd variable = _manager.variable(literal.variable());
        Robdd term = literal.positive() ? variable : _manager.negation(variable);
        disjunction = _manager.disjunction(disjunction,term);
    }
    _formula = _manager.conjunction(_formula,disjunction);
    ++_clause_count;
}

SatResult SatSolver::solve() {
    if(_formula.is_false()) {
        return SatResult(false,BinaryWord());
    }

    Robdd remaining = _formula;
    BinaryWord model;
    for(SizeType index = 0u; index != _variable_count; ++index) {
        Robdd variable = _manager.variable(index);
        Robdd with_false = _manager.conjunction(
            remaining,_manager.negation(variable));
        if(not with_false.is_false()) {
            model.append(false);
            remaining = with_false;
        } else {
            // Satisfiability of remaining implies this branch is feasible.
            remaining = _manager.conjunction(remaining,variable);
            model.append(true);
        }
    }
    return SatResult(true,std::move(model));
}

} // namespace Ariadne
