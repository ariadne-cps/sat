/***************************************************************************
 *            sat/sat_solver.hpp
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

/*! \file sat/sat_solver.hpp
 *  \brief Boolean literals, solver interface, and SAT results.
 */

#ifndef ARIADNE_SAT_SAT_SOLVER_HPP
#define ARIADNE_SAT_SAT_SOLVER_HPP

#include <vector>
#include <memory>

#include "utility/typedefs.hpp"
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

//! \brief Select the algorithm used by SatSolver.
//! \details BDD remains the default to preserve existing API behavior.
enum class SatBackendKind { BDD, CDCL };

//! \brief Backend implementation detail, defined in the SAT source tree.
class SatBackend;

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

//! \brief Backend-selectable incremental CNF SAT solver.
//! \details Clauses persist across solve() calls, regardless of backend.
//! The BDD backend builds a canonical ROBDD. CDCL constructs a fresh
//! one-shot CdclSolver at each solve() from the accumulated clauses.
class SatSolver {
  public:
    explicit SatSolver(SizeType variable_count,
                       SatBackendKind backend = SatBackendKind::BDD);
    ~SatSolver();

    SatSolver(SatSolver const&) = delete;
    SatSolver& operator=(SatSolver const&) = delete;
    SatSolver(SatSolver&&) noexcept;
    SatSolver& operator=(SatSolver&&) noexcept;

    //! \brief Add a disjunction, with empty clause meaning contradiction.
    //! \throws std::out_of_range without modifying state for invalid variables.
    Void add_clause(SatClause const& clause);

    //! \brief Return SAT with a total model, or exact UNSAT.
    SatResult solve();

    SizeType variable_count() const { return _variable_count; }
    SizeType clause_count() const { return _clause_count; }
    SatBackendKind backend_kind() const { return _backend_kind; }

    //! \brief Number of BDD nodes, including intermediate nodes.
    //! \throws std::logic_error if the selected backend is not BDD.
    SizeType bdd_node_count() const;

  private:
    SizeType _variable_count;
    SizeType _clause_count = 0u;
    SatBackendKind _backend_kind;
    std::unique_ptr<SatBackend> _backend;
};

} // namespace Ariadne

#endif // ARIADNE_SAT_SAT_SOLVER_HPP
