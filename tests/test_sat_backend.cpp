/***************************************************************************
 *            test_sat_backend.cpp
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

/*! \file test_sat_backend.cpp
 *  \brief Regression tests for the internal SAT backend contract.
 */

#include <stdexcept>

#include "ariadne-testing.hpp"
#include "sat_backend.hpp"

using namespace Ariadne;

namespace {
class TestSatBackend : public Ariadne::Testing::ContextFixture {
  public:
    using ContextFixture::ContextFixture;

    Void test_backend_diagnostics() {
    auto bdd=make_sat_backend(1u,SatBackendKind::BDD);
    ARIADNE_TESTING_EXPECT_EQUAL(bdd->bdd_node_count(),0u);
    bdd->add_clause({SatLiteral(0u)});
    ARIADNE_TESTING_PROVE(bdd->bdd_node_count()>0u);
    BinaryWord bdd_model;
    ARIADNE_TESTING_PROVE(bdd->solve(bdd_model));
    ARIADNE_TESTING_EXPECT_EQUAL(bdd_model,make_binary_word("1"));

    auto cdcl=make_sat_backend(1u,SatBackendKind::CDCL);
    ARIADNE_TESTING_EXPECT_THROWS(std::logic_error, cdcl->bdd_node_count());
    // Rejecting an unsupported diagnostic must leave the backend usable.
    cdcl->add_clause({SatLiteral(0u,false)});
    BinaryWord cdcl_model;
    ARIADNE_TESTING_PROVE(cdcl->solve(cdcl_model));
    ARIADNE_TESTING_EXPECT_EQUAL(cdcl_model,make_binary_word("0"));
}

Void test_zero_variable_backend_results() {
    for(SatBackendKind kind:{SatBackendKind::BDD,SatBackendKind::CDCL}) {
        ARIADNE_TESTING_NOTE("Zero-variable backend "
            << (kind==SatBackendKind::BDD ? "BDD" : "CDCL"));
        auto backend=make_sat_backend(0u,kind);
        BinaryWord model;
        ARIADNE_TESTING_PROVE(backend->solve(model));
        ARIADNE_TESTING_PROVE(model.empty());
        backend->add_clause({});
        ARIADNE_TESTING_PROVE(not backend->solve(model));
        ARIADNE_TESTING_PROVE(model.empty());
    }
}
};
}

int main(int argc, char** argv) {
    ARIADNE_TESTING_SUITE("test_sat_backend");
    ARIADNE_TESTING_ADD_TEST(TestSatBackend, test_backend_diagnostics);
    ARIADNE_TESTING_ADD_TEST(TestSatBackend, test_zero_variable_backend_results);
    return ARIADNE_TESTING_RUN_TESTS_WITH_ARGS(argc, argv);
}
