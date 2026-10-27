#include <chrono>
#include <iostream>
#include <stdexcept>
#include "sat/sat_solver.hpp"

using namespace Ariadne;

namespace {
SizeType index(SizeType holes, SizeType pigeon, SizeType hole) {
    return pigeon*holes+hole;
}
Void add_problem(SatSolver& solver, SizeType pigeons, SizeType holes) {
    for(SizeType pigeon=0u; pigeon!=pigeons; ++pigeon) {
        SatClause at_least_one;
        for(SizeType hole=0u; hole!=holes; ++hole) {
            at_least_one.emplace_back(index(holes,pigeon,hole));
            for(SizeType other=hole+1u; other!=holes; ++other) {
                solver.add_clause({
                    SatLiteral(index(holes,pigeon,hole),false),
                    SatLiteral(index(holes,pigeon,other),false)
                });
            }
        }
        solver.add_clause(at_least_one);
    }
    for(SizeType hole=0u; hole!=holes; ++hole) {
        for(SizeType p=0u; p!=pigeons; ++p) {
            for(SizeType q=p+1u; q!=pigeons; ++q) {
                solver.add_clause({
                    SatLiteral(index(holes,p,hole),false),
                    SatLiteral(index(holes,q,hole),false)
                });
            }
        }
    }
}
Void benchmark(SizeType pigeons, SizeType holes, Bool expected_sat) {
    SatSolver solver(pigeons*holes);
    auto start=std::chrono::steady_clock::now();
    add_problem(solver,pigeons,holes);
    SatResult result=solver.solve();
    auto finish=std::chrono::steady_clock::now();
    if(result.is_sat()!=expected_sat) {
        throw std::runtime_error("Incorrect pigeonhole result");
    }
    double ms=std::chrono::duration<double,std::milli>(finish-start).count();
    std::cout << pigeons << "\t" << holes << "\t"
              << (result.is_sat() ? "SAT" : "UNSAT") << "\t"
              << solver.bdd_node_count() << "\t" << ms << "\n";
}
}
int main() {
    std::cout << "pigeons\tholes\tresult\tallocated_bdd_nodes\ttime_ms\n";
    for(SizeType holes=1u; holes<=3u; ++holes) {
        benchmark(holes,holes,true);
        benchmark(holes+1u,holes,false);
    }
    return 0;
}
