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
