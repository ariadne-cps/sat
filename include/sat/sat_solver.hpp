/***************************************************************************
 *            sat_solver.hpp
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

#ifndef ARIADNE_SAT_SAT_SOLVER_HPP
#define ARIADNE_SAT_SAT_SOLVER_HPP

#include <vector>

#include "bdd/robdd.hpp"
#include "utility/binary_word.hpp"

namespace Ariadne {

//! \brief A Boolean literal with zero-based variable numbering.
class SatLiteral {
  public:
    explicit SatLiteral(SizeType variable, Bool positive=true)
        : _variable(variable), _positive(positive) { }

    SizeType variable() const { return _variable; }
    Bool positive() const { return _positive; }

  private:
    SizeType _variable;
    Bool _positive;
};

using SatClause = std::vector<SatLiteral>;

//! \brief Exact outcome of an ROBDD-backed propositional SAT query.
class SatResult {
  public:
    Bool is_sat() const { return _satisfiable; }
    Bool is_unsat() const { return not _satisfiable; }
    Bool has_model() const { return _satisfiable; }

    //! \brief Total model in variable-index order; only valid for SAT results.
    BinaryWord const& model() const;

  private:
    SatResult(Bool satisfiable, BinaryWord model);

    Bool _satisfiable;
    BinaryWord _model;

    friend class SatSolver;
};

//! \brief Minimal incremental CNF solver using a canonical ROBDD.
//! \details Clauses persist across solve() calls. Variables are numbered
//! from 0 to variable_count()-1. This is exact, but BDD size may be exponential.
class SatSolver {
  public:
    explicit SatSolver(SizeType variable_count);

    //! \brief Add a disjunction of literals; an empty clause forces UNSAT.
    //! \throws std::out_of_range if any literal uses an undeclared variable.
    Void add_clause(SatClause const& clause);

    //! \brief Return SAT and a total witness, or UNSAT.
    SatResult solve();

    SizeType variable_count() const { return _variable_count; }
    SizeType clause_count() const { return _clause_count; }
    //! \brief Number of allocated BDD nodes (includes intermediate nodes).
    SizeType bdd_node_count() const { return _manager.node_count(); }

  private:
    SizeType _variable_count;
    SizeType _clause_count = 0u;
    RobddManager _manager;
    Robdd _formula;
};

} // namespace Ariadne

#endif // ARIADNE_SAT_SAT_SOLVER_HPP
