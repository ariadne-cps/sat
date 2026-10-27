/***************************************************************************
 *            test_sat_solver.cpp
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

/*! \file test_sat_solver.cpp
 *  \brief Regression tests for the propositional SAT solver.
 */

#include <vector>

#include "macro/testing.hpp"
#include "sat/sat_solver.hpp"

using namespace Ariadne;

namespace {

Bool satisfies(SatClause const& clause, BinaryWord const& model) {
    for(auto const& literal : clause) {
        if(model[literal.variable()] == literal.positive()) {
            return true;
        }
    }
    return false;
}

Bool satisfies(std::vector<SatClause> const& clauses, BinaryWord const& model) {
    for(auto const& clause : clauses) {
        if(not satisfies(clause,model)) {
            return false;
        }
    }
    return true;
}

class TestSatSolver {
  public:
    void test_empty_and_units() {
        SatSolver empty(0u);
        ARIADNE_TEST_EQUALS(empty.variable_count(),0u)
        ARIADNE_TEST_EQUALS(empty.clause_count(),0u)
        ARIADNE_TEST_ASSERT(empty.solve().is_sat())
        ARIADNE_TEST_ASSERT(empty.solve().model().empty())

        empty.add_clause({});
        ARIADNE_TEST_ASSERT(empty.solve().is_unsat())
        ARIADNE_TEST_ASSERT(not empty.solve().has_model())
        ARIADNE_TEST_FAIL(empty.solve().model())

        SatSolver positive(1u);
        positive.add_clause({SatLiteral(0u)});
        ARIADNE_TEST_ASSERT(positive.solve().is_sat())
        ARIADNE_TEST_EQUALS(positive.solve().model(),make_binary_word("1"))

        SatSolver negative(1u);
        negative.add_clause({SatLiteral(0u,false)});
        ARIADNE_TEST_EQUALS(negative.solve().model(),make_binary_word("0"))

        positive.add_clause({SatLiteral(0u,false)});
        ARIADNE_TEST_ASSERT(positive.solve().is_unsat())
    }

    void test_model_and_incrementality() {
        SatSolver solver(3u);
        solver.add_clause({SatLiteral(0u),SatLiteral(1u)});
        solver.add_clause({SatLiteral(0u,false),SatLiteral(1u)});
        ARIADNE_TEST_EQUALS(solver.clause_count(),2u)
        ARIADNE_TEST_ASSERT(solver.solve().is_sat())
        ARIADNE_TEST_EQUALS(solver.solve().model(),make_binary_word("010"))

        // A tautology changes the clause count, not satisfiability.
        solver.add_clause({SatLiteral(2u),SatLiteral(2u,false)});
        ARIADNE_TEST_ASSERT(solver.solve().is_sat())

        solver.add_clause({SatLiteral(1u,false)});
        ARIADNE_TEST_ASSERT(solver.solve().is_unsat())
        ARIADNE_TEST_ASSERT(solver.solve().is_unsat())
    }

    void test_invalid_literal_is_atomic() {
        SatSolver solver(2u);
        ARIADNE_TEST_FAIL(solver.add_clause({SatLiteral(0u),SatLiteral(2u)}))
        ARIADNE_TEST_EQUALS(solver.clause_count(),0u)
        ARIADNE_TEST_ASSERT(solver.solve().is_sat())
        ARIADNE_TEST_EQUALS(solver.solve().model(),make_binary_word("00"))
    }

    void test_deterministic_crosscheck() {
        // Small exhaustive truth-table oracle against many generated CNFs.
        for(SizeType seed=0u; seed!=128u; ++seed) {
            SatSolver solver(3u);
            std::vector<SatClause> clauses;
            for(SizeType j=0u; j!=6u; ++j) {
                SizeType code=(seed*11u+j*19u+7u)%27u;
                SatClause clause;
                for(SizeType v=0u; v!=3u; ++v) {
                    SizeType digit=code%3u;
                    code/=3u;
                    if(digit==1u) clause.emplace_back(v,true);
                    if(digit==2u) clause.emplace_back(v,false);
                }
                solver.add_clause(clause);
                clauses.push_back(std::move(clause));
            }

            Bool expected=false;
            for(SizeType bits=0u; bits!=8u; ++bits) {
                BinaryWord model;
                for(SizeType v=0u; v!=3u; ++v) {
                    model.append(((bits>>v)&1u)!=0u);
                }
                expected = expected or satisfies(clauses,model);
            }

            SatResult sat_result=solver.solve();
            ARIADNE_TEST_ASSERT(sat_result.is_sat()==expected)
            if(sat_result.is_sat()) {
                ARIADNE_TEST_EQUALS(sat_result.model().size(),3u)
                ARIADNE_TEST_ASSERT(satisfies(clauses,sat_result.model()))
            }
        }
    }

    void test_cross_backend() {
        for(SatBackendKind backend:{SatBackendKind::BDD,SatBackendKind::CDCL}) {
            SatSolver solver(3u,backend);
            ARIADNE_TEST_ASSERT(solver.backend_kind()==backend)
            solver.add_clause({SatLiteral(0u),SatLiteral(1u)});
            solver.add_clause({SatLiteral(0u,false),SatLiteral(1u)});
            ARIADNE_TEST_ASSERT(solver.solve().is_sat())
            ARIADNE_TEST_ASSERT(solver.solve().is_sat())
            ARIADNE_TEST_ASSERT(satisfies(
                {SatClause{SatLiteral(0u),SatLiteral(1u)},
                 SatClause{SatLiteral(0u,false),SatLiteral(1u)}},
                solver.solve().model()))
            solver.add_clause({SatLiteral(1u,false)});
            ARIADNE_TEST_ASSERT(solver.solve().is_unsat())
            ARIADNE_TEST_ASSERT(solver.solve().is_unsat())
            if(backend==SatBackendKind::CDCL) {
                ARIADNE_TEST_FAIL(solver.bdd_node_count())
            }
        }
        for(SizeType seed=0u;seed!=128u;++seed) {
            SatSolver bdd(3u,SatBackendKind::BDD);
            SatSolver cdcl(3u,SatBackendKind::CDCL);
            std::vector<SatClause> clauses;
            for(SizeType j=0u;j!=6u;++j) {
                SizeType code=(seed*11u+j*19u+7u)%27u;
                SatClause clause;
                for(SizeType v=0u;v!=3u;++v) {
                    SizeType digit=code%3u;
                    code/=3u;
                    if(digit==1u) clause.emplace_back(v,true);
                    if(digit==2u) clause.emplace_back(v,false);
                }
                bdd.add_clause(clause);
                cdcl.add_clause(clause);
                clauses.push_back(std::move(clause));
            }
            SatResult lhs=bdd.solve();
            SatResult rhs=cdcl.solve();
            ARIADNE_TEST_ASSERT(lhs.is_sat()==rhs.is_sat())
            if(lhs.is_sat()) {
                ARIADNE_TEST_EQUALS(lhs.model().size(),3u)
                ARIADNE_TEST_EQUALS(rhs.model().size(),3u)
                ARIADNE_TEST_ASSERT(satisfies(clauses,lhs.model()))
                ARIADNE_TEST_ASSERT(satisfies(clauses,rhs.model()))
            }
        }
    }

    void test() {
        ARIADNE_TEST_CALL(test_cross_backend())
        ARIADNE_TEST_CALL(test_empty_and_units())
        ARIADNE_TEST_CALL(test_model_and_incrementality())
        ARIADNE_TEST_CALL(test_invalid_literal_is_atomic())
        ARIADNE_TEST_CALL(test_deterministic_crosscheck())
    }
};

} // namespace

int main() {
    TestSatSolver().test();
    return ARIADNE_TEST_FAILURES;
}
