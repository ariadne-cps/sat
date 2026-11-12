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

/** @file sat/sat_solver.hpp
 *  @brief Boolean literals, backend-selectable CNF solver and satisfiability results.
 */

#ifndef ARIADNE_SAT_SAT_SOLVER_HPP
#define ARIADNE_SAT_SAT_SOLVER_HPP

#include <memory>
#include <vector>

#include "utility/binary_word.hpp"
#include "utility/typedefs.hpp"

namespace Ariadne {

//! \brief A Boolean literal with zero-based variable numbering.
class SatLiteral {
  public:
    /** @brief Construct a literal for a zero-based variable index and polarity. */
    explicit SatLiteral(SizeType variable, Bool positive = true)
        : _variable(variable), _positive(positive) {}

    /** @brief Return the zero-based variable index. */
    SizeType variable() const { return _variable; }
    /** @brief Return true for an unnegated literal. */
    Bool positive() const { return _positive; }

  private:
    SizeType _variable;
    Bool _positive;
};

using SatClause = std::vector<SatLiteral>;

//! \brief Select the algorithm used by SatSolver.
//! \details CDCL is the default search engine; BDD remains an explicit alternative.
enum class SatBackendKind { BDD,
                            CDCL };

//! \brief Backend implementation detail, defined in the SAT source tree.
class SatBackend;

//! \brief Exact SAT/UNSAT result independent of the selected backend.
class SatResult {
  public:
    /** @brief Report whether a satisfying assignment exists. */
    Bool is_sat() const { return _satisfiable; }
    /** @brief Report whether the formula is unsatisfiable. */
    Bool is_unsat() const { return not _satisfiable; }
    /** @brief Report whether a satisfying model is available. */
    Bool has_model() const { return _satisfiable; }

    //! \brief Total model in variable-index order; only valid for SAT results.
    const BinaryWord& model() const;

  private:
    /** @brief Construct a solver result and its optional model. */
    SatResult(Bool satisfiable, BinaryWord model);

    Bool _satisfiable;
    BinaryWord _model;

    friend class SatSolver;
};

//! \brief Backend-selectable incremental CNF SAT solver.
//! \details Clauses persist across solve() calls, regardless of backend.
//! CDCL is the default; BDD builds a canonical ROBDD if selected explicitly.
//! CDCL constructs a fresh one-shot CdclSolver for each solve() call.
class SatSolver {
  public:
    /** @brief Construct a solver for the specified number of variables and backend. */
    explicit SatSolver(SizeType variable_count,
                       SatBackendKind backend = SatBackendKind::CDCL);
    /** @brief Destroy the solver and release its backend. */
    ~SatSolver();

    /** @brief Copy construction is disabled because the solver uniquely owns its backend. */
    SatSolver(const SatSolver&) = delete;
    /** @brief Copy assignment is disabled. */
    SatSolver& operator=(const SatSolver&) = delete;
    /** @brief Move-construct a solver and transfer its backend. */
    SatSolver(SatSolver&&) noexcept;
    /** @brief Move-assign a solver and transfer its backend. */
    SatSolver& operator=(SatSolver&&) noexcept;

    //! \brief Add a disjunction, with empty clause meaning contradiction.
    //! \throws std::out_of_range without modifying state for invalid variables.
    Void add_clause(const SatClause& clause);

    //! \brief Return SAT with a total model, or exact UNSAT.
    SatResult solve();

    /** @brief Return the configured number of Boolean variables. */
    SizeType variable_count() const { return _variable_count; }
    /** @brief Return the number of clauses added to the solver. */
    SizeType clause_count() const { return _clause_count; }
    /** @brief Return the selected SAT backend. */
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
