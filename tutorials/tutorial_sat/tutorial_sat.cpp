/***************************************************************************
 *            tutorial_sat.cpp
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

/*! \file tutorial_sat.cpp
 *  \brief Standalone C++ tutorial for the SAT solver.
 */

#include <iostream>
#include "ariadne-sat.hpp"

using namespace Ariadne;

int main() {
    // CDCL is the default CNF search backend.
    SatSolver solver(2u);
    if(solver.backend_kind()!=SatBackendKind::CDCL) { return 1; }
    solver.add_clause({SatLiteral(0u),SatLiteral(1u)});
    solver.add_clause({SatLiteral(0u,false),SatLiteral(1u)});
    SatResult first=solver.solve();
    if(not first.is_sat() or first.model()!=make_binary_word("01")) {
        std::cerr << "Unexpected SAT result or witness.\n";
        return 1;
    }
    std::cout << "SAT, model: " << first.model() << "\n";
    solver.add_clause({SatLiteral(1u,false)});
    if(not solver.solve().is_unsat()) {
        std::cerr << "Expected UNSAT.\n";
        return 1;
    }
    std::cout << "UNSAT after adding NOT x1.\n";

    // Select BDD when a canonical representation and its statistics help.
    SatSolver symbolic(2u,SatBackendKind::BDD);
    symbolic.add_clause({SatLiteral(0u),SatLiteral(1u)});
    if(not symbolic.solve().is_sat()) { return 1; }
    std::cout << "BDD backend allocated nodes: "
              << symbolic.bdd_node_count() << "\n";
    return 0;
}
