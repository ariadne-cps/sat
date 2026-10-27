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

/*! \file sat_backend.hpp
 *  \brief Internal interface for interchangeable SAT solver backends.
 */

#ifndef ARIADNE_SAT_BACKEND_HPP
#define ARIADNE_SAT_BACKEND_HPP

#include <memory>

#include "sat/sat_solver.hpp"

namespace Ariadne {

//! Internal boundary shared by ROBDD and CDCL SAT implementations.
class SatBackend {
  public:
    virtual ~SatBackend() = default;
    virtual Void add_clause(SatClause const& clause) = 0;
    virtual Bool solve(BinaryWord& model) = 0;
    virtual SizeType bdd_node_count() const = 0;
};

std::unique_ptr<SatBackend> make_sat_backend(
    SizeType variable_count,SatBackendKind kind);

} // namespace Ariadne

#endif // ARIADNE_SAT_BACKEND_HPP
