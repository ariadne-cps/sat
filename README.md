# Ariadne SAT

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Unix](https://github.com/ariadne-cps/sat/actions/workflows/unix.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/unix.yml)
[![Windows](https://github.com/ariadne-cps/sat/actions/workflows/win.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/win.yml)
[![Sanitizers](https://github.com/ariadne-cps/sat/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/sanitizers.yml)
[![Coverage](https://github.com/ariadne-cps/sat/actions/workflows/coverage.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/coverage.yml)

Ariadne SAT is a C++20 propositional satisfiability solver with interchangeable
**CDCL (default)** and **ROBDD (optional)** backends. Both return exact SAT/UNSAT
answers and a total witness on SAT. The CDCL backend uses the shared
`ariadne-cps/cdcl` search engine; the BDD backend represents the full CNF
canonically. The latter may grow exponentially depending on variable order.

## Public API

- `SatLiteral(index,positive)`: zero-based variable index and optional polarity.
- `SatClause`: vector of literals; an empty clause is false.
- `SatSolver(number_of_variables,backend=SatBackendKind::CDCL)`: initially empty CNF, CDCL by default.
- `SatBackendKind::BDD`: explicitly selects the canonical ROBDD backend.
- `add_clause(clause)`: persistent incremental addition; validates all indices.
- `solve()`: exact SAT/UNSAT result. SAT results carry a total `BinaryWord` model.
- `model()`: throws `std::logic_error` for UNSAT.
- `variable_count()`, `clause_count()`, `backend_kind()`: backend-independent queries.
- `bdd_node_count()`: BDD-only diagnostic; throws for the CDCL backend.

```cpp
#include "ariadne-sat.hpp"
using namespace Ariadne;
SatSolver solver(2);
solver.add_clause({SatLiteral(0),SatLiteral(1)});
solver.add_clause({SatLiteral(0,false),SatLiteral(1)});
SatResult result=solver.solve(); // SAT, model 01
```

## Dependencies

Direct submodules: `ss/configuration`, `ss/bdd`, and `ss/cdcl`. BDD supplies
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
exposes BDD, CDCL, and SAT types. The optional Python test suite uses pytest.

## Tutorials and benchmarks

The independent installed-library C++ tutorial lives under
`tutorials/tutorial_sat`, with a Python equivalent under
`python/tutorials/tutorial_sat.py`. The `benchmark_sat_pigeonhole` executable
tests small known SAT/UNSAT pigeonhole instances on both backends and
reports BDD allocations only for BDD.

## Limitations

No DIMACS parser, assumptions, variable reordering or resource budgets yet.
Clause learning is implemented in the shared CDCL backend; the BDD backend
provides symbolic function representation instead. For design decisions and follow-up work,
see [the design journal](doc/designs/sat_design.md).

## License

GNU General Public License v3.0 or later.

## CI

Aligned with the current BDD and SMT workflows: Release build and installed tutorial checks, Debug C++/Python tests, separate ThreadSanitizer and AddressSanitizer/UBSan, and C++ coverage. Python installation uses a prefix-relative site-packages directory.
