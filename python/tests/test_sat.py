#!/usr/bin/python3

# Copyright 2026 Luca Geretti
# This file is part of Ariadne SAT (GPL-3.0-or-later).

import pytest
from pyariadne import SatLiteral, SatSolver, SatBackendKind

def test_sat_and_incremental_unsat():
    solver = SatSolver(2)
    solver.add_clause([SatLiteral(0), SatLiteral(1)])
    solver.add_clause([SatLiteral(0, False), SatLiteral(1)])
    result = solver.solve()
    assert result.is_sat()
    assert result.has_model()
    assert str(result.model()) == "01"
    assert solver.variable_count() == 2
    assert solver.clause_count() == 2

    solver.add_clause([SatLiteral(1, False)])
    result = solver.solve()
    assert result.is_unsat()
    assert not result.has_model()
    with pytest.raises(Exception):
        result.model()

def test_empty_formula_and_empty_clause():
    solver = SatSolver(0)
    assert solver.solve().is_sat()
    assert len(solver.solve().model()) == 0
    solver.add_clause([])
    assert solver.solve().is_unsat()

def test_invalid_literal_does_not_modify_formula():
    solver = SatSolver(1)
    with pytest.raises(IndexError):
        solver.add_clause([SatLiteral(0), SatLiteral(1)])
    assert solver.clause_count() == 0
    assert str(solver.solve().model()) == "0"

def test_cdcl_backend_incremental_and_bdd_metric():
    for kind in (SatBackendKind.BDD, SatBackendKind.CDCL):
        solver = SatSolver(2, kind)
        assert solver.backend_kind() == kind
        solver.add_clause([SatLiteral(0), SatLiteral(1)])
        solver.add_clause([SatLiteral(0, False), SatLiteral(1)])
        first = solver.solve()
        assert first.is_sat()
        assert str(first.model()) == "01"
        assert solver.solve().is_sat()
        solver.add_clause([SatLiteral(1, False)])
        assert solver.solve().is_unsat()
        if kind == SatBackendKind.CDCL:
            with pytest.raises(Exception):
                solver.bdd_node_count()

def test_cdcl_default_and_explicit_bdd():
    default = SatSolver(2)
    assert default.backend_kind() == SatBackendKind.CDCL
    with pytest.raises(Exception):
        default.bdd_node_count()

    bdd = SatSolver(2, SatBackendKind.BDD)
    assert bdd.backend_kind() == SatBackendKind.BDD
    bdd.add_clause([SatLiteral(0), SatLiteral(1)])
    assert bdd.solve().is_sat()
    assert bdd.bdd_node_count() > 0
    bdd.add_clause([SatLiteral(0, False)])
    bdd.add_clause([SatLiteral(1, False)])
    assert bdd.solve().is_unsat()
