# Ariadne SAT

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Unix](https://github.com/ariadne-cps/sat/actions/workflows/unix.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/unix.yml)
[![Windows](https://github.com/ariadne-cps/sat/actions/workflows/win.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/win.yml)
[![Sanitizers](https://github.com/ariadne-cps/sat/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/sanitizers.yml)
[![Coverage](https://github.com/ariadne-cps/sat/actions/workflows/coverage.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/coverage.yml)

Ariadne SAT is a minimal C++20 ROBDD-backed propositional satisfiability solver.
It translates CNF clauses to BDDs and obtains exact SAT/UNSAT answers by checking
the false terminal. SAT assignments are recovered using only existing BDD Boolean operations.

This is a correctness baseline, **not** a competitive CDCL solver. ROBDD sizes
can grow exponentially with the fixed variable ordering.

## Public API

- `SatLiteral(index,positive)`: zero-based variable index and optional polarity.
- `SatClause`: vector of literals; an empty clause is false.
- `SatSolver(number_of_variables)`: initially empty (true) CNF.
- `add_clause(clause)`: persistent incremental addition; validates all indices.
- `solve()`: exact SAT/UNSAT result. SAT results carry a total `BinaryWord` model.
- `model()`: throws `std::logic_error` for UNSAT.
- `variable_count()`, `clause_count()`, `bdd_node_count()`: statistics.

```cpp
#include "ariadne-sat.hpp"
using namespace Ariadne;
SatSolver solver(2);
solver.add_clause({SatLiteral(0),SatLiteral(1)});
solver.add_clause({SatLiteral(0,false),SatLiteral(1)});
SatResult result=solver.solve(); // SAT, model 01
```

## Dependencies

Direct submodules: `ss/configuration` and `ss/bdd`. BDD supplies
Threading, Utility, Macro, Logging and optionally Python Common transitively.
No direct dependency on Foundation, Kernel, Symbolic, SMT, or numerical libraries.

## Build and tests

```bash
git clone --recurse-submodules https://github.com/ariadne-cps/sat.git
cmake -S sat -B sat/build -DCMAKE_BUILD_TYPE=Release
cmake --build sat/build --parallel --target everything
ctest --test-dir sat/build --output-on-failure
```

Requires CMake 3.15+ and C++20. Python bindings are enabled by default when a
compatible Python 3.11+ development environment is available; set
`-DBUILD_PYTHON_BINDINGS=OFF` for C++ only. The combined `pyariadne` extension
exposes BDD and SAT types. The optional Python test suite uses pytest.

## Tutorials and benchmarks

The independent installed-library C++ tutorial lives under
`tutorials/tutorial_sat`, with a Python equivalent under
`python/tutorials/tutorial_sat.py`. The `benchmark_sat_pigeonhole` executable
tests small known SAT/UNSAT pigeonhole instances and reports BDD allocations.

## Limitations

No DIMACS parser, assumptions, clause learning, variable reordering, theory
reasoning or resource budgets yet. For design decisions and follow-up work,
see [the design journal](doc/designs/sat_design.md).

## License

GNU General Public License v3.0 or later.

## CI

Aligned with the current BDD and SMT workflows: Release build and installed tutorial checks, Debug C++/Python tests, separate ThreadSanitizer and AddressSanitizer/UBSan, and C++ coverage. Python installation uses a prefix-relative site-packages directory.
