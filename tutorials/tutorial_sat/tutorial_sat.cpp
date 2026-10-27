#include <iostream>
#include "ariadne-sat.hpp"

using namespace Ariadne;

int main() {
    SatSolver solver(2u);
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
    return 0;
}
