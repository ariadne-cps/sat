/***************************************************************************
 *            sat_backend.hpp
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

/** @file src/sat_backend.hpp
 *  @brief Internal interface and factory for interchangeable SAT solver backends.
 */

#ifndef ARIADNE_SAT_BACKEND_HPP
#define ARIADNE_SAT_BACKEND_HPP

#include <memory>

#include "sat/sat_solver.hpp"

namespace Ariadne {

/** @brief Abstract interface implemented by the interchangeable SAT backends. */
class SatBackend {
  public:
    /** @brief Destroy a concrete SAT backend. */
    virtual ~SatBackend() = default;
    /** @brief Append a CNF clause to the backend. */
    virtual Void add_clause(SatClause const& clause) = 0;
    /** @brief Solve the current formula and write a model when satisfiable. */
    virtual Bool solve(BinaryWord& model) = 0;
    /** @brief Return the BDD node count, when supported by the backend. */
    virtual SizeType bdd_node_count() const = 0;
};

/** @brief Construct a backend of the requested type for a given number of variables. */
std::unique_ptr<SatBackend> make_sat_backend(
    SizeType variable_count,SatBackendKind kind);

} // namespace Ariadne

#endif // ARIADNE_SAT_BACKEND_HPP
