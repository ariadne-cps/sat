#!/usr/bin/python3

# Copyright 2026 Luca Geretti
# This file is part of Ariadne SAT (GPL-3.0-or-later).

from pyariadne import SatLiteral, SatSolver

def main():
    solver = SatSolver(2)
    solver.add_clause([SatLiteral(0), SatLiteral(1)])
    solver.add_clause([SatLiteral(0, False), SatLiteral(1)])
    first = solver.solve()
    assert first.is_sat()
    assert str(first.model()) == "01"
    print("Initial CNF: SAT, model =", first.model())
    solver.add_clause([SatLiteral(1, False)])
    second = solver.solve()
    assert second.is_unsat()
    print("After adding NOT x1: UNSAT")
    print("Ariadne SAT Python tutorial completed successfully.")

if __name__ == "__main__":
    main()
