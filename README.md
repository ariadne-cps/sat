# Ariadne SAT

[![Format Status](https://github.com/ariadne-cps/sat/actions/workflows/format.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/format.yml)
[![macOS Status](https://github.com/ariadne-cps/sat/actions/workflows/macos.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/macos.yml)
[![Ubuntu Status](https://github.com/ariadne-cps/sat/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/ariadne-cps/sat/actions/workflows/win.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/win.yml)
[![Sanitizers](https://github.com/ariadne-cps/sat/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/sanitizers.yml)
[![Coverage Status](https://github.com/ariadne-cps/sat/actions/workflows/coverage.yml/badge.svg)](https://github.com/ariadne-cps/sat/actions/workflows/coverage.yml)
[![codecov](https://codecov.io/gh/ariadne-cps/sat/branch/main/graph/badge.svg)](https://codecov.io/gh/ariadne-cps/sat)

Ariadne SAT is a C++20 propositional satisfiability solver with interchangeable
**CDCL (default)** and **ROBDD (optional)** backends. Both return exact SAT/UNSAT
answers and a total witness on SAT. The CDCL backend uses the shared
`ariadne-cps/cdcl` search engine; the BDD backend represents the full CNF
canonically. The latter may grow exponentially depending on variable order.
Although independently maintained and usable, this repository is also part of the [Ariadne repository structure](https://github.com/ariadne-cps/ariadne/blob/main/doc/structure.md).

## Layout

Public headers live under `include/sat`, with umbrella header
`include/ariadne-sat.hpp`. Implementations are under `src`, C++ regression
tests under `tests`, comparative pigeonhole and N-queens benchmarks under
`benchmarks`, Python bindings and tests under `python`, and standalone C++
examples under `tutorial/` (C++ and Python). The implementation journal is
[`doc/sat_design.md`](doc/sat_design.md).

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

Direct submodules: `ss/bui`, `ss/bdd`, `ss/cdc`, and `ss/bmk` (Benchmarking,
used only by standalone benchmark executables). BDD supplies
Threading, Utility, Macro, Logging and optionally Python Common transitively.
No direct dependency on Foundation, Kernel, Symbolic, SMT, or numerical libraries.

## Build

```bash
git clone --recurse-submodules https://github.com/ariadne-cps/sat.git
cmake -S sat -B sat/build -DCMAKE_BUILD_TYPE=Release
cmake --build sat/build --parallel --target ariadne-sat pyariadne
cmake --build sat/build --parallel --target benchmarks
ctest --test-dir sat/build --output-on-failure
```

Requires CMake 3.15+ and C++20.

## Python bindings

When a compatible Python 3.11+ development environment is available, the
standalone build produces the combined `pyariadne_sat` extension with BDD,
CDCL and SAT bindings. For a C++-only build, configure with
`-DBUILD_PYTHON_BINDINGS=OFF`. Python regression tests use pytest.

## Benchmarks

The `benchmark_sat_pigeonhole` executable
tests small known SAT/UNSAT pigeonhole instances on both backends and
reports BDD allocations only for BDD.

Both comparative benchmarks use the shared Ariadne Benchmarking framework.
Their CLI accepts `--backend both|bdd|cdcl`, an odd `--samples` count between 3
and 101 (default: 5), `--warmups` (default: 0), `--summary`, and the standard
logging/threading options. Each timed round alternates BDD/CDCL execution order
to reduce drift. CNF construction and witness validation remain outside timing.
A fresh solver is built for each sample. Separate phase measurements in
nanoseconds report solver construction and clause insertion, SAT/UNSAT solving,
and their total, along with BDD node counts and median total-time ratios.

```bash
./build/benchmarks/benchmark_sat_pigeonhole --max-holes 3 --samples 5 --warmups 1 --backend both
./build/benchmarks/benchmark_sat_pigeonhole --backend cdcl --max-holes 4 --verbosity 2
./build/benchmarks/benchmark_sat_nqueens --n 4 --backend both --samples 5 --warmups 1
./build/benchmarks/benchmark_sat_nqueens --n 8 --backend cdcl --samples 3
```

Pigeonhole evaluates both SAT and UNSAT instances for each hole count.
N-Queens checks SAT/UNSAT against the chessboard theorem (N=1 or N>=4 is SAT)
and independently validates each returned SAT arrangement. BDD can consume
substantial memory on larger cases. The CI builds benchmarks after the library
and Python bindings in Release without executing the benchmark binaries.

## Limitations

No DIMACS parser, assumptions, variable reordering or resource budgets yet.
Clause learning is implemented in the shared CDCL backend; the BDD backend
provides symbolic function representation instead. For design decisions and follow-up work,
see [the design journal](doc/sat_design.md).

## Contribution guidelines

If you would like to contribute to Ariadne SAT, please contact the developer:

- Luca Geretti <luca.geretti@univr.it>
