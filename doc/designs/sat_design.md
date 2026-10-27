# Ariadne SAT design

This document is the design manual and implementation journal. Update it when
a design decision, milestone, benchmark, or limitation changes.

## Purpose and dependencies

The initial milestone is a minimal, exact propositional solver using the
public `RobddManager` API without modifying BDD. This is a correctness baseline,
not a CDCL solver, and does not solve SMT problems.

Direct dependencies are `configuration` and `bdd`. Configuration must match
the BDD transitive configuration gitlink, checked at configure time.
BDD supplies Threading, Utility, Macro, Logging and Python Common transitively.
`symbolic` is unnecessary for CNF, while `smt` is specialized for continuous
predicates and introduces large numerical dependencies.
`solving` has no reusable implementation at present.

## Contract

`SatLiteral` uses zero-based indices and an explicit polarity. An empty CNF is
SAT; an empty clause is UNSAT. `add_clause` first validates every literal,
ensuring an invalid clause does not modify the formula. Clauses persist across
`solve` calls. `SatResult::model()` throws for UNSAT. A SAT result includes
one total assignment of the initially declared number of variables.

The SMT internal Tseitin encoder instead uses signed **one-based** literals;
an integration layer must explicitly translate conventions.

## Algorithm

Each clause becomes the BDD disjunction of literals; the whole formula is the
conjunction of those clauses. A false root is an exact UNSAT result.

For SAT, maintain a satisfiable residual formula. Iterate over variables in
index order, tentatively conjoining with the negated variable. If the result
is nonfalse, choose false; otherwise choose true. Each step preserves
satisfiability. The final total assignment satisfies the original formula,
including deterministic assignments for otherwise unconstrained variables.

`bdd_node_count` counts allocated nonterminal nodes, including intermediates,
not just reachable nodes.

## Verification

C++ tests cover empty CNFs and clauses, contradictory units, mixed clauses,
tautologies, deterministic models, invalid indices, repeated/incremental solves,
and 128 small generated formulas checked against exhaustive truth tables.
Python tests exercise the public API. C++/Python standalone tutorials and
pigeonhole benchmark provide integration and known-answer checks.

## Limitations and roadmap

A whole-CNF ROBDD with fixed variable ordering may require exponential memory.
The API does not support resource exhaustion, UNKNOWN, assumptions, DIMACS,
or clause learning. Future options include a BDD cofactor/restrict and
direct witness-extraction interface, variable ordering experiments, resource
budgets, DIMACS ingestion, and a separate CDCL backend if workloads demand it.

## Implementation journal

- 2026-10-27: Initial CNF-to-ROBDD SAT solver, exact results and total models,
  direct dependency boundary, crosscheck tests, Python bindings, tutorials,
  CI and pigeonhole benchmark.

- 2026-10-27: Fixed the `ARIADNE_TEST_ASSERT` name collision (`result` renamed to `sat_result`) and aligned CI with the updated BDD/SMT structure: Release/Debug jobs, independent sanitizers and prefix-relative Python installation. Updated BDD gitlink to the workflow-alignment commit. CI execution remains to be verified.

### 2026-10-27 - Standardize source-file comment headers

Updated all C++ header and implementation file comments to use the full
Ariadne copyright and GPL-3.0-or-later notice, followed by a Doxygen file
name and brief description, matching the established SMT solver header style.
Only leading comment banners were changed; solver, tests, tutorials and
Python binding implementations are unchanged.

### 2026-10-27 - Backend-selectable SAT solver (BDD and CDCL)

Introduced the internal `SatBackend` abstraction with `BddSatBackend` and
`CdclSatBackend`. The public `SatSolver` accepts a `SatBackendKind`
constructor argument (BDD by default for compatibility), exposes
`backend_kind()`, and maintains the existing clause-count, incremental
`add_clause()`, repeated `solve()` and total `BinaryWord` model contracts.
The BDD backend retains the original canonical ROBDD implementation and
`bdd_node_count()`; that diagnostic throws for the CDCL backend, rather
than presenting a fabricated metric. CDCL reconstructs a one-shot search
from the persistent CNF at each solve. SAT deliberately retains its existing
`SatLiteral` API, translating explicitly to the shared `BooleanLiteral`
inside the CDCL adapter; this avoids a breaking type change or duplicate
Python registrations.

The new direct `cdcl` git submodule shares the configuration revision with
`bdd`. The aggregated Python extension includes the CDCL binding objects,
preventing unresolved transitive registration symbols. C++ and Python tests
exercise both backends, repeated solve and incremental UNSAT, with 128
deterministic generated CNFs cross-checked against one another and the
truth-table oracle. No SMT dependency is introduced.

### 2026-10-27 - Restore explicit type dependencies in the public SAT header

The backend extraction removed the transitive `bdd/robdd.hpp` include from
`sat/sat_solver.hpp`. A clean macOS build showed that this also removed
implicit definitions of `SizeType`, `Bool`, `StringType` and `StringStream`
needed by `SatSolver` and `utility/binary_word.hpp`. SAT now includes
`utility/typedefs.hpp` explicitly before `utility/binary_word.hpp`.
This is a header self-containment fix, not a reintroduction of the BDD
dependency into the public SAT API or a change to the backend semantics.

### 2026-10-27 - Handle unknown SAT backend enum values explicitly

The macOS warnings-as-errors configuration uses `-Wswitch-default`, which
requires a default label even when all declared `SatBackendKind` values
are covered. The backend factory now throws `std::invalid_argument` from
the `default` branch, preserving the existing invalid-selection contract
without suppressing warnings. Added a regression exercising an invalid
explicitly cast enum value. The BDD/CDCL dispatch and solver behavior for
valid backend kinds remain unchanged.

### 2026-10-27 - Promote CDCL to the default SAT backend

Following green Unix, Windows, Sanitizers, and Coverage checks for the
dual-backend implementation, `SatSolver(variable_count)` now selects CDCL.
BDD remains available explicitly as `SatSolver(count,SatBackendKind::BDD)`.
The Python constructor default matches C++; no backend is removed and the
incremental clause and total witness contracts remain unchanged. Existing
deterministic small-CNF reference tests continue running against CDCL by
default; cross-backend tests continue checking independent BDD results.

The pigeonhole benchmark explicitly exercises both backend kinds and prints
BDD node counts only for the BDD run, avoiding an invalid metric on CDCL.
C++ and Python tutorials show both default CDCL and explicit BDD selection.
New regression tests verify that the default kind really is CDCL and that
BDD-only statistics are rejected on the CDCL backend. The change is an
intentional default-behavior change: users relying on the previous ROBDD
representation or its diagnostics must select BDD explicitly.

### 2026-10-27 - Enforce matching Utility revisions across SAT backends

SAT now invokes `require_same_dependency_commit(Utility ...)` during
configuration with the two transitive gitlink chains
`bdd|threading|logging|utility` and `cdcl|utility`. This uses the
existing shared dependency checker, which resolves nested submodule paths
and compares their committed gitlink SHA values, failing with
`FATAL_ERROR` if they differ. It complements the existing Configuration
consistency check and runs before `setup_project()` and library builds.
At introduction both chains resolve to
`c3b31461cd3747f6c38640524d71899db0f69d4f`.
The check compares the declared Git revisions rather than the currently
checked-out worktree content, so a mismatched dependency cannot be hidden
by locally switching a nested submodule revision.
