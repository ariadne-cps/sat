/***************************************************************************
 *            sat_benchmarking_common.hpp
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne SAT. If not, see <https://www.gnu.org/licenses/>.
 */

/** @file benchmarks/sat_benchmarking_common.hpp
 *  @brief Shared benchmark configuration, timing and comparison helpers for SAT backends.
 */

#ifndef ARIADNE_SAT_BENCHMARKING_COMMON_HPP
#define ARIADNE_SAT_BENCHMARKING_COMMON_HPP

#include "ariadne-benchmarking.hpp"
#include "sat/sat_solver.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace SatBenchmark {

using namespace Ariadne;
namespace Bench=Ariadne::Benchmarking;

/** @brief Select which SAT solving backends to benchmark. */
enum class BackendSelection { BOTH, BDD, CDCL };

/** @brief Check whether a backend is selected for a benchmark run. */
inline bool enabled(BackendSelection selection,SatBackendKind kind) {
    return selection==BackendSelection::BOTH ||
        (selection==BackendSelection::BDD && kind==SatBackendKind::BDD) ||
        (selection==BackendSelection::CDCL && kind==SatBackendKind::CDCL);
}

/** @brief Create command-line parsers for SAT backend selection. */
inline List<ArgumentParser> backend_parser(BackendSelection& selection) {
    List<ArgumentParser> parsers;
    parsers.append(make_valued_argument_parser(
        "b","backend","Backend selection [ both | bdd | cdcl ]",
        [&selection](String const& text)->VoidFunction {
            BackendSelection value;
            if(text=="both") value=BackendSelection::BOTH;
            else if(text=="bdd") value=BackendSelection::BDD;
            else if(text=="cdcl") value=BackendSelection::CDCL;
            else throw std::invalid_argument("backend must be both, bdd or cdcl");
            return [&selection,value] { selection=value; };
        }));
    return parsers;
}

/** @brief Parse a bounded odd number of benchmark samples. */
inline unsigned odd_sample_count(String const& text) {
    unsigned const value=Bench::parse_unsigned<unsigned>(text);
    if(value<3u || value>101u || value%2u==0u)
        throw std::invalid_argument("samples must be odd, between 3 and 101");
    return value;
}

/** @brief Timings and BDD node count recorded for one SAT backend. */
struct PhaseSamples {
    Bench::Samples build;
    Bench::Samples solve;
    Bench::Samples total;
    SizeType bdd_nodes=0u;
};

/** @brief Optional benchmark measurements for both supported backends. */
struct Comparison {
    std::optional<PhaseSamples> bdd;
    std::optional<PhaseSamples> cdcl;
};

/** @brief Per-round solver state and build/solve timings. */
struct Attempt {
    std::optional<SatSolver> solver;
    std::optional<SatResult> result;
    double build_ns=0.0;
    double solve_ns=0.0;
    double total_ns=0.0;
};

// Each Runner round is a pair of backend executions (or just the selected
// backend). This retains the original alternating order and lets Runner
// handle all warm-ups, while separate clocks report build/solve/total.
// Input CNF creation and all correctness checks remain outside phase clocks.
/** @brief Compare the selected SAT backends while checking expected results and satisfying assignments.
 *  @tparam WitnessCheck Callable checking a satisfying assignment.
 */
template<class WitnessCheck>
Comparison run_comparison(Bench::Runner const& runner,
                          std::string const& name,unsigned samples,
                          SizeType variables,std::vector<SatClause> const& clauses,
                          Bool expected_sat,BackendSelection selection,
                          WitnessCheck&& witness_check) {
    Comparison output;
    if(enabled(selection,SatBackendKind::BDD)) output.bdd.emplace();
    if(enabled(selection,SatBackendKind::CDCL)) output.cdcl.emplace();

    std::array<Attempt,2> attempts;
    unsigned round=0u;
    Bench::Samples const completed=runner.run(name,samples,
        [&] {
            for(auto& attempt:attempts) {
                attempt.result.reset();
                attempt.solver.reset();
            }
        },
        [&] {
            std::array<SatBackendKind,2> const order=round%2u==0u
                ? std::array<SatBackendKind,2>{SatBackendKind::BDD,SatBackendKind::CDCL}
                : std::array<SatBackendKind,2>{SatBackendKind::CDCL,SatBackendKind::BDD};
            for(SatBackendKind kind:order) {
                if(not enabled(selection,kind)) continue;
                Attempt& attempt=attempts[kind==SatBackendKind::BDD ? 0u : 1u];
                auto const start=Bench::Clock::now();
                attempt.solver.emplace(variables,kind);
                for(auto const& clause:clauses) attempt.solver->add_clause(clause);
                auto const built=Bench::Clock::now();
                attempt.result.emplace(attempt.solver->solve());
                auto const finished=Bench::Clock::now();
                attempt.build_ns=Bench::elapsed_ns(start,built);
                attempt.solve_ns=Bench::elapsed_ns(built,finished);
                attempt.total_ns=Bench::elapsed_ns(start,finished);
            }
        },
        [&] {
            for(SatBackendKind kind:{SatBackendKind::BDD,SatBackendKind::CDCL}) {
                if(not enabled(selection,kind)) continue;
                Attempt const& attempt=attempts[kind==SatBackendKind::BDD ? 0u : 1u];
                if(not attempt.solver || not attempt.result)
                    throw std::runtime_error("Missing SAT benchmark result");
                if(attempt.result->is_sat()!=expected_sat)
                    throw std::runtime_error("Incorrect SAT/UNSAT benchmark answer");
                if(attempt.result->is_sat() &&
                    (attempt.result->model().size()!=variables ||
                     not witness_check(attempt.result->model())))
                    throw std::runtime_error("SAT benchmark returned invalid witness");
                if(attempt.solver->clause_count()!=clauses.size())
                    throw std::runtime_error("SAT backend modified input clause count");
                SizeType const nodes=kind==SatBackendKind::BDD
                    ? attempt.solver->bdd_node_count() : 0u;
                if(round>=runner.warmups()) {
                    PhaseSamples& values=kind==SatBackendKind::BDD
                        ? *output.bdd : *output.cdcl;
                    values.build.add(attempt.build_ns);
                    values.solve.add(attempt.solve_ns);
                    values.total.add(attempt.total_ns);
                    values.bdd_nodes=nodes;
                }
            }
            ++round;
        });

    if(completed.size()!=samples ||
       (output.bdd && output.bdd->total.size()!=samples) ||
       (output.cdcl && output.cdcl->total.size()!=samples))
        throw std::runtime_error("Incorrect SAT benchmark sample count");
    return output;
}

/** @brief Append the measured build, solve and total timing rows to a benchmark report. */
inline void add_rows(Bench::Report& report,std::vector<std::string> dimensions,
                     Comparison const& comparison) {
    auto add_backend=[&](char const* name,PhaseSamples const& values,bool bdd) {
        auto base=dimensions;
        base.push_back(name);
        base.push_back(bdd ? std::to_string(values.bdd_nodes) : "-");
        for(auto const& phase:std::array<std::pair<char const*,Bench::Samples const*>,3>{{
            {"build",&values.build},{"solve",&values.solve},{"total",&values.total}
        }}) {
            auto row=base;
            row.push_back(phase.first);
            report.add_row(std::move(row),Bench::summarize(*phase.second));
        }
    };
    if(comparison.bdd) add_backend("BDD",*comparison.bdd,true);
    if(comparison.cdcl) add_backend("CDCL",*comparison.cdcl,false);
}

/** @brief Return the ratio of median BDD and CDCL total times, when both are present. */
inline std::optional<double> bdd_cdcl_total_ratio(Comparison const& result) {
    if(not result.bdd || not result.cdcl) return std::nullopt;
    double const bdd=Bench::summarize(result.bdd->total).median;
    double const cdcl=Bench::summarize(result.cdcl->total).median;
    if(not std::isfinite(bdd) || not std::isfinite(cdcl) || cdcl<=0.0)
        throw std::runtime_error("Invalid SAT comparative timing");
    return bdd/cdcl;
}

} // namespace SatBenchmark

#endif
